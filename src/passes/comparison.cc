#include "passes.h"

namespace rego
{
  const inline auto NotArg = ArithInfixArg / T(BoolInfix);
  const inline auto BoolInfixArg = ArithInfixArg / T(Term);

  PassDef comparison()
  {
    return {
      In(Expr) * (BoolInfixArg[Lhs] * BoolToken[Op] * BoolInfixArg[Rhs]) >>
        [](Match& _) {
          return BoolInfix << (BoolArg << _(Lhs)) << _(Op)
                           << (BoolArg << _(Rhs));
        },

      In(Literal) * (T(Expr) << (T(Not) * NotArg[Val])) >>
        [](Match& _) { return NotExpr << (Expr << _(Val)); },

      (In(ArithArg) / In(BoolArg)) * (T(Expr) << ArithInfixArg[Val]) >>
        [](Match& _) { return _(Val); },

      // errors

      In(Expr) * BoolToken[Op] >>
        [](Match& _) { return err(_(Op), "Invalid comparison"); },

      In(Expr) * T(Not)[Not] >>
        [](Match& _) { return err(_(Not), "Invalid not"); },

      In(Literal) * (T(NotExpr)[NotExpr] << End) >>
        [](Match& _) { return err(_(NotExpr), "Invalid not"); },

      In(BoolArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid boolean argument"); },

      In(ArithArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid argument"); },
    };
  }
}