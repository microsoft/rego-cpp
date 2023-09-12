#include "internal.hh"

namespace rego
{
  using namespace wf::ops;

  UnifierDef::UnifierDef(
    const Location& rule,
    const Node& rulebody,
    CallStack call_stack,
    WithStack with_stack,
    const BuiltIns& builtins,
    UnifierCache cache) :
    m_rule(rule),
    m_call_stack(call_stack),
    m_with_stack(with_stack),
    m_builtins(builtins),
    m_parent_type(rulebody->parent()->type()),
    m_cache(cache),
    m_negate(false)
  {
    LOG_HEADER("ASSEMBLING UNIFICATION", "---");
    m_dependency_graph.push_back({"start", {}, 0});
    init_from_body(rulebody, m_statements, 0);
    compute_dependency_scores();
    m_retries = detect_cycles();
    if (m_retries > 0)
    {
      LOG("Detected ", m_retries, " cycles in dependency graph");
    }

    LOG("Dependency graph:");
    LOG_INDENT();
    for (auto& dep : m_dependency_graph)
    {
      std::ostringstream os;
      os << "[" << dep.name << "](" << dep.score << ") -> {";
      std::string sep = "";
      for (auto id : dep.dependencies)
      {
        os << sep << m_dependency_graph[id].name;
        sep = ", ";
      }
      os << "}";
      LOG(os.str());
    }
    LOG_UNINDENT();
    LOG_HEADER("ASSEMBLY COMPLETE", "---");
  }

  std::size_t UnifierDef::detect_cycles() const
  {
    std::size_t cycles = 0;
    for (std::size_t id = 0; id < m_dependency_graph.size(); ++id)
    {
      if (has_cycle(id))
      {
        ++cycles;
      }
    }

    return cycles;
  }

  bool UnifierDef::has_cycle(std::size_t id) const
  {
    std::set<std::size_t> visited;

    auto deps = m_dependency_graph[id].dependencies;
    std::vector<std::size_t> stack(deps.begin(), deps.end());
    while (!stack.empty())
    {
      auto current = stack.back();
      stack.pop_back();

      if (current == id)
      {
        return true;
      }

      if (visited.contains(current))
      {
        continue;
      }

      visited.insert(current);

      for (const auto& dep : m_dependency_graph[current].dependencies)
      {
        stack.push_back(dep);
      }
    }

    return false;
  }

  void UnifierDef::init_from_body(
    const Node& rulebody, std::vector<Node>& statements, std::size_t root)
  {
    std::for_each(rulebody->begin(), rulebody->end(), [&](const auto& stmt) {
      if (stmt->type() == Local)
      {
        std::size_t id = add_variable(stmt);
        m_dependency_graph[id].dependencies.insert(root);
      }
      else if (stmt->type() == UnifyExpr)
      {
        statements.push_back(stmt);
        std::size_t id = add_unifyexpr(stmt);
        m_dependency_graph[id].dependencies.insert(root);
      }
      else if (stmt->type() == UnifyExprWith)
      {
        statements.push_back(stmt);
        std::size_t id = m_dependency_graph.size();
        std::string name = "with" + std::to_string(id);
        m_dependency_graph.push_back({name, {root}, 0});
        m_expr_ids[stmt] = id;
        m_nested_statements[stmt] = {};
        init_from_body(stmt / UnifyBody, m_nested_statements[stmt], id);

        Node withseq = stmt / WithSeq;
        std::vector<std::size_t> dep_ids;
        for (auto& with : *withseq)
        {
          std::vector<Location> deps;
          scan_vars(with / Var, deps);
          std::transform(
            deps.begin(),
            deps.end(),
            std::back_inserter(dep_ids),
            [this](auto& dep) { return m_variables.at(dep).id(); });
        }

        m_dependency_graph[id].dependencies.insert(
          dep_ids.begin(), dep_ids.end());
      }
      else if (stmt->type() == UnifyExprNot)
      {
        statements.push_back(stmt);
        std::size_t id = m_dependency_graph.size();
        std::string name = "not" + std::to_string(id);
        m_dependency_graph.push_back({name, {root}, 0});
        m_expr_ids[stmt] = id;
        m_nested_statements[stmt] = {};
        init_from_body(stmt->front(), m_nested_statements[stmt], root);
        for (auto& nested : m_nested_statements[stmt])
        {
          m_dependency_graph[id].dependencies.insert(m_expr_ids[nested]);
        }
      }
    });
  }

  std::size_t UnifierDef::add_variable(const Node& local)
  {
    auto name = (local / Var)->location();
    std::size_t id = m_dependency_graph.size();
    m_variables.insert({name, Variable(local, id)});
    m_dependency_graph.push_back({std::string(name.view()), {}, 0});
    return id;
  }

  std::size_t UnifierDef::add_unifyexpr(const Node& unifyexpr)
  {
    Node lhs = unifyexpr / Var;
    Node rhs = unifyexpr / Val;
    if (!is_local(lhs))
    {
      std::string name = std::string(lhs->location().view());
      throw std::runtime_error(
        "Unification target " + name + " is not a local variable");
    }

    Variable& var = get_variable(lhs->location());
    std::vector<Location> deps;
    scan_vars(rhs, deps);
    std::vector<std::size_t> dep_ids;
    std::transform(
      deps.begin(), deps.end(), std::back_inserter(dep_ids), [this](auto& dep) {
        return m_variables.at(dep).id();
      });
    std::size_t expr_id = m_dependency_graph.size();
    m_expr_ids[unifyexpr] = expr_id;
    std::string name = Resolver::expr_str(unifyexpr).str();
    m_dependency_graph.push_back(
      {name, std::set(dep_ids.begin(), dep_ids.end()), 0});
    m_dependency_graph[var.id()].dependencies.insert(expr_id);
    return expr_id;
  }

  std::size_t UnifierDef::compute_dependency_score(
    std::size_t id, std::set<std::size_t>& visited)
  {
    if (visited.contains(id))
    {
      return m_dependency_graph[id].score;
    }

    visited.insert(id);
    std::size_t score = 1;
    for (auto dep_id : m_dependency_graph[id].dependencies)
    {
      score += compute_dependency_score(dep_id, visited);
    }

    m_dependency_graph[id].score = score;
    return score;
  }

