#include "lang.h"
#include "passes.h"
#include "resolver.h"

#include <sstream>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  inline const auto wfi =
      (Top <<= Rego)
    | (NumTerm <<= JSONInt | JSONFloat)
    ;
  // clang-format on
}

namespace rego
{
  const inline auto AssignInfixArg = T(RefTerm) / T(NumTerm) / T(UnaryExpr) /
    T(ArithInfix) / T(Term) / T(BoolInfix) / T(Enumerate);

  PassDef assign()
  {
    return {
      (In(RuleComp) / In(RuleFunc) / In(RuleObj) / In(RuleSet)) *
          (T(Expr)
           << (T(Term)[Term]([](auto& n) { return !contains_ref(*n.first); }) *
               End)) >>
        [](Match& _) { return _(Term); },

      (In(RuleComp) / In(RuleFunc) / In(RuleObj) / In(RuleSet)) *
          (T(Expr) << (T(NumTerm)[NumTerm] * End)) >>
        [](Match& _) {
          Node number = _(NumTerm)->at(wfi / NumTerm / NumTerm);
          return Term << (Scalar << number);
        },

      (In(RuleComp) / In(RuleFunc) / In(RuleObj) / In(RuleSet)) *
          (T(Expr)
           << (T(UnaryExpr) << (T(ArithArg) << (T(NumTerm)[NumTerm])) * End)) >>
        [](Match& _) {
          Node number =
            Resolver::negate(_(NumTerm)->at(wfi / NumTerm / NumTerm));
          return Term << (Scalar << number);
        },

      (In(RuleComp) / In(RuleFunc) / In(RuleObj) / In(RuleSet)) *
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

      In(Expr) * T(Enumerate)[Enumerate] >>
        [](Match& _) { return err(_(Enumerate), "Invalid enumerate"); },

      (In(RuleComp) / In(RuleFunc) / In(RuleSet) / In(RuleObj)) *
          T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid rule value"); },
    };
  }
}