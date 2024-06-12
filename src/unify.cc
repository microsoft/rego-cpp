#include "unify.hh"

namespace rego
{
  bool is_undefined(const Node& node)
  {
    if (node->type() == DataModule)
    {
      return false;
    }

    if (node->type() == Undefined)
    {
      return true;
    }

    for (auto& child : *node)
    {
      if (is_undefined(child))
      {
        return true;
      }
    }

    return false;
  }

  bool is_truthy(const Node& node)
  {
    assert(node->type() == Term || node->type() == TermSet);
    if (node->type() == TermSet)
    {
      return true;
    }

    Node value = node->front();
    if (value->type() == Scalar)
    {
      value = value->front();
      return value->type() != False;
    }

    if (
      value->type() == Object || value->type() == Array || value->type() == Set)
    {
      return true;
    }

    return false;
  }

  bool is_module(const Node& var)
  {
    return var->type().in({Submodule, Data});
  }

  bool in_query(const Node& node)
  {
    if (node->type() == Rego)
    {
      return false;
    }

    if (node->type() == RuleComp)
    {
      std::string name = std::string((node / Var)->location().view());
      return name.find("query$") != std::string::npos;
    }

    return in_query(node->parent()->shared_from_this());
  }

  bool contains_local(const Node& node)
  {
    if (node->type() == NestedBody)
    {
      return false;
    }

    if (node->type() == Var)
    {
      Nodes defs = node->lookup();
      if (defs.size() == 0)
      {
        // check if it is a temporary variable
        // (which may not yet be in the symbol table)
        return node->location().view().find('$') != std::string::npos;
      }
      return defs.size() == 1 && defs[0]->type() == Local;
    }

    for (auto& child : *node)
    {
      if (contains_local(child))
      {
        return true;
      }
    }

    return false;
  }

  bool contains_ref(const Node& node)
  {
    if (node->type() == NestedBody)
    {
      return false;
    }

    if (node->type() == Ref || node->type() == Var)
    {
      return true;
    }

    for (auto& child : *node)
    {
      if (contains_ref(child))
      {
        return true;
      }
    }

    return false;
  }

  Node expr_infix(Token op_token, Node lhs, Node rhs)
  {
    if (lhs != Expr)
    {
      lhs = Expr << lhs;
    }

    if (rhs != Expr)
    {
      rhs = Expr << rhs;
    }

    Node op;
    if (op_token.in({Add, Subtract, Multiply, Divide, Modulo}))
    {
      op = InfixOperator << (ArithOperator << op_token);
    }
    else if (op_token.in(
               {Equals,
                NotEquals,
                LessThan,
                LessThanOrEquals,
                GreaterThan,
                GreaterThanOrEquals}))
    {
      op = InfixOperator << (BoolOperator << op_token);
    }
    else if (op_token.in({Assign, Unify}))
    {
      op = InfixOperator << (AssignOperator << op_token);
    }
    else
    {
      op = err(op_token, "Unsupported infix operator");
    }

    return ExprInfix << lhs << op << rhs;
  }

  Rewriter unify(BuiltIns builtins)
  {
    return {
      "unify",
      {
        strings(),
        merge_data(),
        varrefheads(),
        lift_refheads(),
        symbols(),
        replace_argvals(),
        lift_query(),
        expand_imports(builtins),
        constants(),
        explicit_enums(),
        body_locals(builtins),
        value_locals(builtins),
        compr_locals(builtins),
        rules_to_compr(),
        compr(),
        absolute_refs(),
        merge_modules(),
        datarule(),
        skips(),
        infix(),
        assign(builtins),
        skip_refs(builtins),
        simple_refs(),
        init(),
        implicit_enums(),
        enum_locals(),
        rulebody(),
        lift_to_rule(),
        functions(),
        unifier(builtins),
        result(),
      },
      wf_unify_input,
    };
  }
}