  void UnifierDef::compute_dependency_scores()
  {
    std::set<std::size_t> visited;
    for (std::size_t id = 0; id < m_dependency_graph.size(); ++id)
    {
      compute_dependency_score(id, visited);
    }

    std::sort(
      m_statements.begin(), m_statements.end(), [this](auto& lhs, auto& rhs) {
        return dependency_score(lhs) < dependency_score(rhs);
      });

    for (auto& [_, nested_statements] : m_nested_statements)
    {
      std::sort(
        nested_statements.begin(),
        nested_statements.end(),
        [this](auto& lhs, auto& rhs) {
          return dependency_score(lhs) < dependency_score(rhs);
        });
    }
  }

  std::size_t UnifierDef::dependency_score(const Node& expr) const
  {
    return m_dependency_graph[m_expr_ids.at(expr)].score;
  }

  std::size_t UnifierDef::dependency_score(const Variable& var) const
  {
    return m_dependency_graph[var.id()].score;
  }

  bool UnifierDef::is_local(const Node& var)
  {
    return is_variable(var->location());
  }

  std::size_t UnifierDef::scan_vars(
    const Node& expr, std::vector<Location>& locals)
  {
    std::size_t num_vars = 0;
    std::vector<Node> stack;
    stack.push_back(expr);
    while (stack.size() > 0)
    {
      Node current = stack.back();
      stack.pop_back();
      if (current->type() == Var)
      {
        if (is_local(current))
        {
          locals.push_back(current->location());
        }

        num_vars++;
      }
      else
      {
        stack.insert(stack.end(), current->begin(), current->end());
      }
    }

    return num_vars;
  }

  void UnifierDef::execute_statements(
    Nodes::iterator begin, Nodes::iterator end)
  {
    for (auto it = begin; it != end; ++it)
    {
      Node stmt = *it;
      if (stmt->type() == UnifyExpr)
      {
        LOG(Resolver::expr_str(stmt));
        Node lhs = stmt / Var;
        Variable& var = get_variable(lhs->location());
        Values values = evaluate(lhs->location(), stmt / Val);
        if (values.size() == 0)
        {
          if (m_negate && var.is_unify())
          {
            var.unify({ValueDef::create(True ^ "true")});
            LOG("> result: ", var);
          }
          else
          {
            continue;
          }
        }
        else
        {
          if (m_negate && var.is_unify())
          {
            bool all_false = true;
            for (auto& value : values)
            {
              if (!is_falsy(value->node()))
              {
                all_false = false;
                break;
              }
            }
            if (all_false)
            {
              for (auto& value : values)
              {
                value->mark_as_valid();
              }
              var.unify({ValueDef::create(True ^ "true")});
              LOG("> result: ", var);
            }
            else
            {
              continue;
            }
          }
          else
          {
            var.unify(values);
            LOG("> result: ", var);
          }
        }
      }
      else if (stmt->type() == UnifyExprNot)
      {
        LOG(Resolver::not_str(stmt));
        push_not();
        LOG_INDENT();
        auto nested = m_nested_statements[stmt];
        execute_statements(nested.begin(), nested.end());
        LOG_UNINDENT();
        pop_not();
      }
      else if (stmt->type() == UnifyExprWith)
      {
        LOG(Resolver::with_str(stmt));
        push_with(stmt / WithSeq);
        LOG_INDENT();
        auto nested = m_nested_statements[stmt];
        execute_statements(nested.begin(), nested.end());
        LOG_UNINDENT();
        pop_with();
      }
    }
  }

  void UnifierDef::reset()
  {
    for (auto& [_, var] : m_variables)
    {
      var.reset();
    }
  }

  void UnifierDef::pass()
  {
    LOG_MAP_VALUES(m_variables);
    execute_statements(m_statements.begin(), m_statements.end());
  }

  void UnifierDef::remove_invalid_values()
  {
    std::for_each(m_variables.begin(), m_variables.end(), [](auto& pair) {
      pair.second.remove_invalid_values();
    });
  }

  Node UnifierDef::bind_variables()
  {
    LOG("bind and check variables:");
    return std::accumulate(
      m_variables.begin(),
      m_variables.end(),
      Undefined ^ "undefined",
      [](Node result, auto& pair) {
        Variable& var = pair.second;
        if (!var.is_unify() && !var.is_user_var())
        {
          return result;
        }

        Node node = var.bind();

        if (node->type() == Error)
        {
          LOG("> ", var.name().view(), ": Error");
          return node;
        }

        if (node->type() == TermSet)
        {
          if (var.is_unify())
          {
            if (node->size() == 0)
            {
              LOG("> ", var.name().view(), ": Empty TermSet => false");
              return False ^ "false";
            }
            Node maybe_term = Resolver::reduce_termset(node);
            if (maybe_term->type() == Term)
            {
              if (result->type() == Undefined)
              {
                LOG("> ", var.name().view(), ": TermSet => true");
                return True ^ "true";
              }
            }
            else
            {
              LOG("> ", var.name().view(), ": TermSet => Error");
              return err(
                node,
                "complete rules must not produce multiple outputs",
                EvalConflictError);
            }
          }
          else if (node->size() > 0 && result->type() == Undefined)
          {
            LOG("> ", var.name().view(), ": TermSet => true");
            return True ^ "true";
          }
          else if (node->size() == 0 && var.is_user_var())
          {
            LOG("> ", var.name().view(), ": Empty TermSet => false");
            return False ^ "false";
          }
        }
        else if (var.is_unify() && is_falsy(node))
        {
          LOG("> ", var.name().view(), ": false => false");
          return False ^ "false";
        }
        else if (node->type() == Term && result->type() == Undefined)
        {
          LOG("> ", var.name().view(), ": Term => true");
          return True ^ "true";
        }
        else if (var.is_user_var() && is_undefined(node))
        {
          LOG("> ", var.name().view(), ": undefined => false");
          return False ^ "false";
        }

        LOG("> ", var.name().view(), ": ", result->location().view());

        return result;
      });
  }

