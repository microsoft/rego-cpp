#include "unifier.h"

#include "args.h"
#include "lang.h"
#include "log.h"
#include "resolver.h"

#include <sstream>
#include <type_traits>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  inline const auto wfi =
      (Local <<= Var * (Val >>= Term | Undefined))
    | (UnifyExpr <<= Var * (Val >>= Var | Scalar | Function))
    | (DefaultRule <<= Var * Term)
    | (RuleComp <<= Var * (Body >>= JSONTrue | JSONFalse | UnifyBody | Empty) * (Val >>= Term | UnifyBody) * (Idx >>= JSONInt))
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody) * (Val >>= Term | UnifyBody) * (Idx >>= JSONInt))
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term))
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Key >>= UnifyBody | Term) * (Val >>= UnifyBody | Term))
    | (Function <<= JSONString * ArgSeq)
    | (ObjectItem <<= Key * Term)
    | (Term <<= Scalar | Array | Object | Set)
    | (ArgVar <<= Var * Term)
    | (Binding <<= Var * Term)
    ;
  // clang-format on
}

namespace rego
{
  using namespace wf::ops;

  std::ostream& operator<<(std::ostream& os, const Unifier& unifier)
  {
    for (const auto& [first, second] : unifier.m_variables)
    {
      os << first.view() << "(" << second.dependency_score() << ") -> {";
      std::string sep = "";
      for (const auto& dep : second.dependencies())
      {
        os << sep << dep.view();
        sep = ", ";
      }
      os << "}" << std::endl;
    }
    os << std::endl;
    for (const auto& unifyexpr : unifier.m_unifyexprs)
    {
      os << Resolver::expr_str(unifyexpr) << std::endl;
    }
    return os;
  }

  Unifier::Unifier(
    const Location& rule,
    const Node& rulebody,
    CallStack& call_stack,
    std::size_t max_passes) :
    m_rule(rule), m_call_stack(call_stack), m_max_passes(max_passes)
  {
    if (Unifier::push_rule(call_stack, rule))
    {
      throw std::runtime_error("Recursion detected in rule body");
    }
    init_from_body(rulebody);
    compute_dependency_scores();
    m_retries = Variable::detect_cycles(m_variables);
    if (m_retries > 0)
    {
      LOG("Detected ", m_retries, " cycles in dependency graph");
    }
  }

  Unifier::~Unifier()
  {
    Unifier::pop_rule(m_call_stack, m_rule);
  }

  void Unifier::init_from_body(const Node& rulebody)
  {
    std::for_each(
      rulebody->begin(), rulebody->end(), [this](const auto& child) {
        if (child->type() == Local)
        {
          add_variable(child);
        }
        else if (child->type() == UnifyExpr)
        {
          add_unifyexpr(child);
        }
      });
  }

  void Unifier::add_variable(const Node& local)
  {
    auto name = local->at(wfi / Local / Var)->location();
    m_variables.insert({name, Variable(local)});
  }

  void Unifier::add_unifyexpr(const Node& unifyexpr)
  {
    m_unifyexprs.push_back(unifyexpr);

    Node lhs = unifyexpr->at(wfi / UnifyExpr / Var);
    Node rhs = unifyexpr->at(wfi / UnifyExpr / Val);
    if (!is_local(lhs))
    {
      throw std::runtime_error("Unification target is not a local variable");
    }

    Variable& var = m_variables.at(lhs->location());
    std::vector<Location> deps;
    std::size_t num_vars = scan_vars(rhs, deps);
    var.increase_dependency_score(num_vars - deps.size());
    var.insert_dependencies(deps.begin(), deps.end());
  }

  void Unifier::compute_dependency_scores()
  {
    Variable::compute_dependency_scores(m_variables);

    std::sort(
      m_unifyexprs.begin(), m_unifyexprs.end(), [this](auto& lhs, auto& rhs) {
        return compute_dependency_score(lhs) < compute_dependency_score(rhs);
      });
  }

  bool Unifier::is_local(const Node& var)
  {
    return m_variables.count(var->location()) > 0;
  }

