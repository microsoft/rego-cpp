#include "internal.hh"
#include "rego.hh"

#include <cstdint>
#include <iterator>
#include <stdexcept>

namespace
{
  using namespace trieste;

  struct DebugIdx
  {
    DebugIdx(size_t n) : n(n) {}

    size_t n;
  };

  std::ostream& operator<<(std::ostream& os, const DebugIdx& idx)
  {
    os << std::setfill('0') << std::setw(2) << idx.n << "  ";
    return os;
  }

  struct BlockIndent
  {};

  struct BlockUndent
  {};

  std::ostream& operator<<(std::ostream& os, const BlockIndent&)
  {
    os << "++";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BlockUndent&)
  {
    os << "--";
    return os;
  }
}

namespace rego
{
  namespace b = bundle;

  std::ostream& operator<<(std::ostream& os, const DebugKey& node)
  {
    return os << rego::to_key(node.n);
  }

  VirtualMachine::VirtualMachine() :
    m_int_regex(R"(-?(?:0|[1-9][0-9]*))"),
    m_stmt_limit(10000000),
    m_max_call_depth(512),
    m_max_block_depth(512)
  {}

  VirtualMachine& VirtualMachine::bundle(Bundle bundle)
  {
    if (bundle != nullptr)
    {
      // Sole structural-verification point: walk the bundle once at the
      // public API boundary so the VM is never driven by malformed
      // bytecode, regardless of how the BundleDef was produced (binary
      // load, JSON IR, or programmatic construction by an embedder).
      // verify() throws on any inconsistency.
      bundle->verify();
    }
    m_bundle = bundle;
    return *this;
  }

  Bundle VirtualMachine::bundle() const
  {
    return m_bundle;
  }

  VirtualMachine& VirtualMachine::builtins(BuiltIns builtins)
  {
    m_builtins = builtins;
    return *this;
  }

  BuiltIns VirtualMachine::builtins() const
  {
    return m_builtins;
  }

  bool VirtualMachine::State::is_in_call_stack(const Location& func_name) const
  {
    auto it = std::find(m_call_stack.begin(), m_call_stack.end(), func_name);
    return it != m_call_stack.end();
  }

  void VirtualMachine::State::push_function(
    const Location& func_name, size_t num_args)
  {
    m_call_stack.push_back(func_name);
    m_num_args.push_back(num_args);
  }

  void VirtualMachine::State::pop_function(const Location& func_name)
  {
    assert(m_call_stack.back() == func_name);
    m_call_stack.pop_back();
    m_num_args.pop_back();
  }

  void VirtualMachine::State::put_function_result(
    const Location& func_name, Node result)
  {
    m_function_cache[func_name] = result;
  }

  Node VirtualMachine::State::get_function_result(
    const Location& func_name) const
  {
    if (in_with())
    {
      return nullptr;
    }

    auto it = m_function_cache.find(func_name);
    if (it == m_function_cache.end())
    {
      return nullptr;
    }

    return it->second;
  }

  bool VirtualMachine::State::in_with() const
  {
    return m_with_count > 0;
  }

  void VirtualMachine::State::push_with()
  {
    m_with_count++;
  }

  void VirtualMachine::State::pop_with()
  {
    assert(m_with_count > 0);
    m_with_count--;
  }

  bool VirtualMachine::State::in_break() const
  {
    return m_break_count > 0;
  }

  void VirtualMachine::State::push_break(size_t levels)
  {
    m_break_count += levels;
  }

  void VirtualMachine::State::pop_break()
  {
    assert(m_break_count > 0);
    m_break_count--;
  }

  std::string_view VirtualMachine::State::root_function_name() const
  {
    if (m_call_stack.empty())
    {
      return "<invalid>";
    }

    std::string_view name = m_call_stack.front().view();
    if (name.starts_with("g0.querymodule$"))
    {
      if (m_call_stack.size() == 1)
      {
        return "<invalid>";
      }

      name = m_call_stack.at(1).view();
    }

    name = name.substr(name.find('.') + 1);
    return name;
  }

  void VirtualMachine::State::add_result(Node node)
  {
    m_result_set.push_back(node);
  }

  const Nodes& VirtualMachine::State::result_set() const
  {
    return m_result_set;
  }

  void VirtualMachine::State::add_error(Node node)
  {
    m_errors.push_back(node);
  }

  void VirtualMachine::State::add_error_multiple_output(Node node)
  {
    const std::string comp_message =
      "complete rules must not produce multiple outputs";
    const std::string func_message =
      "functions must not produce multiple outputs for same inputs";
    if (m_num_args.empty() || m_num_args.back() == 2)
    {
      add_error(err(node, comp_message, EvalConflictError));
      return;
    }

    add_error(err(node, func_message, EvalConflictError));
  }

