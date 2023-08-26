#include "errors.h"
#include "passes.h"
#include "utils.h"

#include <sstream>

namespace
{
  using namespace rego;

  Location concat(const Node& refterm)
  {
    std::ostringstream path;
    if (refterm->front()->type() == Var)
    {
      path << refterm->front()->location().view();
    }
    else
    {
      Node ref = refterm->front();
      path << (ref / RefHead)->front()->location().view();
      Node refargseq = ref / RefArgSeq;
      for (auto& refarg : *refargseq)
      {
        if (refarg->type() == RefArgDot)
        {
          path << "." << refarg->front()->location().view();
        }
        else if (refarg->type() == RefArgBrack)
        {
          path << "[" << strip_quotes(to_json(refarg->front())) << "]";
        }
      }
    }

    return Location(path.str());
  }

  bool needs_rewrite(
    const NodeRange& n,
    const BuiltIns& builtins,
    const std::shared_ptr<NodeMap<bool>>& cache)
  {
    Node ruleref = n.first[0];
    Node argseq = n.first[1];
    if (cache->contains(ruleref))
    {
      return cache->at(ruleref);
    }

    if (!is_in(ruleref, {UnifyBody}))
    {
      cache->insert({ruleref, false});
      return false;
    }

    Location path = concat(ruleref);
    if (!builtins.is_builtin(path))
    {
      cache->insert({ruleref, false});
      return false;
    }

    auto builtin = builtins.at(path);
    bool result = builtin->arity == argseq->size() - 1;

    cache->insert({ruleref, result});
    return result;
  }
}

namespace rego
{
  const inline auto AssignInfixArg = T(RefTerm) / T(NumTerm) / T(UnaryExpr) /
    T(ArithInfix) / T(BinInfix) / T(Term) / T(BoolInfix) / T(ExprCall) /
    T(Enumerate) / T(Membership);

  // Transforms unification expressions into AssignInfix nodes.
  PassDef assign(const BuiltIns& builtins)
  {
    auto cache = std::make_shared<NodeMap<bool>>();

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

      In(Expr) *
          (AssignInfixArg[Lhs] * T(Unify) * (T(Set) / T(SetCompr))[Rhs]) >>
        [](Match& _) {
          return AssignInfix << (AssignArg << _(Lhs))
                             << (AssignArg << (Term << _(Rhs)));
        },

      In(Expr) *
          ((T(Set) / T(SetCompr))[Lhs] * T(Unify) * AssignInfixArg[Rhs]) >>
        [](Match& _) {
          return AssignInfix << (AssignArg << (Term << _(Lhs)))
                             << (AssignArg << _(Rhs));
        },

      In(Literal) *
          (T(Expr)
           << (T(ExprCall) << (T(RuleRef)[RuleRef] * T(ArgSeq)[ArgSeq])(
                 [builtins, cache](auto& n) {
                   return needs_rewrite(n, builtins, cache);
                 }))) >>
        [builtins](Match& _) {
          Node ruleref = RuleRef << (Var ^ concat(_(RuleRef)));
          Node argseq = _(ArgSeq);
          Node var = argseq->pop_back();
          return Expr
            << (AssignInfix << (AssignArg << var->front())
                            << (AssignArg << (ExprCall << ruleref << argseq)));
        },

      In(LiteralNot) *
          (T(Expr)
           << (T(ExprCall) << (T(RuleRef)[RuleRef] * T(ArgSeq)[ArgSeq])(
                 [builtins, cache](auto& n) {
                   return needs_rewrite(n, builtins, cache);
                 }))) >>
        [builtins](Match& _) {
          Node ruleref = RuleRef << (Var ^ concat(_(RuleRef)));
          Node argseq = _(ArgSeq);
          Node var = argseq->pop_back();
          return Seq << (Lift
                         << UnifyBody
                         << (Literal
                             << (Expr
                                 << (AssignInfix
                                     << (AssignArg << var->front())
                                     << (AssignArg
                                         << (ExprCall << ruleref << argseq))))))
                     << var->clone();
        },

      In(Literal) * (T(Expr) << (AssignInfixArg[Arg] * End)) >>
        [](Match& _) {
          std::string prefix = in_query(_(Arg)) ? "value" : "unify";
          Location temp = _.fresh({prefix});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ temp) << Undefined))
                     << (Expr
                         << (AssignInfix
                             << (AssignArg << (RefTerm << (Var ^ temp)))
                             << (AssignArg << _(Arg))));
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

      In(Expr) * (T(Set) / T(SetCompr))[Set] >>
        [](Match& _) { return err(_(Set), "Invalid set in expression"); },
    };
  }
}