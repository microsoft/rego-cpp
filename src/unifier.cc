#include "unifier.h"

#include "args.h"
#include "lang.h"
#include "log.h"
#include "resolver.h"
#include "wf.h"

#include <sstream>
#include <type_traits>

namespace rego
{
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
    auto name = local->front()->location();
    m_variables.insert({name, Variable(local)});
  }

  void Unifier::add_unifyexpr(const Node& unifyexpr)
  {
    m_unifyexprs.push_back(unifyexpr);

    Node lhs = unifyexpr->front();
    Node rhs = unifyexpr->back();
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
      Node lhs = unifyexpr->front();
      LOG(Resolver::expr_str(unifyexpr));
      Variable& var = m_variables.at(lhs->location());
      Values values = evaluate(lhs->location(), unifyexpr->back());
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
      std::string func_name = std::string(value->front()->location().view());
      Node args_node = value->back();
      if (func_name == "access_args")
      {
        Values arg_values = access_args(var, args_node->front());
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

    for (const auto& def : defs)
    {
      if (def->type() == Local)
      {
        if (m_variables.count(def->front()->location()) > 0)
        {
          // part of the unification
          Variable& var = m_variables.at(def->front()->location());
          Values valid_values = var.valid_values();
          values.insert(values.end(), valid_values.begin(), valid_values.end());
        }
        else
        {
          // a resolved local from another problem in the same rule
          // i.e. referring to a body local from the value.
          values.push_back(ValueDef::create(def->back()));
        }
      }
      else if (def->type() == ArgVar)
      {
        values.push_back(ValueDef::create(def->back()));
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

    return values;
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
          Nodes nodes = maybe_nodes.value();
          for (auto& node : nodes)
          {
            if (node->type() == RuleComp || node->type() == DefaultRule)
            {
              auto maybe_node = resolve_rulecomp(node);
              if (maybe_node)
              {
                values.push_back(
                  ValueDef::create(var, maybe_node.value(), sources));
              }
            }
            else
            {
              values.push_back(ValueDef::create(var, node, sources));
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

  Values Unifier::access_args(const Location& var, const Node& container_var)
  {
    Values args;
    Values container_values = resolve_var(container_var);
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
          args.push_back(
            ValueDef::create(var, Scalar << (JSONInt ^ std::to_string(i))));
        }
      }

      if (container->type() == Object)
      {
        for (const Node& object_item : *container)
        {
          std::string key =
            std::string(object_item->front()->location().view());
          args.push_back(ValueDef::create(var, Scalar << (JSONString ^ key)));
        }
      }

      if (container->type() == Set)
      {
        for (const Node& item : *container)
        {
          args.push_back(ValueDef::create(var, item));
        }
      }
    }

    for (auto& arg : args)
    {
      arg->mark_as_valid();
    }

    return args;
  }

  std::string Unifier::str() const
  {
    std::stringstream buf;
    buf << *this;
    return buf.str();
  }

  std::size_t Unifier::compute_dependency_score(const Node& unifyexpr)
  {
    std::size_t score = 0;
    std::vector<Location> deps;
    std::size_t num_vars = scan_vars(unifyexpr->back(), deps);
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

  std::optional<Node> Unifier::resolve_rulecomp(const Node& rulecomp)
  {
    if (rulecomp->type() == DefaultRule)
    {
      return DefaultTerm << rulecomp->at(wf_resolve / DefaultRule / Term)
                              ->at(wf_resolve / Term / Term);
    }

    assert(rulecomp->type() == RuleComp);

    Location rulename = rulecomp->at(0)->location();
    Node rulebody = rulecomp->at(1);
    Node value = rulecomp->back();
    if (rulebody->type() == JSONFalse)
    {
      return std::nullopt;
    }

    if (rulebody->type() == JSONTrue)
    {
      return value;
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
        return err(rulecomp, e.what());
      }
    }

    LOG("Rule comp body result: ", to_json(body_result));

    if (body_result->type() == Error)
    {
      return body_result;
    }

    if (body_result->type() == JSONTrue)
    {
      LOG("Evaluating rule comp value");
      try
      {
        Unifier unifier(rulename, value, m_call_stack);
        unifier.unify();
        Node result = unifier.bindings()[0]->back();
        rulecomp->replace(value, result);
        value = result;
      }
      catch (const std::exception& e)
      {
        return err(rulecomp, e.what());
      }
    }

    rulecomp->replace(rulebody, body_result);

    if (body_result->type() == JSONTrue)
    {
      return value;
    }

    LOG("No value");
    return std::nullopt;
  }

  std::optional<Node> Unifier::resolve_rulefunc(
    const Node& rulefunc, const Nodes& args)
  {
    assert(rulefunc->type() == RuleFunc);

    Node rule = Resolver::inject_args(rulefunc, args);
    if (rule->type() == Error)
    {
      return rule;
    }

    if (rule->type() == Undefined)
    {
      LOG("No value");
      return std::nullopt;
    }

    Location rulename = rule->at(0)->location();
    Node rulebody = rule->at(2);
    Node body_result;

    try
    {
      Unifier unifier(rulename, rulebody, m_call_stack);
      body_result = unifier.unify();
    }
    catch (const std::exception& e)
    {
      return err(rulefunc, e.what());
    }

    LOG("Rule func body result: ", to_json(body_result));

    if (body_result->type() == Error)
    {
      return body_result;
    }

    if (body_result->type() == JSONFalse || body_result->type() == Undefined)
    {
      LOG("No value");
      return std::nullopt;
    }

    LOG("Evaluating rule func value");

    Node value = rule->back();

    try
    {
      Unifier unifier(rulename, value, m_call_stack);
      unifier.unify();
      return unifier.bindings()[0]->back();
    }
    catch (const std::exception& e)
    {
      return err(rulefunc, e.what());
    }
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