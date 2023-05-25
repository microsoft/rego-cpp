#include "rules_engine.h"

#include "lang.h"
#include "math.h"

#include <queue>
#include <set>

namespace rego
{
  Node RulesEngineDef::resolve(const Node& node) const
  {
    if (node->type() == Query)
    {
      return resolve_query(node);
    }

    return err(node, "Unsupported node");
  }

  Node RulesEngineDef::resolve_query(const Node& query) const
  {
    for (Node literal : *query)
    {
      query->replace(literal, resolve_literal(literal)->clone());
    }
    return query;
  }

  Node RulesEngineDef::resolve_literal(const Node& literal) const
  {
    Node node = literal->front();
    if (node->type() == Expr)
    {
      return resolve_expr(node);
    }

    if (node->type() == NotExpr)
    {
      return resolve_notexpr(node);
    }

    return err(literal, "Unsupported literal type");
  }

  Node RulesEngineDef::resolve_expr(const Node& expr) const
  {
    Node node = expr->front();
    if (node->type() == Term)
    {
      return resolve_term(node);
    }

    if (node->type() == NumTerm)
    {
      return resolve_numterm(node);
    }

    if (node->type() == RefTerm)
    {
      return resolve_refterm(node);
    }

    if (node->type() == BoolInfix)
    {
      Node result = resolve_boolinfix(node);
      if (result->type() == Undefined)
      {
        return Term << Undefined;
      }

      return Term << (Scalar << result);
    }

    if (node->type() == ArithInfix)
    {
      Node result = resolve_arithinfix(node);
      if (result->type() == Undefined)
      {
        return Term << Undefined;
      }

      return Term << (Scalar << result);
    }

    if (node->type() == UnaryExpr)
    {
      Node result = resolve_unaryexpr(node);
      if (result->type() == Undefined)
      {
        return Term << Undefined;
      }

      return Term << (Scalar << result);
    }

    return err(expr, "Unsupported expression type");
  }

  Node RulesEngineDef::resolve_notexpr(const Node& notexpr) const
  {
    Node value = resolve_expr(notexpr->front());
    if (value->type() == Error)
    {
      return value;
    }

    if (is_truthy(value))
    {
      return Term << (Scalar << JSONFalse);
    }

    return Term << (Scalar << JSONTrue);
  }

  Node RulesEngineDef::resolve_term(const Node& term) const
  {
    Node node = term->front();
    if (node->type() == Scalar)
    {
      return term;
    }

    if (node->type() == Object)
    {
      return Term << resolve_object(node);
    }

    if (node->type() == Array)
    {
      return Term << resolve_array(node);
    }

    if (node->type() == Set)
    {
      return Term << resolve_set(node);
    }

    return err(term, "Unsupported term type");
  }

  Node RulesEngineDef::resolve_numterm(const Node& numterm) const
  {
    Node value = numterm->front();
    return Term << (Scalar << value);
  }

  Node RulesEngineDef::resolve_refterm(const Node& refterm) const
  {
    Node node = refterm->front();
    if (node->type() == Var)
    {
      return resolve_var(node);
    }

    if (node->type() == Ref)
    {
      return resolve_ref(node);
    }

    return err(refterm, "Unsupported refterm");
  }

  Node RulesEngineDef::resolve_var(const Node& var) const
  {
    Nodes defs = var->lookup();
    if (defs.size() == 0)
    {
      return err(var, "Unsafe");
    }

    if (defs.front()->type() == RuleComp || defs.front()->type() == DefaultRule)
    {
      return resolve_rulecomp(find_valid_rulecomp(defs));
    }

    if (defs.front()->type() == RuleFunc)
    {
      return err(var, "Missing arguments");
    }

    if (defs.size() > 1)
    {
      return err(var, "Has multiple values");
    }

    Node result = defs.front();

    if (result->type() == Error)
    {
      return result;
    }

    if (result->type() == LocalRule)
    {
      return resolve_localrule(result);
    }

    if (result->type() == ArgVar)
    {
      return result->at(1);
    }

    return err(result, "Unsupported var");
  }

  Node RulesEngineDef::resolve_refhead(const Nodes& defs) const
  {
    Node refhead = find_valid(defs);
    if (refhead->type() == Error)
    {
      return refhead;
    }

    Node result;
    if (
      refhead->type() == Data || refhead->type() == Input ||
      refhead->type() == Error)
    {
      result = refhead;
    }
    else if (refhead->type() == DataModule)
    {
      result = refhead->at(1);
    }
    else if (refhead->type() == RuleComp)
    {
      result = resolve_rulecomp(refhead);
    }
    else if (refhead->type() == LocalRule)
    {
      result = resolve_localrule(refhead);
    }
    else if (refhead->type() == DefaultRule)
    {
      result = refhead->at(1);
    }
    else if (refhead->type() == RuleFunc)
    {
      result = refhead->at(2);
    }
    else if (refhead->type() == ObjectItem || refhead->type() == RefObjectItem)
    {
      result = refhead->back();
    }
    else if (refhead->type() == Term)
    {
      result = refhead->front();
    }
    else
    {
      return err(refhead, "Unsupported refhead");
    }

    if (result->type() == Expr)
    {
      result = resolve_expr(result);
    }

    if (result->type() == Term)
    {
      result = result->front();
    }

    return result;
  }

