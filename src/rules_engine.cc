#include "rules_engine.h"

#include "math.h"

#include <queue>

namespace rego
{
  Node RulesEngineDef::resolve(const Node& node)
  {
    if (node->type() == Query)
    {
      return resolve_query(node);
    }

    return err(node, "Unsupported node");
  }

  Node RulesEngineDef::resolve_query(const Node& query)
  {
    for (Node literal : *query)
    {
      query->replace(literal, resolve_literal(literal)->clone());
    }
    return query;
  }

  Node RulesEngineDef::resolve_literal(const Node& literal)
  {
    Node node = literal->front();
    if (node->type() == Expr)
    {
      return resolve_expr(node);
    }

    return err(literal, "Unsupported literal type");
  }

  Node RulesEngineDef::resolve_expr(const Node& expr)
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

  Node RulesEngineDef::resolve_term(const Node& term)
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

    return err(term, "Unsupported term type");
  }

  Node RulesEngineDef::resolve_numterm(const Node& numterm)
  {
    Node value = numterm->front();
    return Term << (Scalar << value);
  }

  Node RulesEngineDef::resolve_refterm(const Node& refterm)
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

  Node RulesEngineDef::resolve_var(const Node& var)
  {
    Nodes defs = var->lookup();
    if (defs.size() == 0)
    {
      return Term << Undefined;
    }

    if (defs.size() > 1)
    {
      return err(var, "Multiple values not allowed");
    }

    Node result = defs.front();
    if (result->type() == RuleComp)
    {
      return resolve_rulecomp(result);
    }

    return err(result, "Unsupported var");
  }

  Node RulesEngineDef::resolve_refhead(const Node& refhead)
  {
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
    else if (refhead->type() == ObjectItem)
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

  Node RulesEngineDef::resolve_refdot(const Node& refhead, const Node& refdot)
  {
    Node var = refdot->front();
    Nodes defs = refhead->lookdown(var->location());
    if (defs.size() == 0)
    {
      return Term << Undefined;
    }
    if (defs.size() > 1)
    {
      return err(var, "Multiple values not allowed");
    }
    return defs.front();
  }

  Node RulesEngineDef::resolve_refbrack(
    const Node& refhead, const Node& refbrack)
  {
    Node node = refbrack->front();

    if (node->type() == RefTerm)
    {
      node = resolve_refterm(node);
      if (node->type() == Term)
      {
        node = node->front();
      }
    }

    if (node->type() == Scalar)
    {
      Node value = node->front();
      if (value->type() == JSONInt)
      {
        if (refhead->type() != Array)
        {
          return err(refhead, "Not an array");
        }

        long index = get_int(value);
        if (index < 0 || static_cast<size_t>(index) >= refhead->size())
        {
          return Term << Undefined;
        }
        return refhead->at(get_int(value));
      }
      else if (value->type() == String)
      {
        if (refhead->type() != Object)
        {
          return err(refbrack, "Invalid index");
        }

        Nodes brack_defs = refhead->lookdown(value->location());
        if (brack_defs.size() == 0)
        {
          return Term << Undefined;
        }
        if (brack_defs.size() > 1)
        {
          return err(refbrack, "Multiple values not allowed");
        }
        return brack_defs.front();
      }
      else
      {
        return err(refbrack, "Unsupported index type");
      }
    }
    else
    {
      return err(refbrack, "Unsupported refarg");
    }
  }

  Node RulesEngineDef::resolve_ref(const Node& ref)
  {
    Node var = ref->front();
    Node refargs = ref->back();

    Nodes defs = var->lookup();
    if (defs.size() == 0)
    {
      return Term << Undefined;
    }

    if (defs.size() > 1)
    {
      return err(var, "Multiple values not allowed");
    }

    Node refhead = defs.front();
    for (Node refarg : *refargs)
    {
      refhead = resolve_refhead(refhead);
      if (refhead->type() == Error)
      {
        break;
      }

      if (refarg->type() == RefArgDot)
      {
        refhead = resolve_refdot(refhead, refarg);
      }
      else if (refarg->type() == RefArgBrack)
      {
        refhead = resolve_refbrack(refhead, refarg);
      }
      else
      {
        return err(refarg, "Unsupported refarg");
      }
    }

    refhead = resolve_refhead(refhead);

    return Term << refhead;
  }

  Node RulesEngineDef::resolve_rulecomp(const Node& rulecomp)
  {
    Node name = rulecomp->front();
    Node rulehead = rulecomp->at(1);
    if (rulehead->type() == Expr)
    {
      Node rulebody = rulecomp->at(2)->front();
      bool body_ok = true;
      for (Node item : *rulebody)
      {
        if (item->type() == Expr)
        {
          Node value = resolve_expr(item);
          if (value->type() == Error)
          {
            return value;
          }

          if (!is_truthy(value))
          {
            body_ok = false;
            break;
          }
        }
        else if (item->type() == RuleComp)
        {
          continue;
        }
        else
        {
          return err(item, "Invalid rulebody item");
        }
      }

      if (body_ok)
      {
        if (rulehead->type() == Expr)
        {
          Node result = resolve_expr(rulehead);
          rulecomp->replace(rulehead, result);
          rulehead = result;
        }
        else
        {
          return err(rulehead, "Unsupported rulehead type");
        }
      }
      else
      {
        Node result = Term << Undefined;
        rulecomp->replace(rulehead, result);
        rulehead = result;
      }
    }

    return rulehead;
  }

  Node RulesEngineDef::resolve_object(const Node& object)
  {
    for (Node object_item : *object)
    {
      Node expr = object_item->back();
      Node value = resolve_expr(expr);
      object_item->replace(expr, value);
    }

    return object;
  }

  Node RulesEngineDef::resolve_array(const Node& array)
  {
    for (Node expr : *array)
    {
      array->replace(expr, resolve_expr(expr));
    }
    return array;
  }

  Node RulesEngineDef::resolve_arithinfix(const Node& arithinfix)
  {
    Node lhs = resolve_aritharg(arithinfix->at(0));
    Node op = arithinfix->at(1);
    Node rhs = resolve_aritharg(arithinfix->at(2));

    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return Undefined;
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

  Node RulesEngineDef::resolve_boolinfix(const Node& boolinfix)
  {
    Node lhs = resolve_aritharg(boolinfix->at(0));
    Node op = boolinfix->at(1);
    Node rhs = resolve_aritharg(boolinfix->at(2));

    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return Undefined;
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

  Node RulesEngineDef::resolve_unaryexpr(const Node& unaryexpr)
  {
    Node value = resolve_aritharg(unaryexpr->front());
    if (value->type() == Undefined)
    {
      return Undefined;
    }

    return negate(value);
  }

  Node RulesEngineDef::resolve_aritharg(const Node& aritharg)
  {
    Node node = aritharg->front();
    if (node->type() == NumTerm)
    {
      return node->front();
    }

    if (node->type() == RefTerm)
    {
      node = resolve_refterm(node);
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

  bool RulesEngineDef::is_truthy(const Node& node)
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
}