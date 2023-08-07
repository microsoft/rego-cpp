#include "passes.h"

namespace rego
{
  // Transforms addition, subtraction, and unary negation into ArithInfix Nodes.
  PassDef add_subtract()
  {
    return {
      In(Expr) *
          (ArithInfixArg[Lhs] * (T(Add) / T(Subtract))[Op] *
           ArithInfixArg[Rhs]) >>
        [](Match& _) {
          return ArithInfix << (ArithArg << _(Lhs)) << _(Op)
                            << (ArithArg << _(Rhs));
        },

      In(Expr) *
          (ArithInfixArg[Lhs] * (T(Add) / T(Subtract))[Op] * T(Subtract) *
           ArithInfixArg[Rhs]) >>
        [](Match& _) {
          return ArithInfix
            << (ArithArg << _(Lhs)) << _(Op)
            << (ArithArg << (UnaryExpr << (ArithArg << _(Rhs))));
        },

      In(Expr) * (T(Subtract) * ArithInfixArg[Val] * End) >>
        [](Match& _) { return UnaryExpr << (ArithArg << _(Val)); },

      In(Expr) * (T(UnaryExpr) << (T(UnaryExpr) << Any[Val])) >>
        [](Match& _) { return _(Val); },

      In(ArithArg) *
          (T(Expr)
           << ((T(RefTerm) / T(NumTerm) / T(ArithInfix) / T(UnaryExpr) /
                T(ExprCall))[Val] *
               End)) >>
        [](Match& _) { return _(Val); },

      In(Expr) *
          (BinInfixArg[Lhs] * (T(Subtract) / T(Or))[Op] * BinInfixArg[Rhs]) >>
        [](Match& _) {
          return BinInfix << (BinArg << _(Lhs)) << _(Op) << (BinArg << _(Rhs));
        },

      In(BinArg) *
          (T(Expr)
           << ((T(Ref) / T(RefTerm) / T(ExprCall) / T(Set) / T(SetCompr) /
                T(BinInfix))[Val] *
               End)) >>
        [](Match& _) { return _(Val); },

      // errors

      (In(ArithArg) / In(Expr)) * (T(Add) / T(Subtract))[Op] >>
        [](Match& _) { return err(_(Op), "Invalid operator"); },

      (In(BinArg) / In(Expr)) * (T(Or) / T(And) / T(Subtract))[Op] >>
        [](Match& _) { return err(_(Op), "Invalid operator"); },

      T(ArithArg)[ArithArg] << (Any * Any) >>
        [](Match& _) {
          return err(_(ArithArg), "Argument can only have one element");
        },

      T(BinArg)[BinArg] << (Any * Any) >>
        [](Match& _) {
          return err(_(BinArg), "Argument can only have one element");
        },
    };
  }

}