  Node UnifierDef::unify()
  {
    if (this->push_rule(this->m_rule))
    {
      throw std::runtime_error(
        "Recursion detected in rule body: " + std::string(m_rule.view()));
    }

    LOG_HEADER("Unification", "=====");

    LOG("exprs: ");
    LOG_VECTOR_CUSTOM(m_statements, Resolver::stmt_str);

    LOG_INDENT();

    for (std::size_t pass_index = 0; pass_index < m_retries + 1; ++pass_index)
    {
      LOG_HEADER("Pass " + std::to_string(pass_index), "=====");
      pass();
      mark_invalid_values();
      remove_invalid_values();
    }

    LOG_MAP_VALUES(m_variables);

    LOG_UNINDENT();
    Node result = bind_variables();
    LOG_HEADER("Complete", "=====");

    this->pop_rule(this->m_rule);
    return result;
  }

  void UnifierDef::mark_invalid_values()
  {
    for (auto& [_, var] : m_variables)
    {
      if (!var.is_unify())
      {
        // only unification statements can mark values as invalid
        continue;
      }

      var.mark_invalid_values();
    }
  }

  Args UnifierDef::create_args(const Node& args)
  {
    Args function_args;
    for (auto arg : *args)
    {
      Values arg_values;
      if (arg->type() == Var)
      {
        arg_values = resolve_var(arg);
      }
      else
      {
        arg_values.push_back(ValueDef::create(arg));
      }

      if (arg_values.size() == 0)
      {
        arg_values.push_back(ValueDef::create(Undefined));
      }

      function_args.push_back_source(arg_values);
    }

    return function_args;
  }

  Values UnifierDef::evaluate_function(
    const Location& var, const std::string& func_name, const Args& args)
  {
    Values values;
    if (args.size() == 0)
    {
      LOG("> calling ", func_name, " with no args");
      if (func_name == "array")
      {
        values.push_back(ValueDef::create(NodeDef::create(Array)));
      }
      else if (func_name == "object")
      {
        values.push_back(ValueDef::create(NodeDef::create(Object)));
      }
      else if (func_name == "set")
      {
        values.push_back(ValueDef::create(NodeDef::create(Set)));
      }
      return values;
    }

    LOG("> calling ", func_name, " with ", args);
    std::set<Value> valid_args;
    if (func_name == "apply_access")
    {
      for (std::size_t i = 0; i < args.size(); ++i)
      {
        Values call_args = args.at(i);
        Values results = apply_access(var, call_args);
        for (auto& result : results)
        {
          LOG("> result: ", result);
          values.push_back(result);
        }

        if (results.size() > 0)
        {
          // these arguments resulted in valid values
          valid_args.insert(call_args.begin(), call_args.end());
        }
        else if (m_negate)
        {
          for (auto arg : call_args)
          {
            arg->mark_as_valid();
          }
        }
      }
    }
    else if (func_name == "call")
    {
      Values funcs = args.source_at(0);
      Args func_args = args.subargs(1);
      if (func_args.source_size() == 0)
      {
        Node peek = funcs.front()->node();
        if (peek->type() != RuleFunc && peek->type() != BuiltInHook)
        {
          Node term = funcs.front()->to_term();
          // likely the result of the function being replaced by with
          values.push_back(funcs.front());
        }
        else
        {
          auto maybe_result = call_function(var, funcs);
          if (maybe_result.has_value())
          {
            Value result = maybe_result.value();
            LOG("> result: ", result);
            values.push_back(result);
          }
        }
      }

      for (std::size_t i = 0; i < func_args.size(); ++i)
      {
        Value result = nullptr;
        Values arglist = func_args.at(i);
        for (Value func : funcs)
        {
          if (
            func->node()->type() != RuleFunc &&
            func->node()->type() != BuiltInHook)
          {
            // likely the result of the function being replaced by with
            // even though we won't call the function, we still have to check
            // that the arguments were valid, i.e. not undefined.
            LOG("func value ", func, " is not a function.");
            LOG("checking that args are valid");
            bool all_valid = true;
            for (auto& arg : arglist)
            {
              if (is_undefined(arg->node()))
              {
                LOG("Undefined arg -> invalid");
                all_valid = false;
                break;
              }
            }
            if (all_valid)
            {
              LOG("all args valid");
              result = func;
            }
            break;
          }

          Values call_args = {func};
          call_args.insert(call_args.end(), arglist.begin(), arglist.end());
          auto maybe_result = call_function(var, call_args);
          if (maybe_result.has_value())
          {
            if (result == nullptr)
            {
              result = maybe_result.value();
              result->reduce_set();
              valid_args.insert(call_args.begin(), call_args.end());
              if (result->node()->type() == TermSet)
              {
                result = ValueDef::create(
                  var,
                  err(
                    result->node(),
                    "functions must not produce multiple outputs for same "
                    "inputs",
                    EvalConflictError));
                LOG("> result: Termset -> Error");
                break;
              }
              else
              {
                LOG("> result: ", result, "#", result->rank());
              }
            }
            else
            {
              Value old_result = result;
              Value new_result = maybe_result.value();
              if (new_result->rank() < old_result->rank())
              {
                result = new_result;
                LOG("> result: ", result, "#", result->rank());
              }
              else if (new_result->rank() == old_result->rank())
              {
                std::string old_str = to_json(result->to_term());
                std::string new_str = to_json(maybe_result.value()->to_term());
                if (old_str == "undefined" && new_str != "undefined")
                {
                  result = maybe_result.value();
                  LOG("> result: ", result, "#", result->rank());
                }
                else if (new_str == "undefined")
                {
                  continue;
                }
                else if (old_str != new_str)
                {
                  result = ValueDef::create(
                    var,
                    err(
                      func->node(),
                      "functions must not produce multiple outputs for same "
                      "inputs",
                      EvalConflictError));
                  LOG("> second result => Error");
                  break;
                }
              }
            }
          }
          else if (m_negate)
          {
            for (auto arg : call_args)
            {
              arg->mark_as_valid();
            }
          }
        }

        if (result != nullptr)
        {
          values.push_back(result);
          if (result->node()->type() == Error)
          {
            break;
          }
        }
      }
    }
    else
    {
      for (std::size_t i = 0; i < args.size(); ++i)
      {
        Values call_args = args.at(i);
        auto maybe_result = call_named_function(var, func_name, call_args);
        if (maybe_result.has_value())
        {
          Value result = maybe_result.value();
          LOG("> result: ", result);
          values.push_back(result);
          valid_args.insert(call_args.begin(), call_args.end());
        }
        else if (m_negate)
        {
          for (auto arg : call_args)
          {
            arg->mark_as_valid();
          }
        }
      }
    }

    if (m_negate)
    {
      args.mark_invalid(valid_args);
    }
    else
    {
      args.mark_invalid_except(valid_args);
    }

    return values;
  }

