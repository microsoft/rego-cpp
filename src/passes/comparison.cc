#include "internal.hh"

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
      "comparison",
      wf_pass_comparison,
      dir::topdown,
      {
      In(UnifyBody) * (T(Literal) << (T(Expr) << (T(Not) * Any++[Expr]))) >>
        [](Match& _) {
          ACTION();
          return LiteralNot << (UnifyBody << (Literal << (Expr << _[Expr])));
        },

      In(Expr) * (T(Expr) << (BoolInfixArg[Arg] * End)) >>
        [](Match& _) {
          ACTION();
          return _(Arg);
        },

      In(Expr) * (BoolInfixArg[Lhs] * BoolToken[Op] * BoolInfixArg[Rhs]) >>
        [](Match& _) {
          ACTION();
          std::set<Token> set_types = {Set, SetCompr};
          Node lhs = _(Lhs);
          if (contains(set_types, lhs->type()))
          {
            lhs = Term << lhs;
          }

          Node rhs = _(Rhs);
          if (contains(set_types, rhs->type()))
          {
            rhs = Term << rhs;
          }

          return BoolInfix << (BoolArg << lhs) << _(Op) << (BoolArg << rhs);
        },

      In(Expr) * (T(Set) / T(SetCompr))[Set] >>
        [](Match& _) {
          ACTION();
          return Term << _(Set);
        },

      In(RefArgBrack) * (T(Term) << RefArgBrackArg[Arg]) >>
        [](Match& _) {
          ACTION();
          return _(Arg);
        },

      In(ArithArg, BoolArg) * (T(Expr) << ArithInfixArg[Val]) >>
        [](Match& _) {
          ACTION();
          return _(Val);
        },

      // errors

      T(Expr)[Expr] << End >>
        [](Match& _) {
          ACTION();
          return err(_(Expr), "Empty expression");
        },

      In(Expr) * BoolToken[Op] >>
        [](Match& _) {
          ACTION();
          return err(_(Op), "Invalid comparison");
        },

      In(Expr) * T(Not)[Not] >>
        [](Match& _) {
          ACTION();
          return err(_(Not), "Invalid not");
        },

      In(BoolArg) * T(Expr)[Expr] >>
        [](Match& _) {
          ACTION();
          return err(_(Expr), "Invalid boolean argument");
        },

      In(BoolArg) * (T(Set) / T(SetCompr))[Set] >>
        [](Match& _) {
          ACTION();
          return err(_(Set), "Invalid boolean argument");
        },

      In(ArithArg) * T(Expr)[Expr] >>
        [](Match& _) {
          ACTION();
          return err(_(Expr), "Invalid argument");
        },

      In(BinArg) * T(Expr)[Expr] >>
        [](Match& _) {
          ACTION();
          return err(_(Expr), "Invalid set argument");
        },
    }};
  }
}