  void VirtualMachine::State::add_error_object_insert(Node node)
  {
    const std::string message = "object keys must be unique";
    add_error(err(node, message, EvalConflictError));
  }

  const Nodes& VirtualMachine::State::errors() const
  {
    return m_errors;
  }

  Node VirtualMachine::State::read_local(size_t key) const
  {
    assert(key < m_frame.size());

    if (m_frame[key] != nullptr)
    {
      logging::Trace() << "frame[" << key << "]" << " -> "
                       << DebugKey(m_frame[key]);
      return m_frame[key];
    }

    logging::Trace() << "frame[" << key << "]" << " -> " << "<missing>";
    return Undefined;
  }

  void VirtualMachine::State::write_local(size_t key, Node value)
  {
    assert(key < m_frame.size());

    if (value == Term)
    {
      value = value->front();
    }

    if (value == Scalar)
    {
      value = value->front();
    }

    if (value == Undefined)
    {
      return reset_local(key);
    }

    if (value == nullptr)
    {
      logging::Error() << "Attempting to write null value to local: " << key;
      throw std::runtime_error("Cannot write null value to local variable");
    }

    m_frame[key] = value;
    logging::Trace() << DebugKey(value) << " -> frame[" << key << "]";
  }

  bool VirtualMachine::State::is_defined(size_t key) const
  {
    assert(key < m_frame.size());

    if (m_frame[key] == nullptr || m_frame[key] == Undefined)
    {
      logging::Trace() << "frame[" << key << "]" << " -> " << "<missing>";
      return false;
    }

    logging::Trace() << "frame[" << key << "]" << " -> "
                     << DebugKey(m_frame[key]);
    return true;
  }

  void VirtualMachine::State::reset_local(size_t key)
  {
    assert(key < m_frame.size());

    logging::Debug() << "reset frame[" << key << "]";
    m_frame[key] = nullptr;
  }

  Node VirtualMachine::unpack_operand(
    const State& state, const b::Operand& operand) const
  {
    switch (operand.type)
    {
      case b::OperandType::Local:
        return state.read_local(operand.index);

      case b::OperandType::String:
        assert(operand.index < m_bundle->strings.size());
        return JSONString ^ m_bundle->strings[operand.index];

      case b::OperandType::False:
        return False ^ "false";

      case b::OperandType::True:
        return True ^ "true";

      case b::OperandType::Index:
      case b::OperandType::Value:
        throw std::runtime_error("Unpacking raw operand");

      case b::OperandType::None:
        throw std::runtime_error("Uninitialized operand");
    }

    throw std::runtime_error("Invalid operand");
  }

  size_t VirtualMachine::State::stmt_count() const
  {
    return m_stmt_count;
  }

  size_t VirtualMachine::State::inc_stmts()
  {
    m_stmt_count += 1;
    return m_stmt_count;
  }

  size_t VirtualMachine::State::call_depth() const
  {
    return m_call_stack.size();
  }

  size_t VirtualMachine::State::block_depth() const
  {
    return m_block_depth;
  }

  void VirtualMachine::State::enter_block()
  {
    m_block_depth += 1;
  }

  void VirtualMachine::State::leave_block()
  {
    assert(m_block_depth > 0 && "leave_block underflow");
    m_block_depth -= 1;
  }

  VirtualMachine::State::State(Node input, Node data, size_t num_locals) :
    m_with_count(0), m_break_count(0), m_stmt_count(0), m_block_depth(0)
  {
    m_frame.resize(num_locals, nullptr);
    write_local(0, input->front());
    write_local(1, data);
  }

