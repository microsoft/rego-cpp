#include "passes.h"

namespace rego
{
  const inline auto NotArg = ArithInfixArg / T(BoolInfix);
  const inline auto BoolInfixArg = T(RefTerm) / T(NumTerm) / T(UnaryExpr) /
    T(ArithInfix) / T(Term) / T(ExprCall) / T(BinInfix);
  const inline auto RefArgBrackArg =
    T(Scalar) / T(Var) / T(Object) / T(Array) / T(Set);

  // Transforms boolean comparison operations into BoolInfix nodes.
  PassDef comparison()
  {
    return {
      In(Expr) * (T(Expr) << (BoolInfixArg[Arg] * End)) >>
        [](Match& _) { return _(Arg); },

      In(Literal) *
          (T(Expr)
           << (BoolInfixArg[Lhs](
                 [](auto& n) { return is_in(*n.first, {UnifyBody}); }) *
               T(Equals) * BoolInfixArg[Rhs])) >>
        [](Match& _) {
          Location unify0 = _.fresh({"unify"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ unify0) << Undefined))
                     << (Lift << UnifyBody
                              << (Literal
                                  << (Expr << (RefTerm << (Var ^ unify0))
                                           << Unify << _(Lhs))))
                     << (Expr << (RefTerm << (Var ^ unify0)) << Unify
                              << _(Rhs));
        },

      In(Expr) * (BoolInfixArg[Lhs] * BoolToken[Op] * BoolInfixArg[Rhs]) >>
        [](Match& _) {
          return BoolInfix << (BoolArg << _(Lhs)) << _(Op)
                           << (BoolArg << _(Rhs));
        },

      In(Expr) *
          (~T(Not)[Not] * BoolInfixArg[Lhs]([](auto& n) {
            return is_in(*n.first, {UnifyBody});
          }) *
           T(MemberOf) * BoolInfixArg[Rhs]) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          Location unify = _.fresh({_(Not) == nullptr ? "unify" : "not"});
          Node seq = Seq
            << (Lift << UnifyBody << (Local << (Var ^ item) << Undefined))
            << (Lift << UnifyBody << (Local << (Var ^ unify) << Undefined))
            << (Lift << UnifyBody
                     << (Literal
                         << (Expr << (RefTerm << (Var ^ item)) << Unify
                                  << (Enumerate << (Expr << _(Rhs))))))
            << (Lift << UnifyBody
                     << (Literal
                         << (Expr
                             << (RefTerm << (Var ^ unify)) << Unify
                             << (RefTerm
                                 << (Ref << (RefHead << (Var ^ item))
                                         << (RefArgSeq
                                             << (RefArgBrack
                                                 << (Scalar
                                                     << (JSONInt ^ "1")))))))))
            << (RefTerm << (Var ^ unify)) << Unify << _(Lhs);

          if (_(Not) != nullptr)
          {
            seq
              << (Lift << UnifyBody
                       << (Literal
                           << (NotExpr
                               << (Expr << (RefTerm << (Var ^ unify))))));
          }

          return seq;
        },

      In(RefArgBrack) * (T(Term) << RefArgBrackArg[Arg]) >>
        [](Match& _) { return _(Arg); },

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

      In(Expr) * T(MemberOf)[MemberOf] >>
        [](Match& _) { return err(_(MemberOf), "Invalid in statement"); },

      In(BoolArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid boolean argument"); },

      In(ArithArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid argument"); },

      In(BinArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid set argument"); },
    };
  }
}