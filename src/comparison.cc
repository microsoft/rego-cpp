#include "passes.h"

namespace rego
{
  PassDef comparison()
  {
    return {
      In(Expr) * (ArithInfixArg[Lhs] * BoolToken[Op] * ArithInfixArg[Rhs]) >>
        [](Match& _) {
          return BoolInfix << (ArithArg << _(Lhs)) << _(Op)
                           << (ArithArg << _(Rhs));
        },

      In(ArithArg) * (T(Expr) << ArithInfixArg[Value]) >>
        [](Match& _) { return _(Value); },

      // errors

      In(Expr) * BoolToken[Op] >>
        [](Match& _) { return err(_(Op), "Invalid comparison"); },

      In(ArithArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid argument"); },

      In(Expr) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Syntax error"); },
    };
  }
}