  Node VirtualMachine::run_query(Node input) const
  {
    logging::Debug() << "Input: " << input;

    if (m_bundle == nullptr)
    {
      return ErrorSeq << err(Line ^ Location("<query>"), "no bundle loaded");
    }

    auto maybe_index = m_bundle->query_plan;
    if (!maybe_index.has_value())
    {
      logging::Error() << "Plan not found for query";
      return ErrorSeq << err(
               Line ^ Location("<query>"), "query plan not found");
    }

    State state(input, m_bundle->data, m_bundle->local_count);
    run_plan(m_bundle->plans[*maybe_index], state);

    if (!state.errors().empty())
    {
      return ErrorSeq << state.errors();
    }

    if (state.result_set().empty())
    {
      return Undefined;
    }

    Node results = NodeDef::create(Results);
    for (Node result : state.result_set())
    {
      Node result_obj = (result->front() / Val)->front();

      Nodes nodes = Resolver::object_lookdown(result_obj, "\"expressions\"");
      Node terms = NodeDef::create(Terms);
      if (!nodes.empty())
      {
        Node expressions_arr = nodes.front()->front();
        if (expressions_arr->empty())
        {
          continue;
        }

        for (auto expr : *expressions_arr)
        {
          auto maybe_object = unwrap(expr, Object);
          if (!maybe_object.success)
          {
            return ErrorSeq << err(expr, "Expected a results object");
          }

          Nodes values =
            Resolver::object_lookdown(maybe_object.node, "\"value\"");
          if (values.size() != 1)
          {
            return ErrorSeq
              << err(maybe_object.node, "No results in result object");
          }

          terms << values.front();
        }
      }

      nodes = Resolver::object_lookdown(result_obj, "\"bindings\"");
      Node bindings = NodeDef::create(Bindings);
      if (!nodes.empty())
      {
        Node bindings_obj = nodes.front()->front();
        for (Node item : *bindings_obj)
        {
          bindings
            << (Binding << (Var ^ strip_quotes(to_key(item / Key)))
                        << item / Val);
        }
      }

      results << (Result << terms << bindings);
    }

    Node ast = Top << results;
    wf_result.build_st(ast);

    logging::Debug() << "Results: " << results;

    return results;
  }

  Node VirtualMachine::run_entrypoint(
    const Location& entrypoint, Node input) const
  {
    if (m_bundle == nullptr)
    {
      return ErrorSeq << err(Line ^ Location("<query>"), "no bundle loaded");
    }

    auto maybe_index = m_bundle->find_plan(entrypoint);
    if (!maybe_index.has_value())
    {
      logging::Error() << "Plan not found for entrypoint: "
                       << entrypoint.view();
      return ErrorSeq << err(Line ^ entrypoint, "entrypoint not found");
    }

    if (input != Input)
    {
      logging::Error() << "Input node is not of type Input: " << input;
      return ErrorSeq << err(input, "Invalid input node");
    }

    logging::Debug() << "Input: " << input;

    State state(input, m_bundle->data, m_bundle->local_count);
    run_plan(m_bundle->plans[*maybe_index], state);

    if (!state.errors().empty())
    {
      return ErrorSeq << state.errors();
    }

    if (state.result_set().empty())
    {
      return Undefined;
    }

    Node results = NodeDef::create(Results);
    for (Node result : state.result_set())
    {
      auto maybe_object = unwrap(result, Object);
      if (!maybe_object.success)
      {
        return ErrorSeq << err(result, "Expected a result object");
      }

      Nodes values = Resolver::object_lookdown(maybe_object.node, "\"result\"");
      if (values.size() != 1)
      {
        return ErrorSeq << err(
                 maybe_object.node, "No result values in result object");
      }

      results << (Result << (Terms << values.front()) << Bindings);
    }

    Node ast = Top << results;
    wf_result.build_st(ast);

    logging::Debug() << "Results: " << results;

    return results;
  }

  void VirtualMachine::run_plan(const b::Plan& plan, State& state) const
  {
    WFContext ctx({&wf_bundle, &wf_result});
    m_builtins->clear();

    for (b::Block block : plan.blocks)
    {
      if (run_block(state, block) != Code::Continue)
      {
        break;
      }
    }
  }

  VirtualMachine::Code VirtualMachine::run_block(
    State& state, const b::Block& block) const
  {
    // Bound stack growth from pathological block nesting (mirrors call_depth).
    if (state.block_depth() >= m_max_block_depth)
    {
      state.add_error(err(
        Error,
        "maximum block recursion depth (" + std::to_string(m_max_block_depth) +
          ") exceeded",
        RecursionError));
      return Code::Error;
    }

    struct BlockGuard
    {
      State& s;
      BlockGuard(State& st) : s(st)
      {
        s.enter_block();
      }
      ~BlockGuard()
      {
        s.leave_block();
      }
    } guard(state);

    Code code = Code::Continue;
    logging::Debug() << BlockIndent();
    {
      logging::LocalIndent indent;
      for (size_t i = 0; i < block.size(); ++i)
      {
        code = run_stmt(state, i, block[i]);
        switch (code)
        {
          case Code::Continue:
            continue;
          case Code::Undefined:
          case Code::Return:
          case Code::Error:
          case Code::Timeout:
            break;
          case Code::Break:
            state.pop_break();
            if (state.in_break())
            {
              return Code::Break;
            }
            break;
          default:
            logging::Error() << "Unexpected return code from run_stmt: "
                             << static_cast<int>(code);
            throw std::runtime_error("Unexpected return code from run_stmt");
        }

        break;
      }
    }
    logging::Debug() << BlockUndent();
    return code;
  }