  Values UnifierDef::evaluate(const Location& var, const Node& value)
  {
    Values values;
    if (value->type() == Var)
    {
      Values source_values = resolve_var(value);
      std::transform(
        source_values.begin(),
        source_values.end(),
        std::back_inserter(values),
        [var](auto& source_value) {
          return ValueDef::copy_to(source_value, var);
        });
    }
    else if (value->type() == Scalar)
    {
      values.push_back(ValueDef::create(var, value));
    }
    else if (value->type() == Function)
    {
      std::string func_name =
        std::string((value / JSONString)->location().view());
      Node args_node = value / ArgSeq;
      if (func_name == "enumerate")
      {
        Values arg_values = enumerate(var, args_node->front());
        values.insert(values.end(), arg_values.begin(), arg_values.end());
      }
      else if (func_name == "merge")
      {
        Values partials = resolve_var(args_node->front());
        if (partials.size() > 0)
        {
          Node merged = NodeDef::create(partials.front()->node()->type());
          for (auto partial : partials)
          {
            Node partial_node = partial->node();
            merged->insert(
              merged->end(), partial_node->begin(), partial_node->end());
          }
          values.push_back(ValueDef::create(var, merged, partials));
        }
      }
      else if (func_name == "array-compr")
      {
        Values termsets = resolve_var(args_node->front());
        Node argseq = NodeDef::create(ArgSeq);
        for (auto termset_value : termsets)
        {
          LOG("flattening ", Resolver::arg_str(termset_value->node()));
          Resolver::flatten_terms_into(termset_value->node(), argseq);
        }
        values.push_back(ValueDef::create(var, Resolver::array(argseq)));
      }
      else if (func_name == "set-compr")
      {
        Values termsets = resolve_var(args_node->front());
        Node argseq = NodeDef::create(ArgSeq);
        for (auto termset_value : termsets)
        {
          Resolver::flatten_terms_into(termset_value->node(), argseq);
        }
        values.push_back(ValueDef::create(var, Resolver::set(argseq)));
      }
      else if (func_name == "object-compr")
      {
        Values termsets = resolve_var(args_node->front());
        Node argseq = NodeDef::create(ArgSeq);
        for (auto termset_value : termsets)
        {
          LOG("flattening ", Resolver::arg_str(termset_value->node()));
          Resolver::flatten_items_into(termset_value->node(), argseq);
        }
        values.push_back(
          ValueDef::create(var, Resolver::object(argseq, false)));
      }
      else
      {
        Values results =
          evaluate_function(var, func_name, create_args(args_node));
        values.insert(values.end(), results.begin(), results.end());
      }
    }

    return values;
  }

  Values UnifierDef::resolve_var(const Node& node, bool exclude_with)
  {
    Values values;
    if (!exclude_with)
    {
      values = check_with(node);
      if (!values.empty())
      {
        return values;
      }
    }

    if (is_variable(node->location()))
    {
      // part of the unification
      Variable& var = get_variable(node->location());
      Values valid_values = var.valid_values();
      values.insert(values.end(), valid_values.begin(), valid_values.end());
      return values;
    }

    Nodes defs = node->lookup();

    if (defs.empty())
    {
      return values;
    }

    Token peek_type = defs.front()->type();
    if (
      peek_type == RuleSet || peek_type == RuleObj || peek_type == RuleComp ||
      peek_type == Submodule)
    {
      auto maybe_node = resolve_rule(defs);
      if (maybe_node)
      {
        values.push_back(ValueDef::create(*maybe_node));
      }
    }
    else
    {
      for (const auto& def : defs)
      {
        if (def->type() == Local)
        {
          // a resolved local from another problem in the same rule
          // i.e. referring to a body local from the value.
          values.push_back(ValueDef::create(def / Val));
        }
        else if (def->type() == ArgVar)
        {
          values.push_back(ValueDef::create(def / Val));
        }
        else if (def->type() == Skip)
        {
          Values skip_values = resolve_skip(def);
          values.insert(values.end(), skip_values.begin(), skip_values.end());
        }
        else if (def->type() == Input)
        {
          Node term = (def / Val)->clone();
          values.push_back(ValueDef::create(term));
        }
        else if (def->type() == Data)
        {
          values.push_back(ValueDef::create(resolve_module(def)));
        }
        else if (def->type() == RuleFunc)
        {
          // these will always be an argument to apply_access
          values.push_back(ValueDef::create(def));
        }
        else if (def->type() == DataItem)
        {
          Node value = def / Val;
          if (value->type() == DataModule)
          {
            values.push_back(ValueDef::create(resolve_module(def)));
          }
          else
          {
            values.push_back(ValueDef::create(value));
          }
        }
        else
        {
          values.push_back(
            ValueDef::create(err(def, "Unsupported definition type")));
        }
      }
    }

    return values;
  }

