#include "errors.h"
#include "helpers.h"
#include "passes.h"

namespace rego
{
  const inline auto NotArg = ArithInfixArg / T(BoolInfix);
  const inline auto BoolInfixArg = T(RefTerm) / T(NumTerm) / T(UnaryExpr) /
    T(ArithInfix) / T(Term) / T(ExprCall) / T(BinInfix) / T(Set) / T(SetCompr);
  const inline auto RefArgBrackArg =
    T(Scalar) / T(Var) / T(Object) / T(Array) / T(Set);

  // Transforms boolean comparison operations into BoolInfix nodes.
  // Also generates LiteralNot nodes for not expressions.
  PassDef comparison()
  {
    return {
      In(UnifyBody) * (T(Literal) << (T(Expr) << (T(Not) * Any++[Expr]))) >>
        [](Match& _) {
          return LiteralNot << (UnifyBody << (Literal << (Expr << _[Expr])));
        },

      In(Expr) * (T(Expr) << (BoolInfixArg[Arg] * End)) >>
        [](Match& _) { return _(Arg); },

      In(Expr) * (BoolInfixArg[Lhs] * BoolToken[Op] * BoolInfixArg[Rhs]) >>
        [](Match& _) {
          std::set<Token> set_types = {Set, SetCompr};
          Node lhs = _(Lhs);
          if (set_types.contains(lhs->type()))
          {
            lhs = Term << lhs;
          }

          Node rhs = _(Rhs);
          if (set_types.contains(rhs->type()))
          {
            rhs = Term << rhs;
          }

          return BoolInfix << (BoolArg << lhs) << _(Op) << (BoolArg << rhs);
        },

      In(Expr) * (T(Set) / T(SetCompr))[Set] >>
        [](Match& _) { return Term << _(Set); },

      In(RefArgBrack) * (T(Term) << RefArgBrackArg[Arg]) >>
        [](Match& _) { return _(Arg); },

      (In(ArithArg) / In(BoolArg)) * (T(Expr) << ArithInfixArg[Val]) >>
        [](Match& _) { return _(Val); },

      // errors

      T(Expr)[Expr] << End >>
        [](Match& _) { return err(_(Expr), "Empty expression"); },

      In(Expr) * BoolToken[Op] >>
        [](Match& _) { return err(_(Op), "Invalid comparison"); },

      In(Expr) * T(Not)[Not] >>
        [](Match& _) { return err(_(Not), "Invalid not"); },

      In(BoolArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid boolean argument"); },

      In(BoolArg) * (T(Set) / T(SetCompr))[Set] >>
        [](Match& _) { return err(_(Set), "Invalid boolean argument"); },

      In(ArithArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid argument"); },

      In(BinArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid set argument"); },
    };
  }
}