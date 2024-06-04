#include "rego.hh"
#include "trieste/ast.h"
#include "unify.hh"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  void find_assigned_vars(const Node& node, Nodes& vars)
  {
    if (node->type() == LiteralWith || node->type() == SomeDecl)
    {
      return;
    }

    if (node->type() == Var)
    {
      Nodes defs = node->lookup();
      if (defs.size() == 0)
      {
        vars.push_back(node->clone());
      }
    }
    else
    {
      for (auto& child : *node)
      {
        find_assigned_vars(child, vars);
      }
    }
  }

  const inline auto LocalToken = T(RefTerm) / T(Term);
}

namespace rego
{
  // Process most nodes that will be referenced by symbols so that they can be
  // appropriately indexed. Also processes some nodes into forms that will
  // be easier to process for later passes.
  PassDef symbols()
  {
    return {
      "symbols",
      wf_pass_symbols,
      dir::topdown,
      {
        In(Query) * (T(Literal)[Head] * T(Literal)++[Tail]) >>
          [](Match& _) {
            ACTION();
            return UnifyBody << _(Head) << _[Tail];
          },

        In(Module) * (T(ImportSeq)[ImportSeq] * T(Policy)[Policy]) >>
          [](Match& _) {
            ACTION();
            return Policy << *_[ImportSeq] << *_[Policy];
          },

        T(Literal)
            << (T(SomeDecl, Expr, NotExpr)[Expr] * (T(WithSeq) << End)) >>
          [](Match& _) {
            ACTION();
            return Literal << _(Expr);
          },

        T(Literal)
            << (T(SomeDecl, Expr, NotExpr)[Expr] * T(WithSeq)[WithSeq]) >>
          [](Match& _) {
            ACTION();
            return LiteralWith << (UnifyBody << (Literal << _(Expr)))
                               << _(WithSeq);
          },

        In(With) * (T(Term) << T(Var, Ref)[RuleRef]) >>
          [](Match& _) { return RuleRef << _(RuleRef); },

        In(ExprCall) * T(Ref)[Ref] >>
          [](Match& _) {
            ACTION();
            return RuleRef << _(Ref);
          },

        In(Policy) *
            (T(Rule)
             << (T(False) *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Var)[Id]) *
                      (T(RuleHeadComp) << T(Expr)[Expr]))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            ACTION();
            std::size_t num_items = _(RuleBodySeq)->size();

            Node val = _(Expr);
            if (is_constant(val->front()))
            {
              val = val->front();
            }

            if (num_items == 0)
            {
              return RuleComp << _(Id) << Empty << val << (Int ^ "0");
            }

            if (num_items == 1)
            {
              Node item = _(RuleBodySeq)->front();
              Node body = UnifyBody ^ item;
              body->insert(body->end(), item->begin(), item->end());
              return RuleComp << _(Id) << body << val << (Int ^ "0");
            }

            // Else precedence is per rule chain. This means that
            // each set of else sequences must be evaluated individually
            // and resolve to a value (or undefined). This is to deal
            // with rules of the following kind:
            //   conflict_1 {
            //     false
            //   } else = true {
            //     true
            //   }
            //
            //   conflict_1 = false
            //
            // In this case, conflict_1 will resolve to two (conflicting)
            // values. This is why we have to create a proxy rule below.
            // Do NOT optimise this away.
            Node rule_seq = NodeDef::create(Seq);

            // first we create all the variants of the rule
            Node rule_id = Var ^ _.fresh(_(Id)->location());
            for (std::size_t i = 0; i < num_items; ++i)
            {
              Node item = _(RuleBodySeq)->at(i);
              Node body;
              if (item == Query)
              {
                body = UnifyBody ^ item;
                body->insert(body->end(), item->begin(), item->end());
                rule_seq
                  << (RuleComp << rule_id->clone() << body << val->clone()
                               << (Int ^ std::to_string(i)));
              }
              else if (item == Else)
              {
                Node item_val = item / Expr;
                if (is_constant(item_val->front()))
                {
                  item_val = item_val->front();
                }

                Node query = item / Query;
                body = UnifyBody ^ query;
                body->insert(body->end(), query->begin(), query->end());
                rule_seq
                  << (RuleComp << rule_id->clone() << body << item_val
                               << (Int ^ std::to_string(i)));
              }
            }

            // then we create the proxy rule
            rule_seq
              << (RuleComp << _(Id) << Empty
                           << (Expr << (Term << (Var ^ rule_id)))
                           << (Int ^ "0"));

            return rule_seq;
          },

        In(Policy) *
            (T(Rule)
             << (T(True) *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Var)[Id]) *
                      (T(RuleHeadComp) << T(Expr)[Expr]))) *
                 (T(RuleBodySeq) << End))) >>
          [](Match& _) {
            ACTION();
            std::size_t rank = std::numeric_limits<std::uint16_t>::max();
            return RuleComp << _(Id) << Empty << _(Expr)
                            << (Int ^ std::to_string(rank));
          },

        In(Policy) *
            (T(Rule)
             << (T(False) *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Var)[Id]) *
                      (T(RuleHeadFunc)
                       << (T(RuleArgs)[RuleArgs] * T(Expr)[Expr])))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            ACTION();
            std::size_t num_items = _(RuleBodySeq)->size();

            Node val = _(Expr);
            if (is_constant(val->front()))
            {
              val = val->front();
            }

            if (num_items == 0)
            {
              return RuleFunc << _(Id) << _(RuleArgs) << Empty << val
                              << (Int ^ "0");
            }

            if (num_items == 1)
            {
              Node item = _(RuleBodySeq)->front();
              Node body = UnifyBody ^ item;
              body->insert(body->end(), item->begin(), item->end());
              return RuleFunc << _(Id) << _(RuleArgs) << body << val
                              << (Int ^ "0");
            }

            // See above comment. Do not optimise this away.
            Node rule_seq = NodeDef::create(Seq);

            // create rule variants
            Node rule_id = Var ^ _.fresh(_(Id)->location());
            for (std::size_t i = 0; i < num_items; ++i)
            {
              Node item = _(RuleBodySeq)->at(i);
              Node body;
              if (item == Query)
              {
                body = UnifyBody ^ item;
                body->insert(body->end(), item->begin(), item->end());
                rule_seq
                  << (RuleFunc << rule_id->clone() << _(RuleArgs)->clone()
                               << body << val->clone()
                               << (Int ^ std::to_string(i)));
              }
              else if (item == Else)
              {
                Node item_val = item / Expr;
                if (is_constant(item_val->front()))
                {
                  item_val = item_val->front();
                }

                Node query = item / Query;
                body = UnifyBody ^ query;
                body->insert(body->end(), query->begin(), query->end());
                rule_seq
                  << (RuleFunc << rule_id->clone() << _(RuleArgs)->clone()
                               << body << item_val
                               << (Int ^ std::to_string(i)));
              }
              else if (item == Literal)
              {
                body = UnifyBody << item;
                rule_seq
                  << (RuleFunc << rule_id->clone() << _(RuleArgs)->clone()
                               << body << val->clone()
                               << (Int ^ std::to_string(i)));
              }
            }

            // create proxy rule
            // need to be careful here, as we need to pass through the rule args
            Node exprseq = NodeDef::create(ExprSeq);
            for (auto& arg : *_(RuleArgs))
            {
              if (arg == ArgVar)
              {
                exprseq->push_back(Expr << (Term << (arg / Var)->clone()));
              }
              else if (arg == ArgVal)
              {
                exprseq->push_back(Expr << (Term << arg->front()->clone()));
              }
              else
              {
                exprseq->push_back(err(arg, "Invalid rule argument"));
              }
            }

            Location out = _.fresh({"out"});
            Node proxy_body = UnifyBody
              << (Literal
                  << (Expr
                      << (ExprInfix
                          << (Expr << (Term << (Var ^ out)))
                          << (InfixOperator << (AssignOperator << Assign))
                          << (Expr
                              << (ExprCall
                                  << (Ref << (RefHead << rule_id) << RefArgSeq)
                                  << exprseq)))));
            Node proxy_val = Expr << (Term << (Var ^ out));
            rule_seq
              << (RuleFunc << _(Id) << _(RuleArgs) << proxy_body << proxy_val
                           << (Int ^ "0"));

            return rule_seq;
          },

        In(Policy) *
            (T(Rule)
             << (T(True) *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Var)[Id]) *
                      (T(RuleHeadFunc)
                       << (T(RuleArgs)[RuleArgs] * T(Expr)[Expr])))) *
                 (T(RuleBodySeq) << End))) >>
          [](Match& _) {
            ACTION();
            std::size_t rank = std::numeric_limits<std::uint16_t>::max();
            return RuleFunc << _(Id) << _(RuleArgs) << Empty << _(Expr)
                            << (Int ^ std::to_string(rank));
          },

        In(Policy) *
            (T(Rule)
             << (T(False) *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Var)[Id]) *
                      (T(RuleHeadSet) << T(Expr)[Expr]))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            ACTION();
            std::size_t num_items = _(RuleBodySeq)->size();

            Node val = _(Expr);
            if (is_constant(val->front()))
            {
              val = val->front();
            }

            if (num_items == 0)
            {
              return RuleSet << _(Id) << Empty << val << (Int ^ "0");
            }

            Node rule_seq = NodeDef::create(Seq);

            for (std::size_t i = 0; i < num_items; ++i)
            {
              Node item = _(RuleBodySeq)->at(i);
              Node body;
              if (item == Query)
              {
                body = UnifyBody ^ item;
                body->insert(body->end(), item->begin(), item->end());
                rule_seq
                  << (RuleSet << _(Id)->clone() << body << val->clone()
                              << (Int ^ std::to_string(i)));
              }
              else if (item == Else)
              {
                return err(
                  item, "else keyword cannot be used on multi-value rules");
              }
              else if (item == Literal)
              {
                body = UnifyBody << item;
                rule_seq
                  << (RuleSet << _(Id)->clone() << body << val->clone()
                              << (Int ^ std::to_string(i)));
              }
            }

            return rule_seq;
          },

        In(Policy) *
            (T(Rule)
             << (T(False) *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Var)[Id]) *
                      (T(RuleHeadObj) << (T(Expr)[Key] * T(Expr)[Val])))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            ACTION();
            std::size_t num_items = _(RuleBodySeq)->size();

            Node key = _(Key);
            if (is_constant(key->front()))
            {
              key = key->front();
            }
            Node val = _(Val);
            if (is_constant(val->front()))
            {
              val = val->front();
            }

            if (num_items == 0)
            {
              return RuleObj << _(Id) << Empty << key << val << (Int ^ "0");
            }

            Node rule_seq = NodeDef::create(Seq);

            for (std::size_t i = 0; i < num_items; ++i)
            {
              Node item = _(RuleBodySeq)->at(i);
              Node body;
              if (item == Query)
              {
                body = UnifyBody ^ item;
                body->insert(body->end(), item->begin(), item->end());
                rule_seq
                  << (RuleObj << _(Id)->clone() << body << key->clone()
                              << val->clone() << (Int ^ std::to_string(i)));
              }
              else if (item == Else)
              {
                return err(
                  item, "else keyword cannot be used on multi-value rules");
              }
              else if (item == Literal)
              {
                body = UnifyBody << item;
                rule_seq
                  << (RuleObj << _(Id)->clone() << body << key->clone()
                              << val->clone() << (Int ^ std::to_string(i)));
              }
            }

            return rule_seq;
          },

        In(Expr) * (T(Term) << (T(Ref) / T(Var))[Val]) >>
          [](Match& _) {
            ACTION();
            return RefTerm << _(Val);
          },

        In(Expr) * (T(Term) << (T(Scalar) << (T(Int) / T(Float))[Val])) >>
          [](Match& _) {
            ACTION();
            return NumTerm << _(Val);
          },

        In(RefArgBrack) * T(Var)[Var] >>
          [](Match& _) {
            ACTION();
            return RefTerm << _(Var);
          },

        In(RefArgBrack) * T(ExprInfix)[ExprInfix] >>
          [](Match& _) {
            ACTION();
            return Expr << _(ExprInfix);
          },

        In(UnifyBody) *
            (T(Literal)
             << (T(SomeDecl)
                 << ((T(ExprSeq)[ExprSeq]
                      << ((T(Expr) << (T(Term) << T(Var)))++ * End)) *
                     T(Undefined)))) >>
          [](Match& _) {
            ACTION();
            Node seq = NodeDef::create(Seq);
            for (auto& var : *_(ExprSeq))
            {
              seq->push_back(Local << var->front()->front() << Undefined);
            }
            return seq;
          },

        In(UnifyBody) *
            (T(Literal)
             << (T(SomeDecl)
                 << ((T(ExprSeq) << (T(Expr)[Item] * End)) *
                     T(Expr)[ItemSeq]))) >>
          [](Match& _) {
            ACTION();
            Location item = _.fresh({"item"});
            return Seq << (Local << (Var ^ item) << Undefined)
                       << (LiteralEnum << (Var ^ item) << _(ItemSeq))
                       << (Literal
                           << (Expr << expr_infix(
                                 Unify,
                                 _(Item),
                                 (RefTerm
                                  << (Ref
                                      << (RefHead << (Var ^ item))
                                      << (RefArgSeq
                                          << (RefArgBrack
                                              << (Scalar << (Int ^ "1")))))))));
          },

        In(UnifyBody) *
            (T(Literal)
             << (T(SomeDecl)
                 << ((T(ExprSeq) << (T(Expr)[Idx] * T(Expr)[Item] * End)) *
                     T(Expr)[ItemSeq]))) >>
          [](Match& _) {
            ACTION();
            Location item = _.fresh({"item"});
            return Seq << (Local << (Var ^ item) << Undefined)
                       << (LiteralEnum << (Var ^ item) << _(ItemSeq))
                       << (Literal
                           << (Expr << expr_infix(
                                 Unify,
                                 _(Idx),
                                 (RefTerm
                                  << (Ref
                                      << (RefHead << (Var ^ item))
                                      << (RefArgSeq
                                          << (RefArgBrack
                                              << (Scalar << (Int ^ "0")))))))))
                       << (Literal
                           << (Expr << expr_infix(
                                 Unify,
                                 _(Item),
                                 (RefTerm
                                  << (Ref
                                      << (RefHead << (Var ^ item))
                                      << (RefArgSeq
                                          << (RefArgBrack
                                              << (Scalar << (Int ^ "1")))))))));
          },

        In(UnifyBody, LiteralWith) *
            (T(Literal)
             << (T(Expr)
                 << (T(ExprInfix)
                     << (T(Expr)[Lhs] *
                         (T(InfixOperator)
                          << (T(AssignOperator) << T(Assign))) *
                         T(Expr)[Rhs])))) >>
          [](Match& _) {
            ACTION();
            // this is now an infinite loop
            Node seq = NodeDef::create(Seq);
            Nodes vars;
            find_assigned_vars(_(Lhs), vars);

            for (auto& var : vars)
            {
              seq->push_back(Lift << UnifyBody << (Local << var << Undefined));
            }

            seq << (Literal << (Expr << expr_infix(Unify, _(Lhs), _(Rhs))));

            return seq;
          },

        In(RuleComp, RuleFunc) * (T(Empty) * T(Expr)[Expr]) >>
          [](Match& _) {
            ACTION();
            Location out = _.fresh({"out"});
            Location value = _.fresh({"value"});
            return Seq
              << (UnifyBody
                  << (Literal
                      << (Expr << expr_infix(
                            Unify, (RefTerm << (Var ^ out)), _(Expr)))))
              << (UnifyBody
                  << (Literal
                      << (Expr << expr_infix(
                            Unify,
                            (RefTerm << (Var ^ value)),
                            (RefTerm << (Var ^ out))))));
          },

        In(RuleComp, RuleFunc) * T(Expr)[Expr] >>
          [](Match& _) {
            ACTION();
            Location value = _.fresh({"value"});
            return UnifyBody
              << (Literal
                  << (Expr << expr_infix(
                        Unify, (RefTerm << (Var ^ value)), _(Expr))));
          },

        T(ArrayCompr) << (T(Expr)[Val] * T(Query)[Query]) >>
          [](Match& _) {
            ACTION();
            Node compr = ArrayCompr << _(Val);
            Node body = UnifyBody ^ _(Query);
            body->insert(body->end(), _(Query)->begin(), _(Query)->end());
            return compr
              << (NestedBody << (Key ^ _.fresh({"arraycompr"})) << body);
          },

        T(SetCompr) << (T(Expr)[Val] * T(Query)[Query]) >>
          [](Match& _) {
            ACTION();
            Node compr = SetCompr << _(Val);
            Node body = UnifyBody ^ _(Query);
            body->insert(body->end(), _(Query)->begin(), _(Query)->end());
            return compr
              << (NestedBody << (Key ^ _.fresh({"setcompr"})) << body);
          },

        T(ObjectCompr) << (T(Expr)[Key] * T(Expr)[Val] * T(Query)[Query]) >>
          [](Match& _) {
            ACTION();
            Node compr = ObjectCompr << _(Key) << _(Val);
            Node body = UnifyBody ^ _(Query);
            body->insert(body->end(), _(Query)->begin(), _(Query)->end());
            return compr
              << (NestedBody << (Key ^ _.fresh({"objectcompr"})) << body);
          },

        In(Expr) *
            ((T(ExprEvery) * In(UnifyBody)++)
             << ((T(VarSeq) << (T(Var)[Val] * End)) *
                 T(Term, ExprCall, ExprInfix)[IsIn] * T(Query)[Query])) >>
          [](Match& _) {
            ACTION();
            Location item = _.fresh({"item"});
            Location itemseq = _.fresh({"itemseq"});
            Node query =
              Query << (Local << (Var ^ item) << Undefined)
                    << (LiteralEnum << (Var ^ item)
                                    << (Expr << (RefTerm << (Var ^ itemseq))))
                    << (Literal
                        << (Expr << expr_infix(
                              Unify,
                              (RefTerm << _(Val)->clone()),
                              (RefTerm
                               << (Ref << (RefHead << (Var ^ item))
                                       << (RefArgSeq
                                           << (RefArgBrack
                                               << (Scalar << (Int ^ "1")))))))))
                    << *_[Query];

            return Seq
              << (Lift << UnifyBody << (Local << (Var ^ itemseq) << Undefined))
              << (Lift
                  << UnifyBody
                  << (Literal
                      << (Expr << expr_infix(
                            Unify, (RefTerm << (Var ^ itemseq)), _(IsIn)))))
              << expr_infix(
                     Equals,
                     ExprCall
                       << (Ref << (RefHead << (Var ^ "count")) << RefArgSeq)
                       << (ExprSeq
                           << (Expr
                               << (Term
                                   << (SetCompr
                                       << (Expr << (RefTerm << _(Val)->clone()))
                                       << query)))),
                     ExprCall
                       << (Ref << (RefHead << (Var ^ "count")) << RefArgSeq)
                       << (ExprSeq
                           << (Expr
                               << (ExprCall
                                   << (Ref << (RefHead << (Var ^ "cast_set"))
                                           << RefArgSeq)
                                   << (ExprSeq
                                       << (Expr
                                           << (RefTerm
                                               << (Var ^ itemseq))))))));
          },

        In(Expr) *
            ((T(ExprEvery) * In(UnifyBody)++)
             << ((T(VarSeq) << (T(Var)[Idx] * T(Var)[Val] * End)) *
                 T(Term, ExprCall, ExprInfix)[IsIn] * T(Query)[Query])) >>
          [](Match& _) {
            ACTION();
            Location item = _.fresh({"item"});
            Location itemseq = _.fresh({"itemseq"});
            Node query =
              Query << (Local << (Var ^ item) << Undefined)
                    << (LiteralEnum << (Var ^ item)
                                    << (Expr << (RefTerm << (Var ^ itemseq))))
                    << (Literal
                        << (Expr << expr_infix(
                              Unify,
                              (RefTerm << _(Idx)->clone()),
                              (RefTerm
                               << (Ref << (RefHead << (Var ^ item))
                                       << (RefArgSeq
                                           << (RefArgBrack
                                               << (Scalar << (Int ^ "0")))))))))
                    << (Literal
                        << (Expr << expr_infix(
                              Unify,
                              (RefTerm << _(Val)->clone()),
                              (RefTerm
                               << (Ref << (RefHead << (Var ^ item))
                                       << (RefArgSeq
                                           << (RefArgBrack
                                               << (Scalar << (Int ^ "1")))))))))
                    << *_[Query];

            return Seq
              << (Lift << UnifyBody << (Local << (Var ^ itemseq) << Undefined))
              << (Lift
                  << UnifyBody
                  << (Literal
                      << (Expr << expr_infix(
                            Unify, (RefTerm << (Var ^ itemseq)), _(IsIn)))))
              << expr_infix(
                     Equals,
                     ExprCall
                       << (Ref << (RefHead << (Var ^ "count")) << RefArgSeq)
                       << (ExprSeq
                           << (Expr
                               << (Term
                                   << (ArrayCompr
                                       << (Expr << (RefTerm << _(Val)->clone()))
                                       << query)))),
                     ExprCall
                       << (Ref << (RefHead << (Var ^ "count")) << RefArgSeq)
                       << (ExprSeq << (Expr << (RefTerm << (Var ^ itemseq)))));
          },

        In(RuleComp, RuleFunc, RuleObj, RuleSet) * (T(Term) << T(Ref)[Ref]) >>
          [](Match& _) {
            ACTION();
            return UnifyBody << (Literal << (Expr << (RefTerm << _(Ref))));
          },

        T(Placeholder)[Placeholder] * In(UnifyBody)++ >>
          [](Match& _) {
            ACTION();
            auto name = _.fresh({"_"});
            return Seq << (Lift << UnifyBody
                                << (Local << (Var ^ name) << Undefined))
                       << (Var ^ name);
          },

        // errors

        In(Policy) * T(Rule)[Rule] >>
          [](Match& _) {
            ACTION();
            return err(_(Rule), "Invalid rule");
          },

        In(UnifyBody) * (T(RuleHead)[RuleHead] << (T(Var) * T(RuleHeadFunc))) >>
          [](Match& _) {
            ACTION();
            return err(_(RuleHead), "No rule functions allowed in rule bodies");
          },

        In(Term) * T(Ref)[Ref] >>
          [](Match& _) {
            ACTION();
            return err(_(Ref), "Invalid ref term");
          },

        In(Term) * T(Var)[Var] >>
          [](Match& _) {
            ACTION();
            return err(_(Var), "Invalid var term");
          },

        In(Literal) * T(SomeDecl)[SomeDecl] >>
          [](Match& _) {
            ACTION();
            return err(_(SomeDecl), "Invalid some declaration");
          },

        In(Expr) * T(Assign)[Assign] >>
          [](Match& _) {
            ACTION();
            return err(_(Assign), "Invalid assignment");
          },

        T(UnifyBody)[UnifyBody] << End >>
          [](Match& _) {
            ACTION();
            return err(_(UnifyBody), "Invalid unification body");
          },

        In(Expr) * T(Dot)[Dot] >>
          [](Match& _) {
            ACTION();
            return err(_(Dot), "Invalid dot expression");
          },

        In(Expr) * T(ExprEvery)[ExprEvery] >>
          [](Match& _) {
            ACTION();
            return err(_(ExprEvery), "Invalid every expression");
          },

        In(With) * T(Term)[Term] >>
          [](Match& _) {
            ACTION();
            return err(_(Term), "Invalid with term");
          },
      }};
  }

}