  VirtualMachine::Code VirtualMachine::run_call(
    State& state,
    const Location& func,
    const std::vector<b::Operand>& args,
    size_t target) const
  {
    if (state.is_in_call_stack(func))
    {
      throw std::runtime_error(
        "Recursion detected in rule body: " +
        std::string(state.root_function_name()));
    }

    if (m_builtins->is_builtin(func))
    {
      Nodes arg_values;
      std::transform(
        args.begin(),
        args.end(),
        std::back_inserter(arg_values),
        [this, state](const b::Operand& arg) {
          return unpack_operand(state, arg);
        });

      Node value = m_builtins->call(func, {"v1"}, arg_values);
      state.write_local(target, value);
      if (value == Error)
      {
        state.add_error(value);
        return Code::Error;
      }

      return Code::Continue;
    }

    auto maybe_index = m_bundle->find_function(func);
    if (!maybe_index.has_value())
    {
      throw std::runtime_error(
        "Function not found: " + std::string(func.view()));
    }

    const b::Function& function = m_bundle->functions[*maybe_index];
    Node cached_result = state.get_function_result(function.name);
    if (cached_result != nullptr)
    {
      state.write_local(target, cached_result);
      if (cached_result == Error)
      {
        return Code::Error;
      }

      return Code::Continue;
    }

    if (args.size() < function.parameters.size())
    {
      // Guaranteed by BundleDef::verify() — call sites for user
      // functions supply at least the function's parameter count.
      // VirtualMachine::bundle() invokes verify() on bind, so this
      // path is unreachable for any well-formed embedder use.
      // Surface an explicit error instead of silently returning
      // Undefined so any future code path that mutates m_bundle
      // post-bind cannot drive a wrong-arity call into a quiet
      // mis-evaluation.
      assert(false && "VM invariant: call arity >= parameter count");
      state.add_error(err(
        Error,
        "internal: call arity mismatch for '" + std::string(func.view()) +
          "' (bundle was not verified)",
        EvalBuiltInError));
      return Code::Error;
    }

    for (size_t i = 2; i < function.parameters.size(); ++i)
    {
      state.write_local(function.parameters[i], unpack_operand(state, args[i]));
    }

    // Implementation-defined call-depth guard.
    //
    // OPA itself does not impose a maximum call depth, but the rego-cpp
    // interpreter recurses through the C++ stack on every user-function
    // call. A bundle with a long chain of mutually recursive functions
    // (each distinct, so the existing is_in_call_stack check does not
    // fire) could otherwise overflow the stack and crash the host.
    //
    // The default of 512 frames is sized for the smallest typical
    // platform thread stack (Windows: 1 MiB) with headroom for
    // sanitiser builds, while remaining well above any depth
    // observed in the OPA conformance suite. Embedders on platforms
    // with larger stacks (Linux/macOS: 8 MiB) may safely raise this
    // via VirtualMachine::max_call_depth(size_t) /
    // Interpreter::max_call_depth(size_t) if their workloads need
    // deeper chains.
    if (state.call_depth() >= m_max_call_depth)
    {
      state.add_error(err(
        Error,
        "interpreter stack-depth limit exceeded: "
        "maximum function call depth (" +
          std::to_string(m_max_call_depth) +
          ") reached. This is an implementation limit on C++ stack "
          "growth, not a Rego rule-recursion cycle; "
          "raise via Interpreter::max_call_depth(size_t).",
        RecursionError));
      return Code::Error;
    }

    state.push_function(func, function.arity);

    Code code;

    // Run the function's block
    for (const b::Block& block : function.blocks)
    {
      code = run_block(state, block);
      if (code == Code::Return)
      {
        break;
      }

      if (code == Code::Break)
      {
        break;
      }

      if (code == Code::Error)
      {
        break;
      }
    }

    state.pop_function(func);

    if (code == Code::Return)
    {
      Node value = state.read_local(function.result);
      state.write_local(target, value);
      if (function.cacheable && !state.in_with())
      {
        state.put_function_result(function.name, value);
      }

      if (value == Error)
      {
        state.add_error(value);
        return Code::Error;
      }

      return Code::Continue;
    }

    if (code == Code::Break)
    {
      return code;
    }

    return Code::Undefined;
  }

