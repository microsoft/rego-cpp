#include "passes.h"

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

          Node ruleargs = _(RuleArgs);
          Node newargs = NodeDef::create(RuleArgs);
          for (auto& arg : *ruleargs)
          {
            if (arg->type() == ArgVar)
            {
              newargs->push_back(arg);
              continue;
            }

            Location argloc = _.fresh({"arg"});
            body
              << (Literal
                  << (Expr << (RefTerm << (Var ^ argloc)) << Equals
                           << (Term << arg->front())));
            newargs->push_back(ArgVar << (Var ^ argloc) << Undefined);
          }

          return Seq << newargs << body;
        },

      // errors
      In(Literal) * T(SomeExpr)[SomeExpr] >>
        [](Match& _) { return err(_(SomeExpr), "Invalid some expression"); },
    };
  }
}