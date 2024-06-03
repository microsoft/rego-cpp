#include "rego.hh"
#include "unify.hh"

namespace rego
{
  PassDef infix()
  {
    return PassDef{
      "infix",
      wf_pass_infix,
      dir::bottomup | dir::once,
      {
        In(Expr) * (T(ExprParens) << T(Expr)[Expr]) >>
          [](Match& _) { return _(Expr)->front(); },

        T(ExprInfix)
            << (T(Expr)[Lhs] * (T(InfixOperator) << T(ArithOperator)[Op]) *
                T(Expr)[Rhs]) >>
          [](Match& _) {
            return ArithInfix << _(Lhs) << _(Op)->front() << _(Rhs);
          },

        T(ExprInfix)
            << (T(Expr)[Lhs] * (T(InfixOperator) << T(BinOperator)[Op]) *
                T(Expr)[Rhs]) >>
          [](Match& _) {
            return BinInfix << _(Lhs) << _(Op)->front() << _(Rhs);
          },

        T(ExprInfix)
            << (T(Expr)[Lhs] * (T(InfixOperator) << T(BoolOperator)[Op]) *
                T(Expr)[Rhs]) >>
          [](Match& _) {
            return BoolInfix << _(Lhs) << _(Op)->front() << _(Rhs);
          },

        T(Literal) << (T(NotExpr) << T(Expr)[Expr]) >>
          [](Match& _) {
            ACTION();
            return LiteralNot << (UnifyBody << (Literal << _[Expr]));
          },

        // errors

        In(Expr) * T(ExprParens)[ExprParens] >>
          [](Match& _) {
            return err(_(ExprParens), "missing expression", EvalTypeError);
          },
      }};
  }
}