  VirtualMachine::Code VirtualMachine::run_stmt(
    State& state, size_t index, const b::Statement& stmt) const
  {
    if (state.inc_stmts() >= m_stmt_limit)
    {
      state.add_error(err(
        Line ^ stmt.location, "Maximum statement count reached", TimeoutError));
      return Code::Timeout;
    }

    logging::Debug() << DebugIdx(index) << stmt;
    switch (stmt.type)
    {
      case b::StatementType::MakeObject:
        state.write_local(stmt.target, NodeDef::create(Object));
        break;

      case b::StatementType::MakeArray:
        state.write_local(stmt.target, NodeDef::create(Array));
        break;

      case b::StatementType::MakeSet:
        state.write_local(stmt.target, NodeDef::create(Set));
        break;

      case b::StatementType::MakeNull:
        state.write_local(stmt.target, NodeDef::create(Null));
        break;

      case b::StatementType::MakeNumberRef: {
        const Location& num_value = m_bundle->strings[stmt.op0.index];
        if (TRegex::FullMatch(num_value.view(), m_int_regex))
        {
          state.write_local(stmt.target, Int ^ num_value);
        }
        else
        {
          state.write_local(stmt.target, Float ^ num_value);
        }
      }
      break;

      case b::StatementType::AssignInt:
      case b::StatementType::MakeNumberInt:
        state.write_local(stmt.target, Int ^ std::to_string(stmt.op0.value));
        break;

      case b::StatementType::Block:
        for (const b::Block& block : stmt.ext->blocks())
        {
          Code code = run_block(state, block);
          switch (code)
          {
            case Code::Continue:
            case Code::Undefined:
              continue;
            case Code::Return:
            case Code::Error:
            case Code::Break:
            case Code::Timeout:
              return code;
            default:
              logging::Error() << "Unexpected return code from block: "
                               << static_cast<int>(code);
              throw std::runtime_error("Unexpected return code from block");
          }
        }
        break;

      case b::StatementType::Len: {
        Node source = unpack_operand(state, stmt.op0);
        Node len = Int ^ std::to_string(source->size());
        state.write_local(stmt.target, Term << (Scalar << len));
      }
      break;

      case b::StatementType::IsObject:
        if (unpack_operand(state, stmt.op0) != Object)
        {
          return Code::Undefined;
        }
        break;

      case b::StatementType::IsArray:
        if (unpack_operand(state, stmt.op0) != Array)
        {
          return Code::Undefined;
        }
        break;

      case b::StatementType::IsSet:
        if (unpack_operand(state, stmt.op0) != Set)
        {
          return Code::Undefined;
        }
        break;

      case b::StatementType::ResetLocal:
        state.reset_local(stmt.target);
        break;

      case b::StatementType::AssignVarOnce: {
        Node source = unpack_operand(state, stmt.op0);
        if (source == Undefined)
        {
          return Code::Continue;
        }

        if (state.is_defined(stmt.target))
        {
          Node existing = state.read_local(stmt.target);
          std::string lhs = to_key(source);
          std::string rhs = to_key(existing);
          if (lhs == rhs)
          {
            return Code::Continue;
          }

          state.add_error_multiple_output(Line ^ stmt.location);
          return Code::Error;
        }

        state.write_local(stmt.target, source);
      }
      break;

      case b::StatementType::IsDefined:
        if (!state.is_defined(stmt.target))
        {
          return Code::Undefined;
        }
        break;

      case b::StatementType::IsUndefined:
        if (state.is_defined(stmt.target))
        {
          return Code::Undefined;
        }
        break;

      case b::StatementType::Not: {
        Code code = run_block(state, stmt.ext->block());
        if (code == Code::Error || code == Code::Timeout)
        {
          // Propagate genuine errors (including strict-mode builtin errors)
          // and timeouts instead of treating them as a failed negation. In
          // non-strict mode a builtin error has already become Undefined
          // before reaching here, so `not` still succeeds.
          logging::Debug() << DebugIdx(index) << "NotStmt() -> propagate";
          return code;
        }
        if (code != Code::Undefined)
        {
          logging::Debug() << DebugIdx(index) << "NotStmt() -> Undefined";
          return Code::Undefined;
        }
        logging::Debug() << DebugIdx(index) << "NotStmt() -> Continue";
        break;
      }

      case b::StatementType::ReturnLocal:
        return Code::Return;

      case b::StatementType::ObjectInsert: {
        Node key = unpack_operand(state, stmt.op0);
        Node value = unpack_operand(state, stmt.op1);
        Node object = state.read_local(stmt.target);
        if (object != nullptr)
        {
          if (insert_into_object(object, key, value, false))
          {
            state.add_error_object_insert(Line ^ stmt.location);
            return Code::Error;
          }
        }
      }
      break;

      case b::StatementType::ObjectInsertOnce: {
        Node key = unpack_operand(state, stmt.op0);
        Node value = unpack_operand(state, stmt.op1);
        Node object = state.read_local(stmt.target);
        if (object != nullptr)
        {
          if (insert_into_object(object, key, value, true))
          {
            state.add_error_object_insert(Line ^ stmt.location);
            return Code::Error;
          }
        }
      }
      break;

      case b::StatementType::ObjectMerge: {
        Node a = state.read_local(stmt.op0.index);
        Node b = state.read_local(stmt.op1.index);
        Node merged = merge_objects(a, b);
        state.write_local(stmt.target, merged);
      }
      break;

      case b::StatementType::ArrayAppend: {
        Node array = state.read_local(stmt.target);
        if (array != nullptr)
        {
          Node value = unpack_operand(state, stmt.op0);
          if (value == Undefined)
          {
            return Code::Undefined;
          }

          array << to_term(value);
        }
      }
      break;

      case b::StatementType::SetAdd: {
        Node set = state.read_local(stmt.target);
        if (set != nullptr)
        {
          Node value = unpack_operand(state, stmt.op0);
          if (value == Undefined)
          {
            return Code::Undefined;
          }

          // TODO need a better intermediate representation for sets
          std::set<std::string> members;
          std::transform(
            set->begin(),
            set->end(),
            std::inserter(members, members.end()),
            [](const Node& item) { return to_key(item); });
          if (members.find(to_key(value)) == members.end())
          {
            set << to_term(value);
          }
        }
      }
      break;

      case b::StatementType::Dot: {
        Node source = unpack_operand(state, stmt.op0);
        Node key = unpack_operand(state, stmt.op1);
        Node value = dot(source, key);
        if (value == nullptr)
        {
          logging::Warn() << "Dot operation returned null for source: "
                          << source->location().view()
                          << ", key: " << key->location().view();
          return Code::Undefined;
        }

        state.write_local(stmt.target, value);
      }
      break;

      case b::StatementType::AssignVar:
        state.write_local(stmt.target, unpack_operand(state, stmt.op0));
        break;

      case b::StatementType::ResultSetAdd: {
        Node value = state.read_local(stmt.target);
        if (value != nullptr)
        {
          state.add_result(value);
        }
      }
      break;

      case b::StatementType::Equal: {
        Node a = unpack_operand(state, stmt.op0);
        Node b = unpack_operand(state, stmt.op1);
        Node result = Resolver::boolinfix(NodeDef::create(Equals), a, b);
        if (result == False)
        {
          return Code::Undefined;
        }
      }
      break;

      case b::StatementType::NotEqual: {
        Node a = unpack_operand(state, stmt.op0);
        Node b = unpack_operand(state, stmt.op1);
        if (a == Undefined || b == Undefined)
        {
          return Code::Undefined;
        }
        Node result = Resolver::boolinfix(NodeDef::create(Equals), a, b);
        if (result == True)
        {
          return Code::Undefined;
        }
      }
      break;

      case b::StatementType::Call: {
        const b::CallExt& call = stmt.ext->call();
        return run_call(state, call.func, call.ops, stmt.target);
      }

      case b::StatementType::CallDynamic: {
        const b::CallDynamicExt& call_dynamic = stmt.ext->call_dynamic();
        bool found = false;
        size_t valid_index = 0;
        std::ostringstream path_buf;
        Location func;
        path_buf << "g0";
        for (size_t i = 0; i < call_dynamic.path.size(); ++i)
        {
          path_buf << "."
                   << strip_quotes(
                        to_key(unpack_operand(state, call_dynamic.path[i])));

          if (m_bundle->is_function(path_buf.str()))
          {
            func = path_buf.str();
            logging::Trace() << "dynamic path: " << func.view();
            valid_index = i;
            found = true;
          }
        }

        if (!found)
        {
          // No rule matched this dynamic path; Undefined runs OPA's fallback.
          logging::Trace() << "CallDynamic: no function matched path "
                           << path_buf.str();
          return Code::Undefined;
        }

        Code code = run_call(state, func, call_dynamic.ops, stmt.target);
        if (
          valid_index == call_dynamic.path.size() - 1 || code != Code::Continue)
        {
          return code;
        }

        Node value = state.read_local(stmt.target);
        for (size_t i = valid_index + 1; i < call_dynamic.path.size(); ++i)
        {
          Node key = unpack_operand(state, call_dynamic.path[i]);
          value = dot(value, key);
          if (value == nullptr)
          {
            logging::Warn() << "Dot operation returned null path operand: "
                            << key->location().view();
            return Code::Undefined;
          }
        }

        state.write_local(stmt.target, value);
        return Code::Continue;
      }

      case b::StatementType::Scan:
        return run_scan(state, stmt);

      case b::StatementType::With:
        return run_with(state, stmt);

      case b::StatementType::Break:
        state.push_break(stmt.op0.index);
        return Code::Break;

      case b::StatementType::Nop:
        break;
    }

    return Code::Continue;
  }