  Nodes RulesEngineDef::resolve_refdot(
    const Nodes& defs, const Node& refdot) const
  {
    Node refhead = resolve_refhead(defs);
    return lookdown(refhead, refdot->front());
  }

  Nodes RulesEngineDef::resolve_refbrack(
    const Nodes& defs, const Node& refbrack) const
  {
    Node refhead = resolve_refhead(defs);
    Node node = refbrack->front();

    if (node->type() == RefTerm)
    {
      node = resolve_refterm(node);
      if (node->type() == Error)
      {
        return {node};
      }

      if (node->type() == Term)
      {
        node = node->front();
      }
    }

    if (refhead->type() == Set)
    {
      if (node->type() == Object)
      {
        node = resolve_object(node);
      }

      if (node->type() == Array)
      {
        node = resolve_array(node);
      }

      if (node->type() == Set)
      {
        node = resolve_set(node);
      }

      return {resolve_set_membership(refhead, node)};
    }

    if (node->type() == Scalar)
    {
      Node value = node->front();
      if (value->type() == JSONInt)
      {
        if (refhead->type() != Array)
        {
          return {err(refhead, "Not an array")};
        }

        std::int64_t index = get_int(value);
        if (index < 0 || static_cast<size_t>(index) >= refhead->size())
        {
          return {Term << Undefined};
        }
        return {refhead->at(get_int(value))};
      }
      else if (value->type() == String)
      {
        return lookdown(refhead, value);
      }
      else
      {
        return {err(refbrack, "Unsupported index type")};
      }
    }
    else
    {
      return {err(refbrack, "Unsupported refarg")};
    }
  }

  Nodes RulesEngineDef::resolve_refcall(
    const Nodes& defs, const Node& refcall) const
  {
    Nodes args;
    for (const auto& expr : *refcall)
    {
      Node arg = resolve_expr(expr);
      if (arg->type() == Error)
      {
        return {arg};
      }

      args.push_back(arg);
    }

    return {find_valid_rulefunc(defs, args)};
  }

  Node RulesEngineDef::resolve_ref(const Node& ref) const
  {
    Node var = ref->front();
    Node refargs = ref->back();

    Nodes defs = var->lookup();
    for (Node refarg : *refargs)
    {
      if (defs.size() == 0)
      {
        return err(ref, "Invalid reference");
      }

      if (defs.front()->type() == Error)
      {
        break;
      }

      if (refarg->type() == RefArgDot)
      {
        defs = resolve_refdot(defs, refarg);
      }
      else if (refarg->type() == RefArgBrack)
      {
        defs = resolve_refbrack(defs, refarg);
      }
      else if (refarg->type() == RefArgCall)
      {
        defs = resolve_refcall(defs, refarg);
      }
      else
      {
        return err(refarg, "Unsupported refarg");
      }
    }

    if (defs.size() == 0)
    {
      return err(ref, "Invalid reference");
    }

    Node result = resolve_refhead(defs);

    if (result->type() == Error)
    {
      if (result->size() == 1)
      {
        result->push_back(ErrorAst << ref);
      }

      return result;
    }

    return Term << result;
  }

  Node RulesEngineDef::resolve_rulecomp(const Node& rulecomp) const
  {
    Node name = rulecomp->front();
    Node rulehead = rulecomp->at(1);
    if (rulehead->type() == Expr)
    {
      Node result = resolve_expr(rulehead);
      rulecomp->replace(rulehead, result);
      rulehead = result;
    }

    return rulehead;
  }

  Node RulesEngineDef::resolve_object(const Node& object) const
  {
    for (Node object_item : *object)
    {
      if (object_item->type() == RefObjectItem)
      {
        Node key = object_item->front();
        Node key_value = resolve_ref(key);
        object_item->replace(key, key_value);
      }

      Node expr = object_item->back();
      Node value = resolve_expr(expr);
      object_item->replace(expr, value);
    }

    return object;
  }

  Node RulesEngineDef::resolve_array(const Node& array) const
  {
    for (Node expr : *array)
    {
      array->replace(expr, resolve_expr(expr));
    }
    return array;
  }

