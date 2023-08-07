#include "lang.h"
#include "passes.h"

#include <sstream>

namespace rego
{
  const inline auto AssignInfixArg = T(RefTerm) / T(NumTerm) / T(UnaryExpr) /
    T(ArithInfix) / T(BinInfix) / T(Term) / T(BoolInfix) / T(ExprCall) /
    T(Enumerate);

  // Transforms unification expressions into AssignInfix nodes.
  PassDef assign()
  {
    return {
      In(Expr) *
          (AssignInfixArg[Head](
             [](auto& n) { return is_in(*n.first, {UnifyBody}); }) *
           T(Unify) * AssignInfixArg[Lhs] * T(Unify) * AssignInfixArg[Rhs] *
           End) >>
        [](Match& _) {
          return Seq << (Lift
                         << UnifyBody
                         << (Literal
                             << (Expr
                                 << (AssignInfix << (AssignArg << _(Lhs))
                                                 << (AssignArg << _(Rhs))))))
                     << (AssignInfix << (AssignArg << _(Head))
                                     << (AssignArg << _(Lhs)->clone()));
        },

      In(Expr) * (AssignInfixArg[Lhs] * T(Unify) * AssignInfixArg[Rhs]) >>
        [](Match& _) {
          return AssignInfix << (AssignArg << _(Lhs)) << (AssignArg << _(Rhs));
        },

      In(UnifyBody) *
          (T(Literal) << (T(Expr) << (AssignInfixArg[Arg] * End))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          std::string prefix = in_query(_(Arg)) ? "value" : "unify";
          Location temp = _.fresh({prefix});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          seq->push_back(
            Literal
            << (Expr
                << (AssignInfix << (AssignArg << (RefTerm << (Var ^ temp)))
                                << (AssignArg << _(Arg)))));
          return seq;
        },

      // errors
      In(Expr) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Syntax error"); },

      In(AssignArg) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid assignment argument"); },

      In(Expr) * T(Unify)[Unify] >>
        [](Match& _) { return err(_(Unify), "Invalid assignment"); },

      (In(RuleComp) / In(RuleFunc) / In(RuleSet) / In(RuleObj)) *
          T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid rule value"); },

      In(Expr) * T(Enumerate)[Enumerate] >>
        [](Match& _) { return err(_(Enumerate), "Invalid enumeration"); },
    };
  }
}