  VirtualMachine::Code VirtualMachine::run_with(
    State& state, const b::Statement& stmt) const
  {
    state.push_with();
    Node value = unpack_operand(state, stmt.op0);
    Node old_value =
      write_and_swap(state, stmt.target, stmt.ext->with().path, value);
    Code result = run_block(state, stmt.ext->with().block);
    state.write_local(stmt.target, old_value);
    state.pop_with();
    return result;
  }

  Node VirtualMachine::dot(const Node& node, const Node& key) const
  {
    auto maybe_source = unwrap(node, {Object, Array, Set});
    if (!maybe_source.success)
    {
      logging::Warn() << "Invalid source type for dot operation: "
                      << node->type().str();
      return nullptr;
    }

    Node source = maybe_source.node;
    if (source == Object)
    {
      std::string query_str = to_key(key);
      for (Node& member : *source)
      {
        std::string member_str = to_key(member / Key);
        if (member_str == query_str)
        {
          return member / Val;
        }
      }

      return nullptr;
    }
    if (source == Set)
    {
      std::string query_str = to_key(key);
      for (Node& member : *source)
      {
        std::string member_str = to_key(member);
        if (member_str == query_str)
        {
          return member;
        }
      }

      return nullptr;
    }
    else if (source == Array)
    {
      auto maybe_index = unwrap(key, {Int, Float});
      if (!maybe_index.success)
      {
        logging::Trace() << "Invalid index for array dot operation: " << key;
        return nullptr;
      }

      try
      {
        std::uint32_t index = to_uint32(maybe_index.node);
        if (index < source->size())
        {
          return source->at(index);
        }

        logging::Trace() << "Index out of bounds for array dot operation: "
                         << index;
        return nullptr;
      }
      catch (const std::runtime_error&)
      {
        // to_uint32 throws std::runtime_error for both invalid literals
        // (e.g. floating-point indices) and out-of-range values
        // (e.g. negative or > UINT32_MAX). Either way the dot operation
        // is undefined.
        logging::Trace() << "Invalid index for array dot operation: "
                         << key->location().view();
        return nullptr;
      }
    }
    else
    {
      logging::Error() << "Invalid source type for dot operation: "
                       << source->type().str();
      throw std::runtime_error("Invalid source type for dot operation");
    }
  }

