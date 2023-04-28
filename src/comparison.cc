#include "passes.h"

namespace rego
{
  PassDef comparison()
  {
    return {
      In(Expression) * (ExpressionArg[Lhs] * CompOp[Op] * ExpressionArg[Rhs]) >>
        [](Match& _) {
          return Comparison << _(Op) << (Expression << _(Lhs))
                            << (Expression << _[Rhs]);
        },

      In(Expression) *
          (ExpressionArg[Lhs] * CompOp[Op] * T(Subtract) *
           ExpressionArg[Rhs]) >>
        [](Match& _) {
          auto negate = Math << Multiply << (Expression << (Int ^ "-1"))
                             << (Expression << _(Rhs));
          return Comparison << _(Op) << (Expression << _(Lhs))
                            << (Expression << negate);
        },

      T(Expression) << T(Expression)[Expression] >>
        [](Match& _) { return _(Expression); },

      // errors

      In(Expression) * (Any * T(Error)[Error]) >>
        [](Match& _) { return _(Error); },

      T(Expression) << T(Error)[Error] >> [](Match& _) { return _(Error); },

      In(Expression) * (CompOp / T(Subtract))[Op] >>
        [](Match& _) { return err(_(Op), "invalid comparison."); },

      In(Expression) * (Any * Any[Rhs]) >>
        [](Match& _) { return err(_(Rhs), "too many values in expression"); },
    };
  }
}