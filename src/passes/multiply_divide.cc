#include "passes.h"

namespace rego
{
  const inline auto Ops = T(Multiply) / T(Divide) / T(Modulo);

  // Processes multiply, divide, and modulo operations into ArithInfix nodes.
  PassDef multiply_divide()
  {
    return {
      In(Expr) * (ArithInfixArg[Lhs] * Ops[Op] * ArithInfixArg[Rhs]) >>
        [](Match& _) {
          return ArithInfix << (ArithArg << _(Lhs)) << _(Op)
                            << (ArithArg << _(Rhs));
        },

      T(Expr) << (T(Expr)[Expr] * End) >> [](Match& _) { return _(Expr); },

      In(Expr) *
          (ArithInfixArg[Lhs] * Ops[Op] * T(Subtract) * ArithInfixArg[Rhs]) >>
        [](Match& _) {
          return ArithInfix
            << (ArithArg << _(Lhs)) << _(Op)
            << (ArithArg << (UnaryExpr << (ArithArg << _(Rhs))));
        },

      In(Expr) * (BinInfixArg[Lhs] * T(And) * BinInfixArg[Rhs]) >>
        [](Match& _) {
          return BinInfix << (BinArg << _(Lhs)) << And << (BinArg << _(Rhs));
        },

      // errors

      In(Expr) * Ops[Op] >>
        [](Match& _) { return err(_(Op), "Invalid multiply/divide"); },

      In(Expr) * T(And)[And] >>
        [](Match& _) { return err(_(And), "Invalid and"); },
    };
  }

}