#include "unify.hh"

namespace
{
  using namespace rego;

  using FunctionArity = std::shared_ptr<std::map<std::string, std::size_t>>;

  std::string concat(const Node& ref)
  {
    std::ostringstream path;
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
        path << "[" << strip_quotes(to_key(refarg->front())) << "]";
      }
    }

    return path.str();
  }

  bool needs_rewrite(const NodeRange& n, const FunctionArity& func_arity)
  {
    Node ref = n[0]->front();
    assert(ref == Ref);
    Node exprseq = n[1];
    assert(exprseq == ExprSeq);

    std::string path = concat(ref);
    if (!contains(func_arity, path))
    {
      return false;
    }

    bool result = func_arity->at(path) == exprseq->size() - 1;
    return result;
  }
}

namespace rego
{
  const inline auto BoolInfixArg = T(RefTerm) / T(NumTerm) / T(UnaryExpr) /
    T(ArithInfix) / T(Term) / T(ExprCall) / T(BinInfix);

  // Transforms unification expressions into AssignInfix nodes.
  PassDef assign(BuiltIns builtins)
  {
    FunctionArity func_arity =
      std::make_shared<std::map<std::string, std::size_t>>();

    PassDef assign{
      "assign",
      wf_pass_assign,
      dir::bottomup | dir::once,
      {
        T(ExprInfix)
            << (T(Expr)[Lhs] * (T(InfixOperator) << T(AssignOperator)) *
                T(Expr)[Rhs]) >>
          [](Match& _) {
            ACTION();
            return AssignInfix << (AssignArg << _(Lhs))
                               << (AssignArg << _(Rhs));
          },

        In(Literal) *
            (T(Expr)
             << (T(ExprCall)
                 << ((T(RuleRef) << T(Ref)[Ref]) *
                     T(ExprSeq)[ExprSeq])([func_arity](auto& n) {
                      // tests if this is a function call that has an extra
                      // argument that should be unified with the result.
                      return needs_rewrite(n, func_arity);
                    }))) >>
          [](Match& _) {
            ACTION();
            // A convention in the Golang implementation of Rego is
            // that you can call any function with an additional argument
            // that will be unified with its result. This is not documented
            // anywhere but is expected behavior used in tests. This
            // rewrite turns these back into the documented form which we
            // expect.
            Node ref = Ref << (RefHead << (Var ^ concat(_(Ref)))) << RefArgSeq;
            Node exprseq = _(ExprSeq);
            Node var = exprseq->pop_back();
            return Expr
              << (AssignInfix
                  << (AssignArg << var)
                  << (AssignArg
                      << (Expr << (ExprCall << (RuleRef << ref) << exprseq))));
          },

        In(Literal) *
            (T(Expr)
             << (T(AssignInfix)
                 << (T(AssignArg)[Lhs] *
                     (T(AssignArg)
                      << (T(ExprCall)
                          << ((T(RuleRef) << T(Ref)[Ref]) *
                              T(ExprSeq)[ExprSeq] *
                              In(UnifyBody)++)([func_arity](auto& n) {
                               return needs_rewrite(n, func_arity);
                             })))))) >>
          [](Match& _) {
            ACTION();
            // see above comment.
            Node ref = Ref << (RefHead << (Var ^ concat(_(Ref)))) << RefArgSeq;
            Node exprseq = _(ExprSeq);
            Node var = exprseq->pop_back();
            return Seq << (Lift
                           << UnifyBody
                           << (Literal
                               << (Expr
                                   << (AssignInfix
                                       << (AssignArg << var)
                                       << (AssignArg
                                           << (Expr
                                               << (ExprCall << (RuleRef << ref)
                                                            << exprseq)))))))
                       << (Expr
                           << (AssignInfix << _(Lhs)
                                           << (AssignArg << var->clone())));
          },

        In(Literal) * (T(Expr)[Expr] << !T(AssignInfix)) >>
          [](Match& _) {
            ACTION();
            // bare expressions are treated as unification requirements
            // in non-query rules.
            std::string prefix = in_query(_(Expr)) ? "value" : "unify";
            Location temp = _.fresh({prefix});
            return Seq << (Lift << UnifyBody
                                << (Local << (Var ^ temp) << Undefined))
                       << (Expr
                           << (AssignInfix
                               << (AssignArg
                                   << (Expr << (RefTerm << (Var ^ temp))))
                               << (AssignArg << _(Expr))));
          },

        // errors

        In(Expr) * T(Set, SetCompr)[Set] >>
          [](Match& _) {
            ACTION();
            return err(_(Set), "Invalid set argument");
          },

        In(Expr) * T(ExprInfix)[ExprInfix] >>
          [](Match& _) {
            ACTION();
            return err(_(ExprInfix), "Invalid infix expression");
          },
      }};

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

      for (auto& [loc, builtin] : *builtins)
      {
        func_arity->insert({std::string(loc.view()), builtin->arity});
      }

      return 0;
    });

    assign.post([func_arity](Node) {
      func_arity->clear();
      return 0;
    });

    return assign;
  }
}