  VirtualMachine::Code VirtualMachine::run_scan(
    State& state, const b::Statement& stmt) const
  {
    Node source = state.read_local(stmt.target);
    if (source->in({Int, Float, JSONString, True, False, Null}))
    {
      // non-iterable domain
      logging::Warn() << "Non-iterable domain: " << source->type().str();
      return Code::Undefined;
    }

    for (size_t i = 0; i < source->size(); ++i)
    {
      logging::Trace() << "ScanStmt(index=" << i << ")";
      if (source == Object)
      {
        Node item = source->at(i);
        state.write_local(stmt.op0.index, item / Key);
        state.write_local(stmt.op1.index, item / Val);
      }
      else if (source == Array)
      {
        state.write_local(
          stmt.op0.index, Term << (Scalar << (Int ^ std::to_string(i))));
        state.write_local(stmt.op1.index, source->at(i));
      }
      else if (source == Set)
      {
        state.write_local(stmt.op0.index, source->at(i));
        state.write_local(stmt.op1.index, source->at(i));
      }
      else
      {
        logging::Error() << "Invalid source type for scan operation: "
                         << source->type().str();
        throw std::runtime_error("Invalid source type for scan");
      }

      Code code = run_block(state, stmt.ext->block());
      if (code != Code::Continue && code != Code::Undefined)
      {
        return code;
      }
    }

    return Code::Continue;
  }

  Node VirtualMachine::write_and_swap(
    State& state, size_t key, const std::vector<size_t>& path, Node value) const
  {
    Node old_source = state.read_local(key);

    if (path.size() == 0)
    {
      state.write_local(key, value);
      return old_source;
    }

    Node source;
    if (old_source == nullptr || old_source == Undefined)
    {
      source = NodeDef::create(Object);
    }
    else
    {
      source = old_source->clone();
    }

    Node current = source;
    for (size_t i = 0; i < path.size(); ++i)
    {
      const Location& query_str = m_bundle->strings[path[i]];

      if (current != Object)
      {
        Node obj = NodeDef::create(Object);
        current->parent()->replace(current, obj);
        current = obj;
      }

      Node child;
      for (Node& member : *current)
      {
        std::string member_str = strip_quotes(to_key(member / Key));
        if (member_str == query_str.view())
        {
          child = member;
          break;
        }
      }

      if (i == path.size() - 1)
      {
        if (child == nullptr)
        {
          current
            << (ObjectItem << to_term((JSONString ^ query_str))
                           << to_term(value));
        }
        else
        {
          child->replace_at(1, to_term(value));
        }
      }
      else
      {
        if (child == nullptr)
        {
          child = ObjectItem << to_term(JSONString ^ query_str)
                             << to_term(NodeDef::create(Object));
          current << child;
        }

        current = (child / Val)->front();
      }
    }

    state.write_local(key, source);

    return old_source;
  }