  Values UnifierDef::apply_access(const Location& var, const Values& args)
  {
    Values values;
    Values sources;
    std::copy_if(
      args.begin(), args.end(), std::back_inserter(sources), [this](auto& arg) {
        return is_variable(arg->var());
      });

    Node container = args[0]->node();

    if (container->type() == Error)
    {
      values.push_back(ValueDef::create(var, container, sources));
      return values;
    }

    if (container->type() == Term)
    {
      container = container->front();
    }

    if (container->type() == Undefined)
    {
      values.push_back(ValueDef::create(var, container, sources));
    }

    else
    {
      auto maybe_nodes = Resolver::apply_access(container, args[1]->node());
      if (maybe_nodes)
      {
        Nodes defs = maybe_nodes.value();
        if (!defs.empty())
        {
          Token peek_type = defs.front()->type();
          if (
            peek_type == RuleSet || peek_type == RuleObj ||
            peek_type == RuleComp)
          {
            auto maybe_node = resolve_rule(defs);
            if (maybe_node)
            {
              values.push_back(ValueDef::create(var, *maybe_node, sources));
            }
          }
          else
          {
            std::transform(
              defs.begin(),
              defs.end(),
              std::back_inserter(values),
              [var, sources](auto& def) {
                return ValueDef::create(var, def, sources);
              });
          }
        }
      }
    }

    return values;
  }

  std::optional<Value> UnifierDef::call_named_function(
    const Location& var, const std::string& func_name, const Values& args)
  {
    Values sources;
    std::copy_if(
      args.begin(), args.end(), std::back_inserter(sources), [this](auto& arg) {
        return is_variable(arg->var());
      });

    if (func_name == "arithinfix")
    {
      Node result =
        Resolver::arithinfix(args[0]->node(), args[1]->node(), args[2]->node());
      return ValueDef::create(var, result, sources);
    }

    if (func_name == "bininfix")
    {
      Node result =
        Resolver::bininfix(args[0]->node(), args[1]->node(), args[2]->node());
      return ValueDef::create(var, result, sources);
    }

    if (func_name == "boolinfix")
    {
      Node result =
        Resolver::boolinfix(args[0]->node(), args[1]->node(), args[2]->node());
      return ValueDef::create(var, result, sources);
    }

    if (func_name == "unary")
    {
      return ValueDef::create(var, Resolver::unary(args[0]->node()), sources);
    }

    if (func_name == "not")
    {
      Node term = args[0]->to_term();
      if (is_truthy(term))
      {
        return ValueDef::create(var, False ^ "false", sources);
      }
      else
      {
        return ValueDef::create(var, True ^ "true", sources);
      }
    }

    if (func_name == "membership-tuple")
    {
      Node result =
        Resolver::membership(args[0]->node(), args[1]->node(), args[2]->node());
      return ValueDef::create(var, result, sources);
    }

    if (func_name == "membership-single")
    {
      Node result = Resolver::membership(args[0]->node(), args[1]->node());
      return ValueDef::create(var, result, sources);
    }

    if (func_name == "object")
    {
      Node object_items = NodeDef::create(ArgSeq);
      for (auto& arg : args)
      {
        if (arg->node()->type() == Undefined)
        {
          return std::nullopt;
        }

        object_items->push_back(arg->node());
      }
      return ValueDef::create(
        var, Resolver::object(object_items, false), sources);
    }

    if (func_name == "array")
    {
      Node array_members = NodeDef::create(ArgSeq);
      for (auto& arg : args)
      {
        if (arg->node()->type() == Undefined)
        {
          return std::nullopt;
        }

        array_members->push_back(arg->node());
      }
      return ValueDef::create(var, Resolver::array(array_members), sources);
    }

    if (func_name == "set")
    {
      Node set_members = NodeDef::create(ArgSeq);
      for (auto& arg : args)
      {
        if (arg->node()->type() == Undefined)
        {
          return std::nullopt;
        }

        set_members->push_back(arg->node());
      }
      return ValueDef::create(var, Resolver::set(set_members), sources);
    }

    return std::nullopt;
  }

  std::optional<Value> UnifierDef::call_function(
    const Location& var, const Values& args) const
  {
    Values sources;
    std::copy_if(
      args.begin(), args.end(), std::back_inserter(sources), [this](auto& arg) {
        return is_variable(arg->var());
      });

    Node function = args[0]->node();
    Nodes function_args;
    std::transform(
      args.begin() + 1,
      args.end(),
      std::back_inserter(function_args),
      [](auto& arg) { return arg->node(); });

    if (m_builtins.is_builtin(function->location()))
    {
      Node node = m_builtins.call(function->location(), function_args);
      return ValueDef::create(var, node, sources);
    }

    auto maybe_node = resolve_rulefunc(function, function_args);
    if (maybe_node)
    {
      return ValueDef::create(var, maybe_node.value(), sources);
    }

    return std::nullopt;
  }

  Values UnifierDef::enumerate(const Location& var, const Node& container_var)
  {
    Values items;
    Values container_values = resolve_var(container_var);
    for (auto& value : container_values)
    {
      if (value->node()->type() == Error)
      {
        items.push_back(value);
        return items;
      }
    }
    LOG_VECTOR(container_values);
    for (auto& container_value : container_values)
    {
      Node container = container_value->node();

      if (container->type() == Term)
      {
        container = container->front();
      }

      if (container->type() == Array)
      {
        for (std::size_t i = 0; i < container->size(); ++i)
        {
          Node index = Scalar << (Int ^ std::to_string(i));
          Node tuple = Term << (Array << index << container->at(i));
          items.push_back(ValueDef::create(var, tuple));
        }
      }

      if (container->type() == Object)
      {
        for (const Node& object_item : *container)
        {
          Node tuple = Term
            << (Array << object_item / Key << object_item / Val);
          items.push_back(ValueDef::create(var, tuple));
        }
      }

      if (container->type() == Set)
      {
        for (const Node& value : *container)
        {
          Node tuple = Term << (Array << value << value);
          items.push_back(ValueDef::create(var, tuple));
        }
      }
    }

    for (auto& item : items)
    {
      item->mark_as_valid();
    }

    return items;
  }

  std::string UnifierDef::str() const
  {
    std::ostringstream buf;
    buf << this;
    return buf.str();
  }

  bool UnifierDef::push_rule(const Location& rule)
  {
    LOG("Pushing rule: ", rule.view());
    LOG("Call stack: ", *m_call_stack);

    if (
      std::find(m_call_stack->begin(), m_call_stack->end(), rule) !=
      m_call_stack->end())
    {
      return true;
    }

    m_call_stack->push_back(rule);
    return false;
  }

