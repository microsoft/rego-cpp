#include "unifier.h"

#include "CLI/TypeTools.hpp"
#include "args.h"
#include "lang.h"
#include "log.h"
#include "resolver.h"

#include <algorithm>
#include <iostream>
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
    | (UnifyExprWith <<= UnifyBody * WithSeq)
    | (DefaultRule <<= Var * Term)
    | (RuleComp <<= Var * (Body >>= JSONTrue | JSONFalse | UnifyBody | Empty) * (Val >>= Term | UnifyBody) * (Idx >>= JSONInt))
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody) * (Val >>= Term | UnifyBody) * (Idx >>= JSONInt))
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term))
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term))
    | (Function <<= JSONString * ArgSeq)
    | (ObjectItem <<= (Key >>= Term) * (Val >>= Term))
    | (Term <<= Scalar | Array | Object | Set)
    | (ArgVar <<= Var * Term)
    | (Binding <<= Var * Term)
    | (With <<= Ref * Var)
    ;
  // clang-format on
}

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
    m_cache(cache)
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
          scan_vars(wfi / with / Var, deps);
          std::transform(
            deps.begin(),
            deps.end(),
            std::back_inserter(dep_ids),
            [this](auto& dep) { return m_variables.at(dep).id(); });
        }

        m_dependency_graph[id].dependencies.insert(
          dep_ids.begin(), dep_ids.end());
      }
    });
  }

  std::size_t UnifierDef::add_variable(const Node& local)
  {
    auto name = (wfi / local / Var)->location();
    std::size_t id = m_dependency_graph.size();
    m_variables.insert({name, Variable(local, id)});
    m_dependency_graph.push_back({std::string(name.view()), {}, 0});
    return id;
  }

  std::size_t UnifierDef::add_unifyexpr(const Node& unifyexpr)
  {
    Node lhs = wfi / unifyexpr / Var;
    Node rhs = wfi / unifyexpr / Val;
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
      if (stmt->type() == UnifyExprWith)
      {
        LOG(Resolver::with_str(stmt));
        push_with(stmt / WithSeq);
        LOG_INDENT();
        auto nested = m_nested_statements[stmt];
        execute_statements(nested.begin(), nested.end());
        LOG_UNINDENT();
        pop_with();
      }
      else if (stmt->type() == UnifyExpr)
      {
        LOG(Resolver::expr_str(stmt));
        Node lhs = wfi / stmt / Var;
        Variable& var = get_variable(lhs->location());
        Values values = evaluate(lhs->location(), wfi / stmt / Val);
        if (values.size() == 0)
        {
          continue;
        }

        var.unify(values);
        LOG("> result: ", var);
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
              return JSONFalse ^ "false";
            }
            if (result->type() == Undefined)
            {
              LOG("> ", var.name().view(), ": TermSet => true");
              return JSONTrue ^ "true";
            }
          }
          else if (node->size() > 0 && result->type() == Undefined)
          {
            LOG("> ", var.name().view(), ": TermSet => true");
            return JSONTrue ^ "true";
          }
        }
        else if (var.is_unify() && Resolver::is_falsy(node))
        {
          LOG("> ", var.name().view(), ": false => false");
          return JSONFalse ^ "false";
        }
        else if (node->type() == Term && result->type() == Undefined)
        {
          LOG("> ", var.name().view(), ": Term => true");
          return JSONTrue ^ "true";
        }

        LOG("> ", var.name().view(), ": ", result->location().view());

        return result;
      });
  }

  Node UnifierDef::unify()
  {
    if (this->push_rule(this->m_rule))
    {
      throw std::runtime_error("Recursion detected in rule body");
    }

    LOG_HEADER("Unification", "=====");

    LOG("exprs: ");
    LOG_VECTOR_CUSTOM(m_statements, Resolver::stmt_str);

    LOG_INDENT();

    for (std::size_t pass_index = 0; pass_index < m_retries + 1; ++pass_index)
    {
      LOG_HEADER("Pass " + std::to_string(pass_index), "=====");
      pass();
      // mark_invalid_values();
      remove_invalid_values();
    }

    LOG_MAP_VALUES(m_variables);

    LOG_UNINDENT();
    Node result = bind_variables();
    LOG_HEADER("Complete", "=====");

    this->pop_rule(this->m_rule);
    return result;
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
      function_args.push_back(arg_values);
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
        [var](auto& value) { return ValueDef::copy_to(value, var); });
    }
    else if (value->type() == Scalar)
    {
      values.push_back(ValueDef::create(var, value));
    }
    else if (value->type() == Function)
    {
      std::string func_name =
        std::string((wfi / value / JSONString)->location().view());
      Node args_node = wfi / value / ArgSeq;
      if (func_name == "enumerate")
      {
        Values arg_values = enumerate(var, args_node->front());
        values.insert(values.end(), arg_values.begin(), arg_values.end());
      }
      else if (func_name == "to-values")
      {
        Values termsets = resolve_var(args_node->front());
        for (auto termset_value : termsets)
        {
          Node termset = termset_value->node();
          if (termset->type() == Term)
          {
            values.push_back(ValueDef::create(var, termset));
          }
          else if (termset->type() == TermSet)
          {
            for (auto term : *termset)
            {
              values.push_back(ValueDef::create(var, term));
            }
          }
          else
          {
            throw std::runtime_error("Not a term");
          }
        }
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
          Node termset = termset_value->node();
          if (termset->type() == Term)
          {
            argseq->push_back(termset);
          }
          else if (termset->type() == TermSet)
          {
            argseq->insert(argseq->end(), termset->begin(), termset->end());
          }
          else
          {
            throw std::runtime_error("Not a term");
          }
        }
        values.push_back(ValueDef::create(var, Resolver::array(argseq)));
      }
      else if (func_name == "set-compr")
      {
        Values termsets = resolve_var(args_node->front());
        Node argseq = NodeDef::create(ArgSeq);
        for (auto termset_value : termsets)
        {
          Node termset = termset_value->node();
          if (termset->type() == Term)
          {
            argseq->push_back(termset);
          }
          else if (termset->type() == TermSet)
          {
            argseq->insert(argseq->end(), termset->begin(), termset->end());
          }
          else
          {
            throw std::runtime_error("Not a term");
          }
        }
        values.push_back(ValueDef::create(var, Resolver::set(argseq)));
      }
      else if (func_name == "object-compr")
      {
        Values termsets = resolve_var(args_node->front());
        Node argseq = NodeDef::create(ArgSeq);
        for (auto termset_value : termsets)
        {
          Node termset = termset_value->node();
          if (termset->type() == Term)
          {
            Node array = termset->front();
            argseq->push_back(array->front());
            argseq->push_back(array->back());
          }
          else if (termset->type() == TermSet)
          {
            for (auto term : *termset)
            {
              Node array = term->front();
              argseq->push_back(array->front());
              argseq->push_back(array->back());
            }
          }
          else
          {
            throw std::runtime_error("Not a term");
          }
        }
        values.push_back(ValueDef::create(var, Resolver::object(argseq)));
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

  Values UnifierDef::resolve_var(const Node& node)
  {
    Values values = check_with(node);
    if (!values.empty())
    {
      return values;
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
          // a resolved local from another problem in the same rule
          // i.e. referring to a body local from the value.
          values.push_back(ValueDef::create(wfi / def / Val));
        }
        else if (def->type() == ArgVar)
        {
          values.push_back(ValueDef::create(wfi / def / Term));
        }
        else if (def->type() == Skip)
        {
          Values skip_values = resolve_skip(def);
          values.insert(values.end(), skip_values.begin(), skip_values.end());
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

  Values UnifierDef::call_function(
    const Location& var, const std::string& func_name, const Values& args)
  {
    Values values;
    Values sources;
    std::copy_if(
      args.begin(), args.end(), std::back_inserter(sources), [this](auto& arg) {
        return is_variable(arg->var());
      });

    if (func_name == "arithinfix")
    {
      Node result =
        Resolver::arithinfix(args[0]->node(), args[1]->node(), args[2]->node());
      values.push_back(ValueDef::create(var, result, sources));
    }
    else if (func_name == "bininfix")
    {
      Node result =
        Resolver::bininfix(args[0]->node(), args[1]->node(), args[2]->node());
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
    else if (func_name == "membership-tuple")
    {
      Node result =
        Resolver::membership(args[0]->node(), args[1]->node(), args[2]->node());
      values.push_back(ValueDef::create(var, result, sources));
    }
    else if (func_name == "membership-single")
    {
      Node result = Resolver::membership(args[0]->node(), args[1]->node());
      values.push_back(ValueDef::create(var, result, sources));
    }
    else if (func_name == "apply_access")
    {
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
    else if (func_name == "every")
    {
      Node varseq = args[0]->node();
      Node nestedbody = args[1]->node();
      values.push_back(
        ValueDef::create(var, resolve_every(varseq, nestedbody), sources));
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

      if (m_builtins.is_builtin(function->location()))
      {
        Node node = m_builtins.call(function->location(), function_args);
        values.push_back(ValueDef::create(var, node, sources));
      }
      else
      {
        auto maybe_node = resolve_rulefunc(function, function_args);
        if (maybe_node)
        {
          values.push_back(ValueDef::create(var, maybe_node.value(), sources));
        }
      }
    }

    return values;
  }

  Values UnifierDef::enumerate(const Location& var, const Node& container_var)
  {
    Values items;
    Values container_values = resolve_var(container_var);
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
          Node index = Scalar << (JSONInt ^ std::to_string(i));
          Node tuple = Term << (Array << index << container->at(i));
          items.push_back(ValueDef::create(var, tuple));
        }
      }

      if (container->type() == Object)
      {
        for (const Node& object_item : *container)
        {
          Node tuple = Term
            << (Array << wfi / object_item / Key << wfi / object_item / Val);
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

  Values UnifierDef::check_with(const Node& var)
  {
    std::string key_str = std::string(var->location().view());

    Values result;
    std::map<std::string, Values> partials;
    for (auto it = m_with_stack->rbegin(); it != m_with_stack->rend(); ++it)
    {
      for (auto& [key, val] : *it)
      {
        if (key == key_str)
        {
          LOG("Found key: ", key_str, " in with stack");
          result = val;
          if (m_parent_type == RuleFunc && m_builtins.is_builtin(key_str))
          {
            if (
              std::find_if(
                result.begin(), result.end(), [&](const Value& value) {
                  Node node = value->node();
                  if (node->type() == RuleFunc)
                  {
                    return (node / Var)->location() == m_rule;
                  }

                  return false;
                }) != result.end())
            {
              LOG("Recursion detected in rule-func: ", key_str);
              return {};
            }
          }

          break;
        }
        else if (key.starts_with(key_str) && !partials.contains(key))
        {
          LOG("Found prefix match: ", key_str, " in with stack");
          partials[key.substr(key_str.size() + 1)] = val;
        }
      }
    }

    if (partials.size() == 0)
    {
      return result;
    }

    std::map<std::string, Node> object_map;
    if (result.size() > 0)
    {
      Node base = result.front()->node();
      for (auto& item : *base)
      {
        std::string key =
          strip_quotes(std::string((item / Key)->location().view()));
        object_map[key] = item / Val;
      }
    }

    for (auto& [key, val] : partials)
    {
      if (val.size() == 0)
      {
        continue;
      }

      object_map[key] = val.front()->node();
    }

    Node object = NodeDef::create(Object);
    for (auto& [loc, val] : object_map)
    {
      Node key = Term << (Scalar << (JSONString ^ loc));
      object->push_back(ObjectItem << key << val);
    }

    return {ValueDef::create(object)};
  }

  Values UnifierDef::resolve_skip(const Node& skip)
  {
    assert(skip->type() == Skip);
    LOG("Resolving skip: ", to_json(skip->front()));
    Values values = check_with(skip->front());
    if (!values.empty())
    {
      return values;
    }

    // not overwritten by with.
    Node ref = skip->back();

    if (ref->type() == Undefined)
    {
      // this is likely a skip to a with-only location
      values.push_back(ValueDef::create(
        err(skip, "Undefined reference (missing document or with?)")));
    }
    else if (ref->type() == BuiltInHook)
    {
      values.push_back(ValueDef::create(ref));
    }
    else if (ref->type() == RuleRef)
    {
      Node varseq = ref->front();
      Node value;
      Nodes rules;
      for (auto& var : *varseq)
      {
        if (value == nullptr)
        {
          value = var->lookup()[0];
        }
        else
        {
          std::string key_str = std::string(var->location().view());
          rules = value->lookdown(key_str);
          value = rules[0] / Val;
        }
      }

      auto peek_type = rules[0]->type();
      if (peek_type == RuleSet)
      {
        // construct a set from all valid rules
        auto maybe_node = resolve_ruleset(rules);
        if (maybe_node)
        {
          values.push_back(ValueDef::create(*maybe_node));
        }
      }
      else if (peek_type == RuleObj)
      {
        // construct an object from all valid rules
        auto maybe_node = resolve_ruleobj(rules);
        if (maybe_node)
        {
          values.push_back(ValueDef::create(*maybe_node));
        }
      }
      else
      {
        for (auto& rule : rules)
        {
          if (rule->type() == RuleComp)
          {
            Node body = rule / Body;
            Node val = rule / Val;
            if (body->type() == Empty && val->type() == Term)
            {
              values.push_back(ValueDef::create(val->clone()));
            }
            else
            {
              auto maybe_node = resolve_rulecomp(rule);
              if (maybe_node)
              {
                values.push_back(ValueDef::create(maybe_node.value()));
              }
            }
          }
          else if (rule->type() == DefaultRule)
          {
            auto maybe_node = resolve_rulecomp(rule);
            if (maybe_node)
            {
              values.push_back(ValueDef::create(maybe_node.value()));
            }
          }
          else
          {
            values.push_back(ValueDef::create(rule));
          }
        }
      }
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

  std::optional<RankedNode> UnifierDef::resolve_rulecomp(const Node& rulecomp)
  {
    if (rulecomp->type() == DefaultRule)
    {
      rank_t index = std::numeric_limits<rank_t>::max();
      return std::make_pair(
        index, DefaultTerm << (wfi / rulecomp / Term)->front());
    }

    assert(rulecomp->type() == RuleComp);
    if (rulecomp->type() != RuleComp)
    {
      return std::nullopt;
    }

    Location rulename = (wfi / rulecomp / Var)->location();
    Node rulebody = wfi / rulecomp / Body;
    Node value = wfi / rulecomp / Val;
    rank_t index = ValueDef::get_rank(wfi / rulecomp / Idx);

    Node body_result;
    if (rulebody->type() == Empty)
    {
      body_result = JSONTrue;
    }
    else
    {
      try
      {
        body_result = rule_unifier(rulename, rulebody)->unify();
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
          Unifier unifier = rule_unifier(rulename, value);
          unifier->unify();
          auto bindings = unifier->bindings();
          Node result;
          for (auto& binding : bindings)
          {
            Node var = binding->front();
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
          return std::make_pair(index, err(rulecomp, e.what()));
        }
      }
    }

    if (body_result->type() == JSONTrue)
    {
      return std::make_pair(index, value);
    }

    LOG("No value");
    return std::nullopt;
  }

  std::optional<RankedNode> UnifierDef::resolve_rulefunc(
    const Node& rulefunc, const Nodes& args)
  {
    if (rulefunc->type() != RuleFunc)
    {
      return std::nullopt;
    }

    rank_t index = ValueDef::get_rank(wfi / rulefunc / Idx);
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

    Location rulename = (wfi / rule / Var)->location();
    Node rulebody = wfi / rule / Body;
    Node body_result;

    if (rulebody->type() == Empty)
    {
      body_result = JSONTrue;
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

    Node value = wfi / rule / Val;

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

  std::optional<Node> UnifierDef::resolve_ruleset(const Nodes& ruleset)
  {
    Node argseq = NodeDef::create(ArgSeq);

    for (const auto& rule : ruleset)
    {
      assert(rule->type() == RuleSet);
      if (rule->type() != RuleSet)
      {
        return std::nullopt;
      }

      Location rulename = (wfi / rule / Var)->location();
      Node rulebody = wfi / rule / Body;
      Node value = wfi / rule / Val;

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
          body_result = rule_unifier(rulename, rulebody)->unify();
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

      if (body_result->type() == JSONTrue && value->type() == Term)
      {
        Node set = value->front();
        argseq->insert(argseq->end(), set->begin(), set->end());
      }
    }

    if (argseq->size() == 0)
    {
      LOG("No value");
      return std::nullopt;
    }

    return Resolver::set(argseq);
  }

  std::optional<Node> UnifierDef::resolve_ruleobj(const Nodes& ruleobj)
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

      Location rulename = (wfi / rule / Var)->location();
      Node rulebody = wfi / rule / Body;
      Node value = wfi / rule / Val;

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
          body_result = rule_unifier(rulename, rulebody)->unify();
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

      if (body_result->type() == JSONTrue && value->type() == Term)
      {
        Node obj = value->front();
        for (Node item : *obj)
        {
          argseq->push_back(item / Key);
          argseq->push_back(item / Val);
        }
      }
    }

    if (argseq->size() == 0)
    {
      LOG("No value");
      return std::nullopt;
    }

    return Resolver::object(argseq);
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
      Node ref = wfi / with / Ref;
      Node var = wfi / with / Var;
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
    CallStack call_stack,
    WithStack with_stack,
    const BuiltIns& builtins,
    UnifierCache cache)
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

  Unifier UnifierDef::rule_unifier(const Location& rule, const Node& rulebody)
  {
    return create(
      rule, rulebody, m_call_stack, m_with_stack, m_builtins, m_cache);
  }

  Node UnifierDef::resolve_every(const Node& varseq, const Node& nestedbody)
  {
    Node item = varseq->at(0);
    Node key, val;
    if (varseq->size() == 2)
    {
      val = varseq->at(1)->lookup()[0];
    }
    else
    {
      key = varseq->at(1)->lookup()[0];
      val = varseq->at(2)->lookup()[0];
    }

    Location name = (nestedbody / Key)->location();
    Node body = nestedbody / Val;

    Values item_values = resolve_var(item);
    for (Value item_value : item_values)
    {
      Node item_node = item_value->node();
      if (item_node->type() == Undefined)
      {
        return JSONFalse ^ "false";
      }

      if (item_node->type() == Term)
      {
        item_node = item_node->front();
      }
      else
      {
        return err(item_node, "Unsupported item node");
      }

      val->back() = item_node->at(1);
      if (key != nullptr)
      {
        key->back() = item_node->at(0);
      }
      Node result = rule_unifier(name, body)->unify();
      if (result->type() == JSONFalse)
      {
        return JSONFalse ^ "false";
      }

      if (result->type() == Error)
      {
        return result;
      }
    }

    return JSONTrue ^ "true";
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

  Values UnifierDef::resolve_compr(const Location& var, const Node& compr)
  {
    Values values;
    if (compr->type() == ArrayCompr)
    {
      Value value =
        ValueDef::create(var, resolve_var(compr->front())[0]->node());
    }
    else if (compr->type() == SetCompr)
    {
      Value value =
        ValueDef::create(var, resolve_var(compr->front())[0]->node());
    }
    else if (compr->type() == ObjectCompr)
    {
      Values key_values = resolve_var(compr / Key);
    }
    else
    {
      throw std::runtime_error("Unsupported comprehension type");
    }

    return values;
  }
}