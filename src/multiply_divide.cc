#include "passes.h"

namespace rego
{

  PassDef multiply_divide()
  {
    return {
      In(Expr) *
          (ArithInfixArg[Lhs] * (T(Multiply) / T(Divide))[Op] *
           ArithInfixArg[Rhs]) >>
        [](Match& _) {
          return ArithInfix << (ArithArg << _(Lhs)) << _(Op)
                            << (ArithArg << _(Rhs));
        },

      T(Expr) << (T(Expr)[Expr] * End) >> [](Match& _) { return _(Expr); },

      In(Expr) *
          (ArithInfixArg[Lhs] * (T(Multiply) / T(Divide))[Op] * T(Subtract) *
           ArithInfixArg[Rhs]) >>
        [](Match& _) {
          return ArithInfix
            << (ArithArg << _(Lhs)) << _(Op)
            << (ArithArg << (UnaryExpr << (ArithArg << _(Rhs))));
        },

      // errors

      In(Expr) * (T(Multiply) / T(Divide))[Op] >>
        [](Match& _) { return err(_(Op), "Invalid multiply/divide"); },
    };
  }

}