  std::size_t Unifier::scan_vars(
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

  bool Unifier::pass()
  {
    LOG_MAP_VALUES(m_variables);

    bool check_finalized = false;
    for (const auto& unifyexpr : m_unifyexprs)
    {
      Node lhs = unifyexpr->at(wfi / UnifyExpr / Var);
      LOG(Resolver::expr_str(unifyexpr));
      Variable& var = m_variables.at(lhs->location());
      Values values =
        evaluate(lhs->location(), unifyexpr->at(wfi / UnifyExpr / Val));
      if (values.size() == 0)
      {
        continue;
      }

      if (var.initialized())
      {
        // If the variable already has valid values,
        // we need to intersect the new values with the existing ones.
        if (var.intersect_with(values))
        {
          check_finalized = true;
        }
      }
      else
      {
        check_finalized = true;
        var.initialize(values);
      }

      // Any remaining values are valid and should be kept,
      // along with their dependents.
      var.mark_valid_values();
    }
    return check_finalized;
  }

  bool Unifier::mark_invalid_values()
  {
    bool finished = false;
    for (auto& [_, var] : m_variables)
    {
      if (!var.is_unify())
      {
        // only unification statements can mark values as invalid
        continue;
      }

      var.mark_invalid_values();

      if (var.all_falsy())
      {
        // If only one unification statement has only false values,
        // unification is impossible.
        finished = true;
      }
    }

    return finished;
  }

  bool Unifier::remove_invalid_values()
  {
    return std::accumulate(
      m_variables.begin(),
      m_variables.end(),
      false,
      [](bool changed, auto& pair) {
        if (pair.second.remove_invalid_values())
        {
          return true;
        }

        return changed;
      });
  }

  Node Unifier::bind_variables()
  {
    return std::accumulate(
      m_variables.begin(),
      m_variables.end(),
      NodeDef::create(JSONTrue),
      [](Node result, auto& pair) {
        Variable& var = pair.second;
        Node node = var.bind();
        if (node->type() == Error || node->type() == Undefined)
        {
          return node;
        }
        else if (node->type() == TermSet)
        {
          if (node->size() == 0 && (var.is_unify() || var.is_user_var()))
          {
            return NodeDef::create(JSONFalse);
          }
        }

        return result;
      });
  }

  Node Unifier::unify()
  {
    LOG_HEADER("Unification", "=====");

    LOG("exprs: ");
    LOG_VECTOR_CUSTOM(m_unifyexprs, std::function(Resolver::expr_str));

    LOG_INDENT();

    bool finalized = false;
    std::size_t pass_index = 0;
    for (; pass_index < m_max_passes && !finalized; ++pass_index)
    {
      LOG_HEADER("Pass " + std::to_string(pass_index), "=====");
      bool check_finalized = pass();

      if (mark_invalid_values())
      {
        check_finalized = false;
      }

      finalized = true;
      if (check_finalized)
      {
        if (remove_invalid_values())
        {
          finalized = false;
        }
      }

      if (finalized)
      {
        if (m_retries > 0)
        {
          m_retries -= 1;
          finalized = false;
        }
      }
    }

    LOG_MAP_VALUES(m_variables);
    LOG_UNINDENT();
    LOG_HEADER("Complete", "=====");

    if (pass_index == m_max_passes)
    {
      return err("Unification could not be resolved");
    }

    Node result = bind_variables();
    return result;
  }

  Args Unifier::create_args(const Node& args)
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
      function_args.push_back(arg_values);
    }

