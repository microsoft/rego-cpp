#include "errors.h"
#include "helpers.h"
#include "passes.h"
#include "tokens.h"

namespace
{
  using namespace rego;

  bool contains_argval(const Node& ruleargs)
  {
    for (auto& child : *ruleargs)
    {
      if (child->type() == ArgVal)
      {
        return true;
      }
    }

    return false;
  }

  Node unwrap_term(const Node& expr)
  {
    Node term = expr;
    if (term->type() != Expr)
    {
      return err(term, "Expected expression");
    }

    term = term->front();
    if (term->type() != Term && term->type() != RefTerm)
    {
      return err(term, "Expected term");
    }

    return term->front();
  }

  void add_constraints(const Node& argref, const Node& argval, Nodes& stmts)
  {
    if (argval->type() == Scalar || argval->type() == Set)
    {
      stmts.push_back(
        Literal << (Expr << (RefTerm << argref) << Unify << (Term << argval)));
    }
    else if (argval->type() == Var)
    {
      stmts.push_back(
        Literal
        << (Expr << (RefTerm << argval) << Unify << (RefTerm << argref)));
    }
    else if (argval->type() == Array)
    {
      for (std::size_t i = 0; i < argval->size(); ++i)
      {
        Node itemref = argref->clone();
        Node item = unwrap_term(argval->at(i));
        (itemref / RefArgSeq)
          << (RefArgBrack
              << (Expr << (Term << (Scalar << (JSONInt ^ std::to_string(i))))));
        add_constraints(itemref, item, stmts);
      }
    }
    else if (argval->type() == Object)
    {
      for (auto& item : *argval)
      {
        Node itemref = argref->clone();
        Node key = item / Key;
        Node val = unwrap_term(item / Val);
        (itemref / RefArgSeq) << (RefArgBrack << key);
        add_constraints(itemref, val, stmts);
      }
    }
  }
}

namespace rego
{
  PassDef replace_argvals()
  {
    return {
      In(RuleFunc) *
          (T(RuleArgs)[RuleArgs](
             [](auto& n) { return contains_argval(*n.first); }) *
           (T(UnifyBody) / T(Empty))[Body]) >>
        [](Match& _) {
          Node body = _(Body);
          if (body->type() == Empty)
          {
            body = NodeDef::create(UnifyBody);
          }

          // we need to replace each argval with an argvar, and then
          // add statements to the body which perform the equivalent
          // constraints and initializations.

          Node ruleargs = _(RuleArgs);
          Node newargs = NodeDef::create(RuleArgs);
          Nodes stmts;
          for (auto& arg : *ruleargs)
          {
            if (arg->type() == ArgVar)
            {
              newargs->push_back(arg);
              continue;
            }

            Location argloc = _.fresh({"arg"});
            Node argref = Ref << (RefHead << (Var ^ argloc)) << RefArgSeq;
            Node argval = arg->front();
            add_constraints(argref, argval, stmts);
            newargs->push_back(ArgVar << (Var ^ argloc) << Undefined);
          }

          body->insert(body->begin(), stmts.begin(), stmts.end());

          return Seq << newargs << body;
        },

      // errors
      In(Literal) * T(SomeExpr)[SomeExpr] >>
        [](Match& _) { return err(_(SomeExpr), "Invalid some expression"); },
    };
  }
}