  Node RulesEngineDef::resolve_arithinfix(const Node& arithinfix) const
  {
    Node lhs = resolve_aritharg(arithinfix->at(0));
    Node op = arithinfix->at(1);
    Node rhs = resolve_aritharg(arithinfix->at(2));

    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return Undefined;
    }

    if (lhs->type() == Error)
    {
      return lhs;
    }

    if (rhs->type() == Error)
    {
      return rhs;
    }

    if (lhs->type() == JSONInt && rhs->type() == JSONInt)
    {
      return math(op, get_int(lhs), get_int(rhs));
    }
    else
    {
      return math(op, get_double(lhs), get_double(rhs));
    }
  }

  Node RulesEngineDef::resolve_boolinfix(const Node& boolinfix) const
  {
    Node lhs = resolve_aritharg(boolinfix->at(0));
    Node op = boolinfix->at(1);
    Node rhs = resolve_aritharg(boolinfix->at(2));

    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return Undefined;
    }

    if (lhs->type() == Error)
    {
      return lhs;
    }

    if (rhs->type() == Error)
    {
      return rhs;
    }

    if (lhs->type() == JSONInt && rhs->type() == JSONInt)
    {
      return compare(op, get_int(lhs), get_int(rhs));
    }
    else
    {
      return compare(op, get_double(lhs), get_double(rhs));
    }
  }

  Node RulesEngineDef::resolve_unaryexpr(const Node& unaryexpr) const
  {
    Node value = resolve_aritharg(unaryexpr->front());
    if (value->type() == Undefined)
    {
      return Undefined;
    }

    if (value->type() == Error)
    {
      return value;
    }

    return negate(value);
  }

  Node RulesEngineDef::resolve_aritharg(const Node& aritharg) const
  {
    Node node = aritharg->front();
    if (node->type() == NumTerm)
    {
      return node->front();
    }

    if (node->type() == RefTerm)
    {
      node = resolve_refterm(node);
      if (node->type() == Error)
      {
        return node;
      }

      if (node->type() == Term)
      {
        node = node->front();
        if (node->type() == Scalar)
        {
          node = node->front();
          if (node->type() == JSONInt || node->type() == JSONFloat)
          {
            return node;
          }
        }
        else if (node->type() == Undefined)
        {
          return node;
        }
      }

      return err(node, "Invalid aritharg");
    }

    if (node->type() == UnaryExpr)
    {
      return resolve_unaryexpr(node);
    }

    if (node->type() == ArithInfix)
    {
      return resolve_arithinfix(node);
    }

    return err(node, "Unsupported aritharg");
  }

  bool RulesEngineDef::is_truthy(const Node& node) const
  {
    assert(node->type() == Term);
    Node value = node->front();
    if (value->type() == Scalar)
    {
      value = value->front();
      return value->type() != JSONFalse;
    }

    if (value->type() == Object || value->type() == Array)
    {
      return true;
    }

    return false;
  }

  Node RulesEngineDef::resolve_set(const Node& set) const
  {
    std::set<std::string> reprs;
    std::vector<Node> members;
    for (Node member : *set)
    {
      if (member->type() == Expr)
      {
        member = resolve_expr(member);
      }

      std::string repr = to_json(member);
      if (reprs.find(repr) == reprs.end())
      {
        reprs.insert(repr);
        members.push_back(member);
      }
    }

    set->erase(set->begin(), set->end());
    set->insert(set->end(), members.begin(), members.end());
    return set;
  }

  Node RulesEngineDef::resolve_set_membership(
    const Node& set, const Node& query) const
  {
    std::set<std::string> reprs;
    std::string query_repr = to_json(query);
    for (Node member : *set)
    {
      if (member->type() == Expr)
      {
        member = resolve_expr(member);
      }

      std::string repr = to_json(member);

      if (repr == query_repr)
      {
        return Term << (Scalar << JSONTrue);
      }
    }

    return Term << (Scalar << JSONFalse);
  }

  Nodes RulesEngineDef::object_lookdown(
    const Node& object, const Node& query) const
  {
    Nodes defs = object->lookdown(query->location());

    std::string query_str = to_json(query);
    for (auto& object_item : *object)
    {
      if (object_item->type() != RefObjectItem)
      {
        continue;
      }

      Node key = object_item->front();
      if (key->type() == Ref)
      {
        Node value = resolve_ref(key);
        object_item->replace(key, value);
        key = value;
      }

      std::string key_str = to_json(key);

      if (key_str == query_str)
      {
        defs.push_back(object_item);
      }
    }

    return defs;
  }

  Nodes RulesEngineDef::lookdown(const Node& parent, const Node& query) const
  {
    Nodes defs;
    if (parent->type() == Object)
    {
      defs = object_lookdown(parent, query);
    }
    else
    {
      defs = parent->lookdown(query->location());
    }

    return defs;
  }

  Node RulesEngineDef::find_valid_rulecomp(const Nodes& rules) const
  {
    Node result = Undefined;
    std::string result_str = to_json(result);
    Node default_value = Undefined;
    for (const auto& rule : rules)
    {
      if (rule->type() == RuleComp)
      {
        Node body_result = evaluate_body(rule->at(2));
        if (body_result->type() == Error)
        {
          return body_result;
        }

        if (body_result->type() == JSONFalse)
        {
          continue;
        }

        Node value = resolve_rulecomp(rule)->front();
        std::string value_str = to_json(value);
        if (result->type() == Undefined)
        {
          result = rule;
          result_str = value_str;
        }
        else if (value_str != result_str)
        {
          return err(rule, "complete rules must not produce multiple outputs");
        }
      }
      else if (rule->type() == DefaultRule)
      {
        default_value = rule;
      }
      else
      {
        return err(rule, "Unsupported rulecomp");
      }
    }

    if (result->type() == Undefined)
    {
      result = default_value;
    }

    if (result->type() == Undefined)
    {
      return Term << Undefined;
    }

    return result;
  }

  Node RulesEngineDef::evaluate_body(const Node& rulebody) const
  {
    for (Node item : *rulebody)
    {
      if (item->type() == Literal)
      {
        Node value = resolve_literal(item);
        if (value->type() == Error)
        {
          return value;
        }

        if (!is_truthy(value))
        {
          return JSONFalse;
        }
      }
      else if (item->type() == LocalRule)
      {
        continue;
      }
      else
      {
        return err(item, "Invalid rulebody item");
      }
    }

    return JSONTrue;
  }

  Node RulesEngineDef::resolve_localrule(const Node& localrule) const
  {
    bool cache = localrule->parent()->parent()->type() != RuleFunc;
    Node value = localrule->back();
    if (value->type() == Expr)
    {
      Node result = resolve_expr(value);
      if (cache)
      {
        localrule->replace(value, result);
      }

      value = result;
    }

    return value;
  }

  Node RulesEngineDef::resolve_rulefunc(const Node& rulefunc) const
  {
    return resolve_expr(rulefunc->at(2));
  }

  Node RulesEngineDef::find_valid_rulefunc(
    const Nodes& rules, const Nodes& args) const
  {
    Node result = Undefined;
    std::string result_str = to_json(result);
    for (const auto& raw_rule : rules)
    {
      if (raw_rule->type() == RuleFunc)
      {
        Node rule = inject_args(raw_rule, args);
        if (rule->type() == Error)
        {
          return rule;
        }

        if (rule->type() == Undefined)
        {
          continue;
        }

        Node body_result = evaluate_body(rule->at(3));
        if (body_result->type() == Error)
        {
          return body_result;
        }

        if (body_result->type() == JSONFalse)
        {
          continue;
        }

        Node value = resolve_rulefunc(rule)->front();
        std::string value_str = to_json(value);
        if (result->type() == Undefined)
        {
          result = rule;
          result_str = value_str;
        }
        else if (value_str != result_str)
        {
          return err(rule, "complete rules must not produce multiple outputs");
        }
      }
      else
      {
        return err(raw_rule, "Expected rulefunc");
      }
    }

    if (result->type() == Undefined)
    {
      return Term << Undefined;
    }

    return result;
  }

  Node RulesEngineDef::find_valid(const Nodes& defs) const
  {
    if (defs.front()->type() == RuleComp || defs.front()->type() == DefaultRule)
    {
      return find_valid_rulecomp(defs);
    }

    if (defs.size() > 1)
    {
      return err("Has multiple values");
    }

    return defs.front();
  }

  Node RulesEngineDef::inject_args(
    const Node& rulefunc, const Nodes& args) const
  {
    Node ruleargs = rulefunc->at(1);
    std::size_t num_args = ruleargs->size();
    if (num_args != args.size())
    {
      std::stringstream buf;
      buf << "function has arity " << num_args << ",  received " << args.size()
          << " arguments";
      return err(rulefunc, buf.str());
    }

    for (std::size_t i = 0; i < num_args; ++i)
    {
      Node rulearg = ruleargs->at(i);
      Node arg = args[i];
      if (rulearg->type() == ArgVal)
      {
        std::string rulearg_str = to_json(rulearg->front());
        std::string arg_str = to_json(arg);
        if (rulearg_str != arg_str)
        {
          return Undefined;
        }
      }
      else if (rulearg->type() == ArgVar)
      {
        rulearg->at(1) = arg;
      }
    }

    return rulefunc;
  }
}