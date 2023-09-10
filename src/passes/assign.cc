#include "internal.h"

namespace
{
  using namespace rego;

  using FunctionArity = std::shared_ptr<std::map<std::string, std::size_t>>;

  std::string concat(const Node& refterm)
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

    return path.str();
  }

  bool needs_rewrite(
    const NodeRange& n,
    const FunctionArity& func_arity,
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

    std::string path = concat(ruleref);
    if (!func_arity->contains(path))
    {
      cache->insert({ruleref, false});
      return false;
    }

    bool result = func_arity->at(path) == argseq->size() - 1;

    cache->insert({ruleref, result});
    return result;
  }
}

namespace rego
{
  const inline auto AssignInfixArg = T(RefTerm) / T(NumTerm) / T(UnaryExpr) /
    T(ArithInfix) / T(BinInfix) / T(Term) / T(BoolInfix) / T(ExprCall) /
    T(Membership);

  // Transforms unification expressions into AssignInfix nodes.
  PassDef assign(const BuiltIns& builtins)
  {
    auto cache = std::make_shared<NodeMap<bool>>();
    FunctionArity func_arity =
      std::make_shared<std::map<std::string, std::size_t>>();

    PassDef assign = {
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
                 [func_arity, cache](auto& n) {
                   // tests if this is a function call that has an extra
                   // argument that should be unified with the result.
                   return needs_rewrite(n, func_arity, cache);
                 }))) >>
        [](Match& _) {
          // A convention in the Golang implementation of Rego is
          // that you can call any function with an additional argument
          // that will be unified with its result. This is not documented
          // anywhere but is expected behavior used in tests. This
          // rewrite turns these back into the documented form which we
          // expect.
          Node ruleref = RuleRef << (Var ^ concat(_(RuleRef)));
          Node argseq = _(ArgSeq);
          Node var = argseq->pop_back();
          return Expr
            << (AssignInfix << (AssignArg << var->front())
                            << (AssignArg << (ExprCall << ruleref << argseq)));
        },

      In(Literal) *
          (T(Expr)
           << (T(AssignInfix)
               << (T(AssignArg)[Lhs] *
                   (T(AssignArg)
                    << (T(ExprCall)
                        << (T(RuleRef)[RuleRef] *
                            T(ArgSeq)[ArgSeq])([func_arity, cache](auto& n) {
                             return needs_rewrite(n, func_arity, cache) &&
                               is_in(*n.first, {UnifyBody});
                           })))))) >>
        [](Match& _) {
          // see above comment.
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
                     << (Expr
                         << (AssignInfix
                             << _(Lhs)
                             << (AssignArg << var->front()->clone())));
        },

      In(Literal) * (T(Expr) << (AssignInfixArg[Arg] * End)) >>
        [](Match& _) {
          // bare expressions are treated as unification requirements
          // in non-query rules.
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

      In(RuleComp, RuleFunc, RuleSet, RuleObj) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid rule value"); },

      In(Expr) * (T(Set) / T(SetCompr))[Set] >>
        [](Match& _) { return err(_(Set), "Invalid set in expression"); },
    };

    assign.pre(Rego, [func_arity, builtins](Node node) {
      if (!func_arity->empty())
      {
        return 0;
      }

      Nodes rules;
      node->get_symbols(rules, [](Node n) { return n->type() == RuleFunc; });
      for (auto rule : rules)
      {
        std::string path = std::string((rule / Var)->location().view());
        std::size_t arity = (rule / RuleArgs)->size();
        func_arity->insert({path, arity});
      }

      for (auto& [loc, builtin] : builtins)
      {
        func_arity->insert({std::string(loc.view()), builtin->arity});
      }

      return 0;
    });

    return assign;
  }
}