  Node VirtualMachine::merge_objects(
    const Node& a, const Node& b, size_t depth) const
  {
    if (depth >= 256)
    {
      return err(
        a, "merge_objects recursion depth exceeded", EvalConflictError);
    }

    auto maybe_lhs = unwrap(a, {Object, Set});
    auto maybe_rhs = unwrap(b, {Object, Set});
    if (!maybe_lhs.success && !maybe_rhs.success)
    {
      return a;
    }

    if (!maybe_lhs.success)
    {
      return err(a, "conflicting values for rule", EvalConflictError);
    }

    if (!maybe_rhs.success)
    {
      return err(b, "conflicting values for rule", EvalConflictError);
    }

    Node lhs_obj = maybe_lhs.node;
    Node rhs_obj = maybe_rhs.node;

    if (lhs_obj == Set && rhs_obj == Set)
    {
      return merge_sets(lhs_obj, rhs_obj);
    }

    if (lhs_obj != Object || rhs_obj != Object)
    {
      return err(b, "conflicting values for rule", EvalConflictError);
    }

    std::map<std::string, Node> items;
    for (auto& item : *lhs_obj)
    {
      items[to_key(item / Key)] = item->clone();
    }

    for (auto& item : *rhs_obj)
    {
      std::string key = to_key(item / Key);
      if (items.contains(key))
      {
        Node merged = merge_objects(items[key] / Val, item / Val, depth + 1);
        if (merged == Error)
        {
          return merged;
        }

        items[key] = ObjectItem << (item / Key)->clone() << merged;
      }
      else
      {
        items[key] = item->clone();
      }
    }

    auto object = NodeDef::create(Object);
    for (auto& [_, item] : items)
    {
      object->push_back(item);
    }

    return Term << object;
  }

  Node VirtualMachine::merge_sets(const Node& a, const Node& b) const
  {
    Node lhs_set = a;
    Node rhs_set = b;

    Node set = NodeDef::create(Set);
    std::set<std::string> items;
    for (auto& item : *lhs_set)
    {
      items.insert(to_key(item));
      set << item;
    }

    for (auto& item : *rhs_set)
    {
      if (!items.contains(to_key(item)))
      {
        set << item;
      }
    }

    return Term << set;
  }

  bool VirtualMachine::insert_into_object(
    const Node& object, const Node& key, const Node& value, bool once) const
  {
    if (object != Object)
    {
      logging::Error() << "attempting to insert into a non-object value: "
                       << object;
      return true;
    }

    Node existing;
    std::string insert_key = to_key(key);
    for (Node item : *object)
    {
      std::string existing_key = to_key(item / Key);
      if (insert_key == existing_key)
      {
        existing = item;
        break;
      }
    }

    if (existing)
    {
      if (once)
      {
        std::string existing_key = to_key(existing / Val);
        std::string new_key = to_key(value);
        if (existing_key != new_key)
        {
          logging::Error()
            << "key " << insert_key
            << " already exists but values do not match: existing="
            << existing_key << " != new=" << new_key;
          return true;
        }
      }

      existing / Val = to_term(value);
      return false;
    }

    object << (ObjectItem << to_term(key) << to_term(value));
    return false;
  }

  Node VirtualMachine::to_term(const Node& value) const
  {
    if (value == Error)
    {
      return value;
    }

    if (value->in({Term}))
    {
      return value->clone();
    }

    if (value->in({Array, Set, Object, Scalar}))
    {
      return Term << value->clone();
    }

    if (value->in({Int, Float, JSONString, True, False, Null}))
    {
      return Term << (Scalar << value->clone());
    }

    return err(value, "Not a term");
  }

  size_t VirtualMachine::stmt_limit() const
  {
    return m_stmt_limit;
  }

  VirtualMachine& VirtualMachine::stmt_limit(size_t stmt_limit)
  {
    m_stmt_limit = stmt_limit;
    return *this;
  }

  size_t VirtualMachine::max_call_depth() const
  {
    return m_max_call_depth;
  }

  VirtualMachine& VirtualMachine::max_call_depth(size_t depth)
  {
    m_max_call_depth = depth;
    return *this;
  }

  size_t VirtualMachine::max_block_depth() const
  {
    return m_max_block_depth;
  }

  VirtualMachine& VirtualMachine::max_block_depth(size_t depth)
  {
    m_max_block_depth = depth;
    return *this;
  }
}