    return function_args;
  }

  Values Unifier::evaluate_function(
    const Location& var, const std::string& func_name, const Args& args)
  {
    LOG("> calling ", func_name, " with ", args);
    Values values;
    std::set<Value> valid_args;
    for (std::size_t i = 0; i < args.size(); ++i)
    {
      Values call_args = args.at(i);
      Values results = call_function(var, func_name, call_args);
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
    }

    args.mark_invalid(valid_args);

    return values;
  }

  Values Unifier::evaluate(const Location& var, const Node& value)
  {
    Values values;
    if (value->type() == Var)
    {
      Values source_values = resolve_var(value);
      std::transform(
        source_values.begin(),
        source_values.end(),
        std::back_inserter(values),
        [var](auto& value) { return ValueDef::copy_to(value, var); });
    }
    else if (value->type() == Scalar)
    {
      values.push_back(ValueDef::create(var, value));
    }
    else if (value->type() == Function)
    {
      std::string func_name =
        std::string(value->at(wfi / Function / JSONString)->location().view());
      Node args_node = value->at(wfi / Function / ArgSeq);
      if (func_name == "enumerate")
      {
        Values arg_values = enumerate(var, args_node->front());
        values.insert(values.end(), arg_values.begin(), arg_values.end());
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

  Values Unifier::resolve_var(const Node& node)
  {
    Values values;
    Nodes defs = node->lookup();
    if (defs.empty())
    {
      return values;
    }

    Token peek_type = defs.front()->type();
    if (peek_type == RuleSet)
    {
      // construct a set from all valid rules
      auto maybe_node = resolve_ruleset(defs);
      if (maybe_node)
      {
        values.push_back(ValueDef::create(*maybe_node));
      }
    }
    else if (peek_type == RuleObj)
    {
      // construct an object from all valid rules
      auto maybe_node = resolve_ruleobj(defs);
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
          Node name = def->at(wfi / Local / Var);
          if (m_variables.count(name->location()) > 0)
          {
            // part of the unification
            Variable& var = m_variables.at(name->location());
            Values valid_values = var.valid_values();
            values.insert(
              values.end(), valid_values.begin(), valid_values.end());
          }
          else
          {
            // a resolved local from another problem in the same rule
            // i.e. referring to a body local from the value.
            values.push_back(ValueDef::create(def->at(wfi / Local / Val)));
          }
        }
        else if (def->type() == ArgVar)
        {
          values.push_back(ValueDef::create(def->at(wfi / ArgVar / Term)));
        }
        else if (
          def->type() == Data || def->type() == Module ||
          def->type() == RuleFunc || def->type() == Input)
        {
          // these will always be an argument to apply_access
          values.push_back(ValueDef::create(def));
        }
        else if (def->type() == RuleComp || def->type() == DefaultRule)
        {
          auto maybe_node = resolve_rulecomp(def);
          if (maybe_node)
          {
            values.push_back(ValueDef::create(maybe_node.value()));
          }
        }
        else
        {
          values.push_back(
            ValueDef::create(err(def, "Unsupported definition type")));
        }
      }
    }

    return ValueDef::filter_by_rank(values);
  }

  Values Unifier::call_function(
    const Location& var, const std::string& func_name, const Values& args)
  {
    Values values;
    Values sources;
    std::copy_if(
      args.begin(), args.end(), std::back_inserter(sources), [this](auto& arg) {
        return m_variables.count(arg->var()) > 0;
      });

    if (func_name == "arithinfix")
    {
      Node result =
        Resolver::arithinfix(args[0]->node(), args[1]->node(), args[2]->node());
      values.push_back(ValueDef::create(var, result, sources));
    }
    else if (func_name == "boolinfix")
    {
      Node result =
        Resolver::boolinfix(args[0]->node(), args[1]->node(), args[2]->node());
      values.push_back(ValueDef::create(var, result, sources));
    }
    else if (func_name == "unary")
    {
      values.push_back(
        ValueDef::create(var, Resolver::unary(args[0]->node()), sources));
    }
    else if (func_name == "not")
    {
      Node term = args[0]->to_term();
      if (Resolver::is_truthy(term))
      {
        values.push_back(ValueDef::create(var, JSONFalse, sources));
      }
      else
      {
        values.push_back(ValueDef::create(var, JSONTrue, sources));
      }
    }
    else if (func_name == "apply_access")
    {
      Node container = args[0]->node();

      if (container->type() == Term)
      {
        container = container->at(wfi / Term / Term);
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
            if (peek_type == RuleSet)
            {
              auto maybe_node = resolve_ruleset(defs);
              if (maybe_node)
              {
                values.push_back(ValueDef::create(*maybe_node));
              }
            }
            else if (peek_type == RuleObj)
            {
              auto maybe_node = resolve_ruleobj(defs);
              if (maybe_node)
              {
                values.push_back(ValueDef::create(*maybe_node));
              }
            }
            else
            {
              for (auto& def : defs)
              {
                if (def->type() == RuleComp || def->type() == DefaultRule)
                {
                  auto maybe_node = resolve_rulecomp(def);
                  if (maybe_node)
                  {
                    values.push_back(
                      ValueDef::create(var, maybe_node.value(), sources));
                  }
                }
                else
                {
                  values.push_back(ValueDef::create(var, def, sources));
                }
              }
            }
          }
        }
      }
    }
    else if (func_name == "object")
    {
      Node object_items = NodeDef::create(ArgSeq);
      for (auto& arg : args)
      {
        if (arg->node()->type() == Undefined)
        {
          return values;
        }

        object_items->push_back(arg->node());
      }
      values.push_back(
        ValueDef::create(var, Resolver::object(object_items), sources));
    }
    else if (func_name == "array")
    {
      Node array_members = NodeDef::create(ArgSeq);
      for (auto& arg : args)
      {
        if (arg->node()->type() == Undefined)
        {
          return values;
        }

        array_members->push_back(arg->node());
      }
      values.push_back(
        ValueDef::create(var, Resolver::array(array_members), sources));
    }
    else if (func_name == "set")
    {
      Node set_members = NodeDef::create(ArgSeq);
      for (auto& arg : args)
      {
        if (arg->node()->type() == Undefined)
        {
          return values;
        }

        set_members->push_back(arg->node());
      }
      values.push_back(
        ValueDef::create(var, Resolver::set(set_members), sources));
    }
    else if (func_name == "call")
    {
      Node function = args[0]->node();
      Nodes function_args;
      std::transform(
        args.begin() + 1,
        args.end(),
        std::back_inserter(function_args),
        [](auto& arg) { return arg->node(); });

      auto maybe_node = resolve_rulefunc(function, function_args);
      if (maybe_node)
      {
        values.push_back(ValueDef::create(var, maybe_node.value(), sources));
      }
    }

    return values;
  }

  Values Unifier::enumerate(const Location& var, const Node& container_var)
  {
    Values items;
    Values container_values = resolve_var(container_var);
    bool add_source = is_local(container_var);
    for (auto& container_value : container_values)
    {
      Node container = container_value->node();

      if (container->type() == Term)
      {
        container = container->at(wfi / Term / Term);
      }

      if (container->type() == Array)
      {
        for (std::size_t i = 0; i < container->size(); ++i)
        {
          Node index = Scalar << (JSONInt ^ std::to_string(i));
          Node tuple = Term << (Array << index << container->at(i));
          Values sources({ValueDef::create(index)});
          if (add_source)
          {
            sources.push_back(container_value);
          }
          items.push_back(ValueDef::create(var, tuple, sources));
        }
      }

      if (container->type() == Object)
      {
        for (const Node& object_item : *container)
        {
          std::string key = std::string(
            object_item->at(wfi / ObjectItem / Key)->location().view());
          Node key_str = (Scalar << (JSONString ^ key));
          Node tuple = Term
            << (Array << key_str << object_item->at(wfi / ObjectItem / Term));

          Values sources({ValueDef::create(key_str)});
          if (add_source)
          {
            sources.push_back(container_value);
          }
          items.push_back(ValueDef::create(var, tuple, sources));
        }
      }

      if (container->type() == Set)
      {
        for (const Node& value : *container)
        {
          std::string key = to_json(value);
          Node key_str = (Scalar << (JSONString ^ key));
          Node tuple = Term << (Array << value << value);

          Values sources({ValueDef::create(key_str)});
          if (add_source)
          {
            sources.push_back(container_value);
          }
          items.push_back(ValueDef::create(var, tuple, sources));
        }
      }
    }

    for (auto& item : items)
    {
      item->mark_as_valid();
    }

    return items;
  }

  std::string Unifier::str() const
  {
    std::stringstream buf;
    buf << *this;
    return buf.str();
  }

  std::string Unifier::dependency_str() const
  {
    std::stringstream buf;
    for (auto& [name, var] : m_variables)
    {
      buf << name.view() << "(" << var.dependency_score() << ") -> {";
      std::string sep = "";
      for (const auto& dep : var.dependencies())
      {
        buf << sep << dep.view();
        sep = ", ";
      }
      buf << "}" << std::endl;
    }

    return buf.str();
  }

  std::size_t Unifier::compute_dependency_score(const Node& unifyexpr)
  {
    std::size_t score = 0;
    std::vector<Location> deps;
    std::size_t num_vars =
      scan_vars(unifyexpr->at(wfi / UnifyExpr / Val), deps);
    score += num_vars - deps.size();
    for (auto& dep : deps)
    {
      score += m_variables.at(dep).dependency_score();
    }

    return score;
  }

  bool Unifier::push_rule(CallStack& callstack, const Location& rule)
  {
    LOG("Pushing rule: ", rule.view());
    LOG("Call stack: ", *callstack);

    if (
      std::find(callstack->begin(), callstack->end(), rule) != callstack->end())
    {
      return true;
    }

    callstack->push_back(rule);
    return false;
  }

  void Unifier::pop_rule(CallStack& callstack, const Location& rule)
  {
    if (callstack->size() == 0)
    {
      return;
    }

    if (callstack->back() != rule)
    {
      return;
    }

    LOG("Popping rule: ", callstack->back().view());
    LOG("Call stack: ", *callstack);
    callstack->pop_back();
  }

  std::optional<RankedNode> Unifier::resolve_rulecomp(const Node& rulecomp)
  {
    if (rulecomp->type() == DefaultRule)
    {
      std::int64_t index = std::numeric_limits<std::int64_t>::max();
      return std::make_pair(
        index,
        DefaultTerm
          << rulecomp->at(wfi / DefaultRule / Term)->at(wfi / Term / Term));
    }

    assert(rulecomp->type() == RuleComp);

    Location rulename = rulecomp->at(wfi / RuleComp / Var)->location();
    Node rulebody = rulecomp->at(wfi / RuleComp / Body);
    Node value = rulecomp->at(wfi / RuleComp / Val);
    std::int64_t index = Resolver::get_int(rulecomp->at(wfi / RuleComp / Idx));
    if (rulebody->type() == JSONFalse)
    {
      return std::nullopt;
    }

    if (rulebody->type() == JSONTrue)
    {
      return std::make_pair(index, value);
    }

    // rulebody has not yet been unified
    Node body_result;
    if (rulebody->type() == Empty)
    {
      body_result = JSONTrue;
    }
    else
    {
      try
      {
        Unifier unifier(rulename, rulebody, m_call_stack);
        body_result = unifier.unify();
      }
      catch (const std::exception& e)
      {
        return std::make_pair(index, err(rulecomp, e.what()));
      }
    }

    LOG("Rule comp body result: ", to_json(body_result));

    if (body_result->type() == Error)
    {
      return std::make_pair(index, body_result);
    }

    if (body_result->type() == JSONTrue)
    {
      if (value->type() == UnifyBody)
      {
        LOG("Evaluating rule comp value");
        try
        {
          Unifier unifier(rulename, value, m_call_stack);
          unifier.unify();
          Node result = unifier.bindings().front()->at(wfi / Binding / Term);
          rulecomp->replace(value, result);
          value = result;
        }
        catch (const std::exception& e)
        {
          return std::make_pair(index, err(rulecomp, e.what()));
        }
      }
    }

    rulecomp->replace(rulebody, body_result);

    if (body_result->type() == JSONTrue)
    {
      return std::make_pair(index, value);
    }

    LOG("No value");
    return std::nullopt;
  }

  std::optional<RankedNode> Unifier::resolve_rulefunc(
    const Node& rulefunc, const Nodes& args)
  {
    assert(rulefunc->type() == RuleFunc);

    std::int64_t index = Resolver::get_int(rulefunc->at(wfi / RuleFunc / Idx));
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

    Location rulename = rule->at(wfi / RuleFunc / Var)->location();
    Node rulebody = rule->at(wfi / RuleFunc / Body);
    Node body_result;

    try
    {
      Unifier unifier(rulename, rulebody, m_call_stack);
      body_result = unifier.unify();
    }
    catch (const std::exception& e)
    {
      return std::make_pair(index, err(rulefunc, e.what()));
    }

    LOG("Rule func body result: ", to_json(body_result));

    if (body_result->type() == Error)
    {
      return std::make_pair(index, body_result);
    }

    if (body_result->type() == JSONFalse || body_result->type() == Undefined)
    {
      LOG("No value");
      return std::nullopt;
    }

    Node value = rule->at(wfi / RuleFunc / Val);

    if (value->type() == UnifyBody)
    {
      LOG("Evaluating rule func value");
      try
      {
        Unifier unifier(rulename, value, m_call_stack);
        unifier.unify();
        value = unifier.bindings().front()->at(wfi / Binding / Term);
      }
      catch (const std::exception& e)
      {
        return std::make_pair(index, err(rulefunc, e.what()));
      }
    }

    return std::make_pair(index, value);
  }

  std::optional<Node> Unifier::resolve_ruleset(const Nodes& ruleset)
  {
    std::map<std::string, Node> members;

    for (const auto& rule : ruleset)
    {
      assert(rule->type() == RuleSet);
      Location rulename = rule->at(wfi / RuleSet / Var)->location();
      Node rulebody = rule->at(wfi / RuleSet / Body);
      Node value = rule->at(wfi / RuleSet / Val);
      if (rulebody->type() == JSONFalse)
      {
        continue;
      }

      if (rulebody->type() == JSONTrue)
      {
        members[to_json(value)] = value;
        continue;
      }

      // rulebody has not yet been unified
      Node body_result;
      if (rulebody->type() == Empty)
      {
        body_result = JSONTrue;
      }
      else
      {
        try
        {
          Unifier unifier(rulename, rulebody, m_call_stack);
          body_result = unifier.unify();
        }
        catch (const std::exception& e)
        {
          return err(rule, e.what());
        }
      }

      LOG("Rule set body result: ", to_json(body_result));

      if (body_result->type() == Error)
      {
        return body_result;
      }

      if (body_result->type() == JSONTrue)
      {
        if (value->type() == UnifyBody)
        {
          LOG("Evaluating rule set value");
          try
          {
            Unifier unifier(rulename, value, m_call_stack);
            unifier.unify();
            Node result = unifier.bindings().front()->at(wfi / Binding / Term);
            rule->replace(value, result);
            value = result;
          }
          catch (const std::exception& e)
          {
            return err(rule, e.what());
          }
        }
      }

      rule->replace(rulebody, body_result);

      if (body_result->type() == JSONTrue)
      {
        members[to_json(value)] = value;
      }
    }

    if (members.size() == 0)
    {
      LOG("No value");
      return std::nullopt;
    }

    Node set = NodeDef::create(Set);
    for (auto& [_, value] : members)
    {
      set->push_back(value);
    }
    return set;
  }

  std::optional<Node> Unifier::resolve_ruleobj(const Nodes& ruleobj)
  {
    std::map<std::string, Node> members;

    for (const auto& rule : ruleobj)
    {
      assert(rule->type() == RuleObj);
      Location rulename = rule->at(wfi / RuleObj / Var)->location();
      Node rulebody = rule->at(wfi / RuleObj / Body);
      Node key = rule->at(wfi / RuleObj / Key);
      Node value = rule->at(wfi / RuleObj / Val);
      if (rulebody->type() == JSONFalse)
      {
        continue;
      }

      if (rulebody->type() == JSONTrue)
      {
        members[to_json(key)] = value;
        continue;
      }

      // rulebody has not yet been unified
      Node body_result;
      if (rulebody->type() == Empty)
      {
        body_result = JSONTrue;
      }
      else
      {
        try
        {
          Unifier unifier(rulename, rulebody, m_call_stack);
          body_result = unifier.unify();
        }
        catch (const std::exception& e)
        {
          return err(rule, e.what());
        }
      }

      LOG("Rule obj body result: ", to_json(body_result));

      if (body_result->type() == Error)
      {
        return body_result;
      }

      if (body_result->type() == JSONTrue)
      {
        if (key->type() == UnifyBody)
        {
          LOG("Evaluating rule obj key");
          try
          {
            Unifier unifier(rulename, key, m_call_stack);
            unifier.unify();
            Node result = unifier.bindings().front()->at(wfi / Binding / Term);
            rule->replace(key, result);
            key = result;
          }
          catch (const std::exception& e)
          {
            return err(rule, e.what());
          }
        }

        if (value->type() == UnifyBody)
        {
          LOG("Evaluating rule obj value");
          try
          {
            Unifier unifier(rulename, value, m_call_stack);
            unifier.unify();
            Node result = unifier.bindings().front()->at(wfi / Binding / Term);
            rule->replace(value, result);
            value = result;
          }
          catch (const std::exception& e)
          {
            return err(rule, e.what());
          }
        }
      }

      rule->replace(rulebody, body_result);

      if (body_result->type() == JSONTrue)
      {
        members[to_json(key)] = value;
      }
    }

    if (members.size() == 0)
    {
      LOG("No value");
      return std::nullopt;
    }

    Node obj = NodeDef::create(Object);
    for (auto& [key, value] : members)
    {
      std::string key_loc = key;
      if (key_loc.starts_with('"') && key_loc.ends_with('"'))
      {
        key_loc = key_loc.substr(1, key_loc.size() - 2);
      }

      obj->push_back(ObjectItem << (Key ^ key_loc) << value);
    }

    return obj;
  }

  Nodes Unifier::expressions() const
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

  Nodes Unifier::bindings() const
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
}