  void UnifierDef::pop_rule(const Location& rule)
  {
    if (m_call_stack->size() == 0)
    {
      return;
    }

    if (m_call_stack->back() != rule)
    {
      return;
    }

    LOG("Popping rule: ", m_call_stack->back().view());
    LOG("Call stack: ", *m_call_stack);
    m_call_stack->pop_back();
  }

  bool UnifierDef::would_recurse(const Node& node)
  {
    if (node->type() == Function)
    {
      std::string func_name = strip_quotes(get_string(node / JSONString));
      if (func_name != "call")
      {
        return false;
      }

      Node func = (node / ArgSeq)->front();
      Values values = check_with(func, true);
      for (auto value : values)
      {
        Node maybe_func = value->node();
        if (maybe_func->type() == RuleFunc)
        {
          Location name = (maybe_func / Var)->location();
          if (
            std::find(m_call_stack->begin(), m_call_stack->end(), name) !=
            m_call_stack->end())
          {
            LOG(
              func->location().view(),
              " is replaced by ",
              name.view(),
              " which would recurse");
            return true;
          }
        }
      }
    }
    else
    {
      for (auto& child : *node)
      {
        if (would_recurse(child))
        {
          return true;
        }
      }
    }

    return false;
  }

  Values UnifierDef::check_with(const Node& var, bool bypass_recurse_check)
  {
    std::string key_str = std::string(var->location().view());

    Values results;
    std::map<std::string, Values> partials;
    for (auto it = m_with_stack->rbegin(); it != m_with_stack->rend(); ++it)
    {
      for (auto& [key, val] : *it)
      {
        if (key == key_str)
        {
          if (results.size() == 0)
          {
            LOG("Found key: ", key_str, " in with stack");
            results = val;
            break;
          }
          else
          {
            LOG("Already overridden with higher precedence.");
          }
        }
        else if (key.starts_with(key_str) && !partials.contains(key))
        {
          LOG("Found prefix match: ", key_str, " in with stack");
          partials[key.substr(key_str.size() + 1)] = val;
        }
      }
    }

    if (!results.empty())
    {
      for (auto& result : results)
      {
        Node node = result->node();
        if (node->type() == RuleFunc && !bypass_recurse_check)
        {
          // check whether anything in the result would cause recursion
          if (would_recurse(node / Body) || would_recurse(node / Val))
          {
            LOG("Recursion detected in with result");
            return {};
          }
        }
      }
    }

    if (partials.empty())
    {
      return results;
    }

    if (results.empty())
    {
      // as there is no base object from the with stack
      // we need to procure the original.
      results = resolve_var(var, true);
    }

    Node object;
    if (results.size() > 0)
    {
      object = results.front()->node()->clone();
      LOG("Found base object for modification: ", Resolver::term_str(object));
      if (object->type() == Term)
      {
        object = object->front();
      }

      if (object->type() != Object)
      {
        LOG("Replacing with object (cannot merge partials)");
        object = NodeDef::create(Object);
      }
    }
    else
    {
      object = NodeDef::create(Object);
    }

    for (auto& [key, val] : partials)
    {
      if (val.size() == 0)
      {
        continue;
      }

      LOG("Inserting ", key);
      Resolver::insert_into_object(object, key, val.front()->node());
      LOG("> result: ", Resolver::term_str(object));
    }

    return {ValueDef::create(object)};
  }

  Values UnifierDef::resolve_skip(const Node& skip)
  {
    assert(skip->type() == Skip);
    LOG("Resolving skip: ", Resolver::term_str(skip->front()));
    Values values = check_with(skip->front());
    if (!values.empty())
    {
      return values;
    }

    // not overwritten by with.
    Node ref = skip->back();

    if (ref->type() == Undefined)
    {
      return values;
    }
    else if (ref->type() == BuiltInHook)
    {
      values.push_back(ValueDef::create(ref));
    }
    else
    {
      Node value;
      for (auto& var : *ref)
      {
        if (value == nullptr)
        {
          value = var->lookup()[0];
        }
        else
        {
          std::string key_str = std::string(var->location().view());
          value = value->lookdown(key_str)[0] / Val;
        }
      }
      values.push_back(ValueDef::create(value->clone()));
    }
    return values;
  }

  Node UnifierDef::resolve_module(const Node& dataitem) const
  {
    Node module = dataitem / Val;
    assert(module->type() == DataModule);

    Location prefix = (dataitem / Key)->location();
    std::size_t prefix_len = prefix.len;

    Node object = NodeDef::create(Object);
    std::map<Location, Nodes> rule_nodes;
    for (auto& rule : *module)
    {
      if (rule->type() == RuleFunc)
      {
        Node args = rule / RuleArgs;
        if (args->size() > 0)
        {
          // no way to include this without arguments
          continue;
        }
      }

      Location name;
      if (rule->type() == Submodule)
      {
        name = (rule / Key)->location();
      }
      else
      {
        name = (rule / Var)->location();
      }

      if (name.view().find("$") != std::string::npos)
      {
        continue;
      }

      auto pos = prefix_len;
      auto len = name.view().size() - pos;
      name.pos = pos;
      name.len = len;

      if (name.view()[0] == '.')
      {
        name.pos += 1;
        name.len -= 1;
      }

      if (name.view()[0] == '[')
      {
        name.pos += 2;
        name.len -= 4;
      }

      if (!rule_nodes.contains(name))
      {
        LOG("adding rule ", name.view());
        rule_nodes[name] = Nodes();
      }

      rule_nodes[name].push_back(rule);
    }

    for (auto& [name, rules] : rule_nodes)
    {
      Node key = Term << (Scalar << (JSONString ^ name));
      std::optional<Node> maybe_node;
      if (rules[0]->type() == RuleFunc)
      {
        // this is a zero-argument function
        auto maybe_val = call_function(name, {ValueDef::create(rules[0])});
        if (maybe_val.has_value())
        {
          maybe_node = maybe_val.value()->node();
        }
      }
      else
      {
        maybe_node = resolve_rule(rules);
      }
      if (maybe_node.has_value())
      {
        Node val = maybe_node.value();
        if (val->type() == Error)
        {
          return val;
        }

        if (is_undefined(val))
        {
          continue;
        }

        object->push_back(ObjectItem << key << val);
      }
    }

    return Term << object;
  }

