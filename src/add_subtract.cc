#include "passes.h"

namespace rego
{
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

      In(Expr) * (T(Subtract) * ArithInfixArg[Value] * End) >>
        [](Match& _) { return UnaryExpr << (ArithArg << _(Value)); },

      In(Expr) * (T(UnaryExpr) << (T(UnaryExpr) << Any[Value])) >>
        [](Match& _) { return _(Value); },

      In(ArithArg) *
          (T(Expr)
           << ((T(RefTerm) / T(NumTerm) / T(ArithInfix) / T(UnaryExpr))[Value] *
               End)) >>
        [](Match& _) { return _(Value); },

      // errors

      (In(ArithArg) / In(Expr)) * (T(Add) / T(Subtract))[Op] >>
        [](Match& _) { return err(_(Op), "Invalid operator"); },

      T(ArithArg)[ArithArg] << (Any * Any) >>
        [](Match& _) {
          return err(_(ArithArg), "Argument can only have one element");
        },
    };
  }

}