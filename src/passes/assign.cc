#include "passes.h"

#include <sstream>

namespace rego
{
  const inline auto AssignInfixArg = ArithInfixArg / T(Term) / T(BoolInfix);

  PassDef assign()
  {
    return {
      (In(RuleComp) / In(RuleFunc)) *
          (T(Expr)
           << (T(Term)[Term]([](auto& n) { return !contains_ref(*n.first); }) *
               End)) >>
        [](Match& _) { return _(Term); },

      (In(RuleComp) / In(RuleFunc)) *
          (T(Expr) << (AssignInfixArg[Arg] * End)) >>
        [](Match& _) {
          Location value = _.fresh({"value"});
          return UnifyBody << (Local << (Var ^ value) << Undefined)
                           << (Literal
                               << (Expr
                                   << (AssignInfix
                                       << (AssignArg
                                           << (RefTerm << (Var ^ value)))
                                       << (AssignArg << _(Arg)))));
        },

      In(Expr) * (AssignInfixArg[Lhs] * T(Unify) * AssignInfixArg[Rhs]) >>
        [](Match& _) {
          return AssignInfix << (AssignArg << _(Lhs)) << (AssignArg << _(Rhs));
        },

      In(UnifyBody) * T(Literal) << (T(Expr) << (AssignInfixArg[Arg] * End)) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          std::string prefix = is_in(_(Arg), Query) ? "value" : "unify";
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

      (In(RuleComp) / In(RuleFunc)) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid rule value"); },
    };
  }
}