  std::optional<Node> UnifierDef::resolve_rule(const Nodes& defs) const
  {
    std::map<Token, Nodes> rules_by_type;
    for (auto& def : defs)
    {
      rules_by_type[def->type()].push_back(def);
    }

    if (rules_by_type.contains(RuleComp))
    {
      return resolve_rulecomp(rules_by_type[RuleComp]);
    }

    if (rules_by_type.contains(RuleSet))
    {
      return resolve_ruleset(rules_by_type[RuleSet]);
    }

    Node object = NodeDef::create(Object);
    if (rules_by_type.contains(RuleObj))
    {
      auto maybe_node = resolve_ruleobj(rules_by_type[RuleObj]);
      if (maybe_node.has_value())
      {
        object = maybe_node.value();
        if (object->type() == Error)
        {
          return object;
        }
      }
    }

    if (rules_by_type.contains(Submodule))
    {
      for (auto& submodule : rules_by_type[Submodule])
      {
        Node mod_object = resolve_module(submodule);
        if (mod_object->type() == Error)
        {
          return mod_object;
        }

        mod_object = mod_object->front();
        object->insert(object->end(), mod_object->begin(), mod_object->end());
      }
    }

    return object;
  }

  std::optional<Node> UnifierDef::resolve_rulecomp(const Nodes& rulecomps) const
  {
    rank_t rank = 0;
    Node result = nullptr;
    for (auto& rulecomp : rulecomps)
    {
      assert(rulecomp->type() == RuleComp);
      if (rulecomp->type() != RuleComp)
      {
        return std::nullopt;
      }

      Location rulename = (rulecomp / Var)->location();
      Node rulebody = rulecomp / Body;
      Node value = rulecomp / Val;
      rank_t index = ValueDef::get_rank(rulecomp / Idx);

      Node body_result;
      if (rulebody->type() == Empty)
      {
        body_result = True;
      }
      else
      {
        try
        {
          body_result = rule_unifier(rulename, rulebody)->unify();
        }
        catch (const std::exception& e)
        {
          return err(rulecomp, e.what());
        }
      }

      LOG("Rule comp body result: ", Resolver::term_str(body_result));

      if (body_result->type() == Error)
      {
        return body_result;
      }

      if (body_result->type() == True)
      {
        if (value->type() == UnifyBody)
        {
          LOG("Evaluating rule comp value");
          try
          {
            Unifier unifier = rule_unifier(rulename, value);
            unifier->unify();
            auto bindings = unifier->bindings();
            Node binding_val;
            for (auto& binding : bindings)
            {
              Node var = binding->front();
              if (var->location().view().starts_with("value$"))
              {
                binding_val = binding / Term;
                break;
              }
            }
            value = binding_val;
          }
          catch (const std::exception& e)
          {
            return err(rulecomp, e.what());
          }
        }
      }

      LOG("Rule comp value result: ", Resolver::term_str(value));

      if (value->type() == Undefined)
      {
        LOG("No value");
        continue;
      }

      if (body_result->type() == True)
      {
        if (result == nullptr)
        {
          rank = index;
          result = value;
        }
        else if (index < rank)
        {
          rank = index;
          result = value;
        }
        else if (index == rank)
        {
          std::string result_str = to_json(result);
          std::string value_str = to_json(value);
          if (result_str != value_str)
          {
            return err(
              rulecomp,
              "complete rules must not produce multiple outputs",
              EvalConflictError);
          }
        }
      }
    }

    if (result != nullptr)
    {
      return result;
    }

    return std::nullopt;
  }

  std::optional<RankedNode> UnifierDef::resolve_rulefunc(
    const Node& rulefunc, const Nodes& args) const
  {
    if (rulefunc->type() != RuleFunc)
    {
      return std::nullopt;
    }

    rank_t index = ValueDef::get_rank(rulefunc / Idx);
    Node rule = Resolver::inject_args(rulefunc, args);
    if (rule->type() == Error)
    {
      return std::make_pair(index, rule);
    }

    if (rule->type() == Undefined)
    {
      LOG("No value");
      return std::nullopt;
    }

    Location rulename = (rule / Var)->location();
    Node rulebody = rule / Body;
    Node body_result;

    if (rulebody->type() == Empty)
    {
      body_result = True;
    }
    else
    {
      try
      {
        body_result = rule_unifier(rulename, rulebody)->unify();
      }
      catch (const std::exception& e)
      {
        return std::make_pair(index, err(rulefunc, e.what()));
      }
    }

    LOG("Rule func body result: ", Resolver::term_str(body_result));

    if (body_result->type() == Error)
    {
      return std::make_pair(index, body_result);
    }

    if (body_result->type() == False || body_result->type() == Undefined)
    {
      LOG("No value");
      return std::nullopt;
    }

    Node value = rule / Val;

    if (value->type() == UnifyBody)
    {
      LOG("Evaluating rule func value");
      try
      {
        Unifier unifier = rule_unifier(rulename, value);
        unifier->unify();
        auto bindings = unifier->bindings();
        for (auto& binding : bindings)
        {
          Node var = binding / Var;
          if (var->location().view().starts_with("value$"))
          {
            value = binding / Term;
            break;
          }
        }
      }
      catch (const std::exception& e)
      {
        return std::make_pair(index, err(rulefunc, e.what()));
      }
    }

    return std::make_pair(index, value);
  }

