#include "passes.h"

namespace rego
{
  PassDef add_subtract()
  {
    return {
      In(Expression) *
          (ExpressionArg[Lhs] * (T(Add) / T(Subtract))[Op] *
           ExpressionArg[Rhs]) >>
        [](Match& _) {
          return Math << _(Op) << (Expression << _(Lhs))
                      << (Expression << _[Rhs]);
        },

      In(Expression) *
          (ExpressionArg[Lhs] * T(Add) * T(Subtract) * ExpressionArg[Rhs]) >>
        [](Match& _) {
          return Math << Subtract << (Expression << _(Lhs))
                      << (Expression << _(Rhs));
        },

      // errors

      In(Expression) * (T(Add))[Op] >>
        [](Match& _) { return err(_(Op), "invalid operation"); },
    };
  }

}