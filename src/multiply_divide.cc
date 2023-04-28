#include "passes.h"

namespace rego
{

  PassDef multiply_divide()
  {
    return {
      In(Expression) *
          (ExpressionArg[Lhs] * (T(Multiply) / T(Divide))[Op] *
           ExpressionArg[Rhs]) >>
        [](Match& _) {
          return Math << _(Op) << (Expression << _(Lhs))
                      << (Expression << _(Rhs));
        },

      In(Expression) * (Start * T(Subtract) * ExpressionArg[Value]) >>
        [](Match& _) {
          return Math << Multiply << (Expression << (Int ^ "-1"))
                      << (Expression << _(Value));
        },

      In(Expression) *
          (ExpressionArg[Lhs] * (T(Multiply) / T(Divide))[Op] * T(Subtract) *
           ExpressionArg[Rhs]) >>
        [](Match& _) {
          auto negate = Math << Multiply << (Expression << (Int ^ "-1"))
                             << (Expression << _(Rhs));
          return Math << _(Op) << (Expression << _(Lhs))
                      << (Expression << negate);
        },

      // errors

      In(Expression) * (T(Multiply) / T(Divide))[Op] >>
        [](Match& _) { return err(_(Op), "invalid operation"); },
    };
  }

}