  std::optional<Node> UnifierDef::resolve_ruleset(const Nodes& ruleset) const
  {
    Node argseq = NodeDef::create(ArgSeq);

    for (const auto& rule : ruleset)
    {
      assert(rule->type() == RuleSet);
      if (rule->type() != RuleSet)
      {
        return std::nullopt;
      }

      Location rulename = (rule / Var)->location();
      Node rulebody = rule / Body;
      Node value = rule / Val;

      // rulebody has not yet been unified
      Node body_result;
      if (rulebody->type() == Empty)
      {
        body_result = True;
      }
      else
      {
        try
        {
          body_result = rule_unifier(rulename, rulebody)->unify();
        }
        catch (const std::exception& e)
        {
          return err(rule, e.what());
        }
      }

      LOG("Rule set body result: ", Resolver::term_str(body_result));

      if (body_result->type() == Error)
      {
        return body_result;
      }

      if (body_result->type() == True)
      {
        if (value->type() == UnifyBody)
        {
          LOG("Evaluating rule set value");
          try
          {
            Unifier unifier = rule_unifier(rulename, value);
            unifier->unify();
            auto bindings = unifier->bindings();
            Node result;
            for (auto& binding : bindings)
            {
              Node var = binding / Var;
              if (var->location().view().starts_with("value$"))
              {
                result = binding / Term;
                break;
              }
            }
            value = result;
          }
          catch (const std::exception& e)
          {
            return err(rule, e.what());
          }
        }
      }

      if (body_result->type() == True && value->type() == Term)
      {
        Node set = value->front();
        argseq->insert(argseq->end(), set->begin(), set->end());
      }

      if (value->type() == Error)
      {
        return value;
      }
    }

    if (argseq->size() == 0)
    {
      LOG("No value");
      return NodeDef::create(Set);
    }

    return Resolver::set(argseq);
  }

  std::optional<Node> UnifierDef::resolve_ruleobj(const Nodes& ruleobj) const
  {
    Node argseq = NodeDef::create(ArgSeq);

    for (const auto& rule : ruleobj)
    {
      if (rule->type() == Error)
      {
        return rule;
      }
      assert(rule->type() == RuleObj);
      if (rule->type() != RuleObj)
      {
        return std::nullopt;
      }

      Location rulename = (rule / Var)->location();
      Node rulebody = rule / Body;
      Node value = rule / Val;

      // rulebody has not yet been unified
      Node body_result;
      if (rulebody->type() == Empty)
      {
        body_result = True;
      }
      else
      {
        try
        {
          body_result = rule_unifier(rulename, rulebody)->unify();
        }
        catch (const std::exception& e)
        {
          return err(rule, e.what());
        }
      }

      LOG("Rule obj body result: ", Resolver::term_str(body_result));

      if (body_result->type() == Error)
      {
        return body_result;
      }

      if (body_result->type() == True)
      {
        if (value->type() == UnifyBody)
        {
          LOG("Evaluating rule obj value");
          try
          {
            Unifier unifier = rule_unifier(rulename, value);
            unifier->unify();
            auto bindings = unifier->bindings();
            Node result;
            for (auto& binding : bindings)
            {
              Node var = binding / Var;
              if (var->location().view().starts_with("value$"))
              {
                result = binding / Term;
                break;
              }
            }
            value = result;
          }
          catch (const std::exception& e)
          {
            return err(rule, e.what());
          }
        }
      }

      if (body_result->type() == True && value->type() == Term)
      {
        Node obj = value->front();
        for (Node item : *obj)
        {
          argseq->push_back(item / Key);
          argseq->push_back(item / Val);
        }
      }

      if (value->type() == Error)
      {
        return value;
      }
    }

    if (argseq->size() == 0)
    {
      LOG("No value");
      return NodeDef::create(Object);
    }

    return Resolver::object(argseq, true);
  }

  Nodes UnifierDef::expressions() const
  {
    Nodes terms;
    for (auto& [loc, var] : m_variables)
    {
      if (loc.view().starts_with("unify$"))
      {
        terms.push_back(var.to_term());
      }
    }
    return terms;
  }

  Nodes UnifierDef::bindings() const
  {
    Nodes bindings;
    for (auto& [loc, var] : m_variables)
    {
      if (var.is_user_var())
      {
        bindings.push_back(Binding << (Var ^ loc) << var.to_term());
      }
    }
    return bindings;
  }

  void UnifierDef::push_with(const Node& withseq)
  {
    LOG("pushing with lookup");
    ValuesLookup lookup;
    for (auto& with : *withseq)
    {
      Node ref = with / RuleRef;
      Node var = with / Var;
      std::ostringstream os;
      os << Resolver::ref_str(ref);
      std::string key = os.str();
      lookup[key] = resolve_var(var);
    }

    m_with_stack->push_back(lookup);
  }

  void UnifierDef::pop_with()
  {
    LOG("popping with lookup");
    m_with_stack->pop_back();
  }

  Unifier UnifierDef::create(
    const Location& rule,
    const Node& rulebody,
    const CallStack& call_stack,
    const WithStack& with_stack,
    const BuiltIns& builtins,
    const UnifierCache& cache)
  {
    if (cache->contains(rulebody))
    {
      Unifier unifier = cache->at(rulebody);
      unifier->reset();
      return unifier;
    }
    else
    {
      Unifier unifier = std::make_shared<UnifierDef>(
        rule, rulebody, call_stack, with_stack, builtins, cache);
      cache->insert({rulebody, unifier});
      return unifier;
    }
  }

  Unifier UnifierDef::rule_unifier(
    const Location& rule, const Node& rulebody) const
  {
    return create(
      rule, rulebody, m_call_stack, m_with_stack, m_builtins, m_cache);
  }

  bool UnifierDef::is_variable(const Location& name) const
  {
    if (m_variables.contains(name))
    {
      return true;
    }

    return false;
  }

  Variable& UnifierDef::get_variable(const Location& name)
  {
    if (m_variables.contains(name))
    {
      return m_variables.at(name);
    }

    std::string name_str = std::string(name.view());
    throw std::runtime_error("Variable " + name_str + " not found");
  }

  void UnifierDef::push_not()
  {
    LOG("Pushing not: ", m_negate, " => ", !m_negate);
    m_negate = !m_negate;
  }

  void UnifierDef::pop_not()
  {
    LOG("Popping not: ", m_negate, " => ", !m_negate);
    m_negate = !m_negate;
  }
}