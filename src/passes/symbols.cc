#include "errors.h"
#include "helpers.h"
#include "passes.h"
#include "resolver.h"

#include <sstream>

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
  // appropriately indexed.
  PassDef symbols()
  {
    return {
      In(Query) * (T(Literal)[Head] * T(Literal)++[Tail]) >>
        [](Match& _) { return UnifyBody << _(Head) << _[Tail]; },

      In(ModuleSeq) *
          (T(Module)
           << (T(Package)[Package] * T(ImportSeq)[ImportSeq] *
               T(Policy)[Policy])) >>
        [](Match& _) {
          Node policy = NodeDef::create(Policy);
          for (auto& import : *_(ImportSeq))
          {
            if (import->type() == Import)
            {
              policy->push_back(import);
            }
          }
          policy->insert(policy->end(), _(Policy)->begin(), _(Policy)->end());
          return Module << _(Package) << policy;
        },

      In(Policy) *
          (T(Rule)
           << (T(JSONFalse) *
               (T(RuleHead)
                << ((T(RuleRef) << T(Var)[Id]) *
                    (T(RuleHeadComp) << (T(AssignOperator) * T(Expr)[Expr])))) *
               (T(Empty) / T(UnifyBody))[Body] * T(ElseSeq)[ElseSeq])) >>
        [](Match& _) {
          Node elseseq = _(ElseSeq);
          if (elseseq->size() == 0)
          {
            return RuleComp << _(Id) << _(Body) << _(Expr) << (JSONInt ^ "0");
          }

          // we need to create sub-rules for each possibility
          Location rulename = _(Id)->location();
          Location subrule = _.fresh(rulename);
          Node seq = NodeDef::create(Seq);
          seq
            << (RuleComp << (Var ^ subrule) << _(Body) << _(Expr)
                         << (JSONInt ^ "0"));
          for (std::size_t i = 0; i < elseseq->size(); ++i)
          {
            Node expr = elseseq->at(i) / Val;
            Node body = elseseq->at(i) / Body;
            seq
              << (RuleComp << (Var ^ subrule) << body << expr
                           << (JSONInt ^ std::to_string(i + 1)));
          }

          Location value = _.fresh({"value"});
          return seq
            << (RuleComp << _(Id) << Empty
                         << (UnifyBody
                             << (Literal
                                 << (Expr << (RefTerm << (Var ^ value)) << Unify
                                          << (RefTerm << (Var ^ subrule)))))
                         << (JSONInt ^ "0"));
        },

      In(Policy) *
          (T(Rule)
           << (T(JSONTrue) *
               (T(RuleHead)
                << ((T(RuleRef) << T(Var)[Id]) *
                    (T(RuleHeadComp) << (T(AssignOperator) * T(Expr)[Expr])))) *
               T(Empty) * (T(ElseSeq) << End))) >>
        [](Match& _) {
          std::size_t rank = std::numeric_limits<std::size_t>::max();
          return RuleComp << _(Id) << Empty << _(Expr)
                          << (JSONInt ^ std::to_string(rank));
        },

      In(Policy) *
          (T(Rule)
           << (T(JSONFalse) *
               (T(RuleHead)
                << ((T(RuleRef) << T(Var)[Id]) *
                    (T(RuleHeadFunc)
                     << (T(RuleArgs)[RuleArgs] * T(AssignOperator) *
                         T(Expr)[Expr])))) *
               (T(Empty) / T(UnifyBody))[Body] * T(ElseSeq)[ElseSeq])) >>
        [](Match& _) {
          Node elseseq = _(ElseSeq);
          if (elseseq->size() == 0)
          {
            return RuleFunc << _(Id) << _(RuleArgs) << _(Body) << _(Expr)
                            << (JSONInt ^ "0");
          }

          // we need to create sub-rules for each possibility
          Location rulename = _(Id)->location();
          Location subrule = _.fresh(rulename);
          Node seq = NodeDef::create(Seq);
          seq
            << (RuleFunc << (Var ^ subrule) << _(RuleArgs) << _(Body) << _(Expr)
                         << (JSONInt ^ "0"));
          for (std::size_t i = 0; i < elseseq->size(); ++i)
          {
            Node expr = elseseq->at(i) / Val;
            Node body = elseseq->at(i) / Body;
            seq
              << (RuleFunc << (Var ^ subrule) << _(RuleArgs)->clone() << body
                           << expr << (JSONInt ^ std::to_string(i + 1)));
          }

          Location value = _.fresh({"value"});
          Node argseq = NodeDef::create(ArgSeq);
          for (auto& arg : *_(RuleArgs))
          {
            if (arg->type() == ArgVar)
            {
              argseq->push_back(Expr << (RefTerm << (arg / Var)->clone()));
            }
            else if (arg->type() == ArgVal)
            {
              argseq->push_back(Expr << (Term << arg->front()->clone()));
            }
          }

          return seq
            << (RuleFunc << _(Id) << _(RuleArgs)->clone() << Empty
                         << (UnifyBody
                             << (Literal
                                 << (Expr << (RefTerm << (Var ^ value)) << Unify
                                          << (ExprCall
                                              << (RuleRef << (Var ^ subrule))
                                              << argseq))))
                         << (JSONInt ^ "0"));
        },

      In(Policy) *
          (T(Rule)
           << (T(JSONTrue) *
               (T(RuleHead)
                << ((T(RuleRef) << T(Var)[Id]) *
                    (T(RuleHeadFunc)
                     << (T(RuleArgs)[RuleArgs] * T(AssignOperator) *
                         T(Expr)[Expr])))) *
               T(Empty) * (T(ElseSeq) << End))) >>
        [](Match& _) {
          std::size_t rank = std::numeric_limits<std::size_t>::max();
          return RuleFunc << _(Id) << _(RuleArgs) << Empty << _(Expr)
                          << (JSONInt ^ std::to_string(rank));
        },

      In(Policy) *
          (T(Rule)
           << (T(JSONFalse) *
               (T(RuleHead)
                << ((T(RuleRef) << T(Var)[Id]) *
                    (T(RuleHeadSet) << T(Expr)[Expr]))) *
               (T(Empty) / T(UnifyBody))[Body] * T(ElseSeq))) >>
        [](Match& _) { return RuleSet << _(Id) << _(Body) << _(Expr); },

      In(Policy) *
          (T(Rule)
           << (T(JSONFalse) *
               (T(RuleHead)
                << ((T(RuleRef) << T(Var)[Id]) *
                    (T(RuleHeadObj)
                     << (T(Expr)[Key] * T(AssignOperator) * T(Expr)[Val])))) *
               (T(Empty) / T(UnifyBody))[Body] * T(ElseSeq))) >>
        [](Match& _) {
          return RuleObj << _(Id) << _(Body) << _(Key) << _(Val);
        },

      In(Expr) * (T(Expr) << (T(Term)[Term] * End)) >>
        [](Match& _) { return _(Term); },

      In(Expr) * (T(Term) << (T(Ref) / T(Var))[Val]) >>
        [](Match& _) { return RefTerm << _(Val); },

      In(Expr) * (T(Term) << (T(Scalar) << (T(JSONInt) / T(JSONFloat))[Val])) >>
        [](Match& _) { return NumTerm << _(Val); },

      In(Expr) * (T(Term) << (T(Set) / T(SetCompr))[Set]) >>
        [](Match& _) { return _(Set); },

      In(RefArgBrack) * T(Var)[Var] >>
        [](Match& _) { return RefTerm << _(Var); },

      In(UnifyBody) * (T(Literal) << (T(SomeDecl) << T(VarSeq)[VarSeq])) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          for (auto& var : *_(VarSeq))
          {
            seq->push_back(Local << var << Undefined);
          }
          return seq;
        },

      In(UnifyBody) *
          (T(Literal)
           << (T(SomeExpr)
               << (T(Undefined) * T(Expr)[Item] *
                   (T(IsIn) << T(Expr)[ItemSeq])))) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          return Seq << (Local << (Var ^ item) << Undefined)
                     << (LiteralEnum << (Var ^ item) << _(ItemSeq))
                     << (Literal
                         << (Expr
                             << _(Item) << Unify
                             << (RefTerm
                                 << (Ref << (RefHead << (Var ^ item))
                                         << (RefArgSeq
                                             << (RefArgBrack
                                                 << (Scalar
                                                     << (JSONInt ^ "1"))))))));
        },

      In(UnifyBody) *
          (T(Literal)
           << (T(SomeExpr)
               << (T(Expr)[Idx] * T(Expr)[Item] *
                   (T(IsIn) << T(Expr)[ItemSeq])))) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          return Seq << (Local << (Var ^ item) << Undefined)
                     << (LiteralEnum << (Var ^ item) << _(ItemSeq))
                     << (Literal
                         << (Expr
                             << _(Idx) << Unify
                             << (RefTerm
                                 << (Ref << (RefHead << (Var ^ item))
                                         << (RefArgSeq
                                             << (RefArgBrack
                                                 << (Scalar
                                                     << (JSONInt ^ "0"))))))))
                     << (Literal
                         << (Expr
                             << _(Item) << Unify
                             << (RefTerm
                                 << (Ref << (RefHead << (Var ^ item))
                                         << (RefArgSeq
                                             << (RefArgBrack
                                                 << (Scalar
                                                     << (JSONInt ^ "1"))))))));
        },

      (In(UnifyBody) / In(LiteralWith)) *
          (T(Literal)
           << (T(Expr) << (LocalToken++[Head] * T(Assign) * Any++[Tail]))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Nodes vars;
          for (auto& node : *_(Head))
          {
            find_assigned_vars(node, vars);
          }

          for (auto& var : vars)
          {
            seq->push_back(Lift << UnifyBody << (Local << var << Undefined));
          }

          seq->push_back(Literal << (Expr << _[Head] << Unify << _[Tail]));
          return seq;
        },

      In(Policy) * (T(Import) << (T(Ref)[Ref] * T(As) * T(Var)[Var])) >>
        [](Match& _) { return Import << _(Var) << _(Ref); },

      (In(RuleComp) / In(RuleFunc) / In(RuleObj) / In(RuleSet)) *
          (T(Expr)
           << (T(Term)[Term]([](auto& n) { return is_constant(*n.first); }) *
               End)) >>
        [](Match& _) { return _(Term); },

      (In(RuleComp) / In(RuleFunc) / In(RuleObj) / In(RuleSet)) *
          (T(Expr) << (T(NumTerm)[NumTerm] * End)) >>
        [](Match& _) {
          Node number = _(NumTerm)->front();
          return Term << (Scalar << number);
        },

      In(Expr) * (Start * T(Subtract) * T(NumTerm)[NumTerm] * End) >>
        [](Match& _) {
          Node number = Resolver::negate(_(NumTerm)->front());
          return NumTerm << number;
        },

      (In(RuleComp) / In(RuleFunc)) * (T(Empty) * T(Expr)[Expr]) >>
        [](Match& _) {
          Location out = _.fresh({"out"});
          Location value = _.fresh({"value"});
          return Seq << (UnifyBody
                         << (Literal
                             << (Expr << (RefTerm << (Var ^ out)) << Unify
                                      << *_[Expr])))
                     << (UnifyBody
                         << (Literal
                             << (Expr << (RefTerm << (Var ^ value)) << Unify
                                      << (RefTerm << (Var ^ out)))));
        },

      (In(RuleComp) / In(RuleFunc)) * T(Expr)[Expr] >>
        [](Match& _) {
          Location value = _.fresh({"value"});
          return UnifyBody
            << (Literal
                << (Expr << (RefTerm << (Var ^ value)) << Unify << *_[Expr]));
        },

      (In(ObjectCompr) / In(SetCompr) / In(ArrayCompr)) *
          T(UnifyBody)[UnifyBody] >>
        [](Match& _) {
          std::string prefix;
          auto& parent_type = _(UnifyBody)->parent()->type();
          if (parent_type == ArrayCompr)
          {
            prefix = "array";
          }
          else if (parent_type == SetCompr)
          {
            prefix = "set";
          }
          else
          {
            prefix = "object";
          }
          Location compr = _.fresh({prefix + "compr"});
          return NestedBody << (Key ^ compr) << _(UnifyBody);
        },

      In(Expr) *
          (T(ExprEvery)([](auto& n) { return is_in(*n.first, {UnifyBody}); })
           << ((T(VarSeq) << (T(Var)[Val] * End)) * T(UnifyBody)[UnifyBody] *
               (T(IsIn) << T(Expr)[Expr]))) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          Location itemseq = _.fresh({"itemseq"});
          Node rulebody = UnifyBody
            << (Local << (Var ^ item) << Undefined)
            << (LiteralEnum << (Var ^ item)
                            << (Expr << (RefTerm << (Var ^ itemseq))))
            << (Literal
                << (Expr << (RefTerm << _(Val)->clone()) << Unify
                         << (RefTerm
                             << (Ref
                                 << (RefHead << (Var ^ item))
                                 << (RefArgSeq
                                     << (RefArgBrack
                                         << (Scalar << (JSONInt ^ "1"))))))))
            << *_[UnifyBody];

          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ itemseq) << Undefined))
                     << (Lift << UnifyBody
                              << (Literal
                                  << (Expr << (RefTerm << (Var ^ itemseq))
                                           << Unify << *_[Expr])))
                     << (ExprCall
                         << (RuleRef << (Var ^ "count"))
                         << (ArgSeq
                             << (Expr
                                 << (Term
                                     << (ArrayCompr
                                         << (Expr
                                             << (RefTerm << _(Val)->clone()))
                                         << rulebody)))))
                     << Equals
                     << (ExprCall
                         << (RuleRef << (Var ^ "count"))
                         << (ArgSeq << (Expr << (RefTerm << (Var ^ itemseq)))));
        },

      In(Expr) *
          (T(ExprEvery)([](auto& n) { return is_in(*n.first, {UnifyBody}); })
           << ((T(VarSeq) << (T(Var)[Idx] * T(Var)[Val] * End)) *
               T(UnifyBody)[UnifyBody] * (T(IsIn) << T(Expr)[Expr]))) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          Location itemseq = _.fresh({"itemseq"});
          Node rulebody = UnifyBody
            << (Local << (Var ^ item) << Undefined)
            << (LiteralEnum << (Var ^ item)
                            << (Expr << (RefTerm << (Var ^ itemseq))))
            << (Literal
                << (Expr << (RefTerm << _(Idx)->clone()) << Unify
                         << (RefTerm
                             << (Ref
                                 << (RefHead << (Var ^ item))
                                 << (RefArgSeq
                                     << (RefArgBrack
                                         << (Scalar << (JSONInt ^ "0"))))))))
            << (Literal
                << (Expr << (RefTerm << _(Val)->clone()) << Unify
                         << (RefTerm
                             << (Ref
                                 << (RefHead << (Var ^ item))
                                 << (RefArgSeq
                                     << (RefArgBrack
                                         << (Scalar << (JSONInt ^ "1"))))))))
            << *_[UnifyBody];

          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ itemseq) << Undefined))
                     << (Lift << UnifyBody
                              << (Literal
                                  << (Expr << (RefTerm << (Var ^ itemseq))
                                           << Unify << *_[Expr])))
                     << (ExprCall
                         << (RuleRef << (Var ^ "count"))
                         << (ArgSeq
                             << (Expr
                                 << (Term
                                     << (ArrayCompr
                                         << (Expr
                                             << (RefTerm << _(Val)->clone()))
                                         << rulebody)))))
                     << Equals
                     << (ExprCall
                         << (RuleRef << (Var ^ "count"))
                         << (ArgSeq << (Expr << (RefTerm << (Var ^ itemseq)))));
        },

      // errors

      In(Policy) * T(Rule)[Rule] >>
        [](Match& _) { return err(_(Rule), "Invalid rule"); },

      In(UnifyBody) * (T(RuleHead)[RuleHead] << (T(Var) * T(RuleHeadFunc))) >>
        [](Match& _) {
          return err(_(RuleHead), "No rule functions allowed in rule bodies");
        },

      In(Term) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Invalid ref term"); },

      In(Term) * T(Var)[Var] >>
        [](Match& _) { return err(_(Var), "Invalid var term"); },

      In(Literal) * T(SomeDecl)[SomeDecl] >>
        [](Match& _) { return err(_(SomeDecl), "Invalid some declaration"); },

      In(Expr) * T(Assign)[Assign] >>
        [](Match& _) { return err(_(Assign), "Invalid assignment"); },

      T(UnifyBody)[UnifyBody] << End >>
        [](Match& _) { return err(_(UnifyBody), "Invalid unification body"); },

      In(Expr) * T(Dot)[Dot] >>
        [](Match& _) { return err(_(Dot), "Invalid dot expression"); },

      In(Expr) * T(ExprEvery)[ExprEvery] >>
        [](Match& _) { return err(_(ExprEvery), "Invalid every expression"); },
    };
  }

}
