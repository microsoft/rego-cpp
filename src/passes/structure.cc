#include "errors.h"
#include "passes.h"
#include "resolver.h"
#include "utils.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const inline auto RefHeadToken = T(Var) / T(ExprCall);

  // clang-format off
  inline const auto wfi =
      (Top <<= Rego)
    | (RefArgDot <<= Var)
    | (Else <<= (Val >>= Undefined | Group) * UnifyBody)
    | (Ref <<= RefHead * RefArgSeq)
    ;
  // clang-format on
}

namespace rego
{
  // Modify the AST to resemble the target Rego syntax as much as possible.
  // At this point "parsing" is done, i.e. the AST resembles the syntactical
  // form of Rego code. From here, passes modify the AST to prepare it for
  // unification.
  PassDef structure()
  {
    return {
      In(Query) * T(Group)[Group] >>
        [](Match& _) { return Literal << (Expr << *_[Group]); },

      (In(RuleHeadComp) / In(RuleHeadFunc) / In(RuleHeadSet) /
       In(RuleHeadObj)) *
          T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      In(Input) * (T(Group) << ScalarToken[Scalar]) >>
        [](Match& _) { return Term << (Scalar << _(Scalar)); },

      In(Input) * (T(Group) << StringToken[String]) >>
        [](Match& _) { return Term << (Scalar << (String << _(String))); },

      In(Input) * (T(Group) << TermToken[Term]) >>
        [](Match& _) { return Term << _(Term); },

      In(With) * (T(WithExpr) << T(Group)[Group]) >>
        [](Match& _) { return Expr << *_[Group]; },

      In(Membership) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      In(UnifyBody) *
          (T(Group)
           << (T(ExprEvery)
               << (T(VarSeq)[VarSeq] * T(UnifyBody)[UnifyBody] *
                   (T(EverySeq) << T(Group)[EverySeq])))) >>
        [](Match& _) {
          
          Node maybe_with = _(EverySeq)->back();
          if (maybe_with->type() == With)
          {
            maybe_with->parent()->pop_back();
            return LiteralWith
              << (UnifyBody
                  << (Literal
                      << (Expr
                          << (ExprEvery << _(VarSeq) << _(UnifyBody)
                                        << _(EverySeq)))))
              << (WithSeq << maybe_with);
          }

          return Literal
            << (Expr
                << (ExprEvery << _(VarSeq) << _(UnifyBody) << _(EverySeq)));
        },

      (In(ObjectItemSeq) / In(Object)) *
          (T(ObjectItem) << T(Group)[Key] * T(Group)[Val]) >>
        [](Match& _) {
          return ObjectItem << (Expr << *_[Key]) << (Expr << *_[Val]);
        },

      (In(ArrayCompr) / In(ObjectCompr) / In(SetCompr)) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      In(Package) * (T(Group) << (T(Var)[Var] * End)) >>
        [](Match& _) { return Ref << (RefHead << _(Var)) << RefArgSeq; },

      In(Package) * (T(Group) << (T(Ref)[Ref] * End)) >>
        [](Match& _) { return _(Ref); },

      In(With) * (T(WithRef) << (T(Group) << (T(Ref)[Ref] * End))) >>
        [](Match& _) {
          Node var = (_(Ref) / RefHead)->front();
          if (var->type() != Var)
          {
            return err(var, "Non-var refhead in with");
          }

          Node varseq = VarSeq << var;
          Node refargseq = _(Ref) / RefArgSeq;
          for (Node refarg : *refargseq)
          {
            if (refarg->type() == RefArgDot)
            {
              varseq << refarg->front();
            }
            else if (refarg->type() == RefArgBrack)
            {
              Node index = refarg->front();
              if (index->type() == Var)
              {
                return err(index, "Non-constant index in with");
              }
              std::string index_str = strip_quotes(to_json(index));
              varseq << (Var ^ index_str);
            }
            else
            {
              return err(refarg, "Invalid refarg in with");
            }
          }

          return varseq;
        },

      In(With) * (T(WithRef) << (T(Group) << (T(Var)[Var] * End))) >>
        [](Match& _) { return VarSeq << _(Var); },

      In(Import) *
          ((T(ImportRef) << (T(Group) << (T(Ref)[Ref] * End))) * T(As) *
           T(Undefined)) >>
        [](Match& _) {
          Node refhead = wfi / _(Ref) / RefHead;
          Node refargseq = wfi / _(Ref) / RefArgSeq;
          Node var;
          if (refargseq->size() == 0)
          {
            if (refhead->front()->type() == Var)
            {
              var = refhead->front()->clone();
            }
            else
            {
              return err(refhead, "Invalid import statement");
            }
          }
          else
          {
            Node refarg = refargseq->back();
            if (refarg->type() == RefArgDot)
            {
              var = refarg->front()->clone();
            }
            else if (refarg->type() == RefArgBrack)
            {
              var = Var ^ strip_quotes(to_json(refarg->front()));
            }
            else
            {
              return err(refarg, "Invalid refarg in import");
            }
          }
          return Seq << _(Ref) << As << var;
        },

      In(Import) * (T(ImportRef) << (T(Group) << (T(Ref)[Ref] * End))) >>
        [](Match& _) { return _(Ref); },

      In(Else) * T(Group)[Group] >> [](Match& _) { return Expr << *_[Group]; },

      In(ArgSeq) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      In(Expr) * (T(Paren) << T(Group)[Group]) >>
        [](Match& _) { return Expr << *_[Group]; },

      In(Expr) * T(ObjectItemSeq)[ObjectItemSeq] >>
        [](Match& _) { return Object << *_[ObjectItemSeq]; },

      In(Array) * (T(Group) << T(Expr)[Expr]) >>
        [](Match& _) { return _(Expr); },

      In(RefArgBrack) * (T(Group) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

      In(RefArgBrack) * (T(Group) << ScalarToken[Val]) >>
        [](Match& _) { return Scalar << _(Val); },

      In(RefArgBrack) * (T(Group) << StringToken[Val]) >>
        [](Match& _) { return Scalar << (String << _(Val)); },

      In(RefArgBrack) * (T(Group) << T(Object)[Object]) >>
        [](Match& _) { return _(Object); },

      In(RefArgBrack) * (T(Group) << T(Array)[Array]) >>
        [](Match& _) { return _(Array); },

      In(RefArgBrack) * (T(Group) << T(Set)[Set]) >>
        [](Match& _) { return _(Set); },

      (In(Array) / In(Set)) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      In(UnifyBody) *
          (T(Group)
           << (T(SomeDecl) << (T(VarSeq)[VarSeq] * T(Group)[Group]))) >>
        [](Match& _) {
          Node maybe_with = _(Group)->back();
          if (maybe_with->type() == With)
          {
            _(Group)->pop_back();
            return LiteralWith
              << (UnifyBody << (Literal << (SomeDecl << _(VarSeq) << _(Group))))
              << (WithSeq << maybe_with);
          }
          return Literal << (SomeDecl << _(VarSeq) << _(Group));
        },

      In(UnifyBody) * T(Group)[Group] >>
        [](Match& _) {
          Node maybe_with = _(Group)->back();
          if (maybe_with->type() == With)
          {
            if (_(Group)->size() == 1)
            {
              return maybe_with;
            }

            Node expr = NodeDef::create(Expr);
            expr->insert(expr->begin(), _(Group)->begin(), _(Group)->end() - 1);
            return LiteralWith << (UnifyBody << (Literal << expr))
                               << (WithSeq << maybe_with);
          }
          return Literal << (Expr << *_[Group]);
        },

      In(UnifyBody) *
          ((T(LiteralWith) << (T(Literal)[Literal] * T(WithSeq)[WithSeq])) *
           T(With)[With]) >>
        [](Match& _) {
          return LiteralWith << (UnifyBody << _(Literal))
                             << (WithSeq << *_[WithSeq] << _(With));
        },

      T(Paren) << (T(Expr)[Expr] * End) >> [](Match& _) { return _(Expr); },

      In(Expr) * (T(Every) * Any++[Every] * End) >>
        [](Match& _) { return ExprEvery << _[Every]; },

      In(Expr) * StringToken[String] >>
        [](Match& _) { return Term << (Scalar << (String << _(String))); },

      In(Expr) * ScalarToken[Val] >>
        [](Match& _) { return Term << (Scalar << _(Val)); },

      In(Expr) * TermToken[Val] >> [](Match& _) { return Term << _(Val); },

      In(RuleArgs) * (T(Group) << StringToken[String]) >>
        [](Match& _) { return Term << (Scalar << (String << _(String))); },

      In(RuleArgs) * (T(Group) << ScalarToken[Scalar]) >>
        [](Match& _) { return Term << (Scalar << _(Scalar)); },

      In(RuleArgs) * (T(Group) << TermToken[Val]) >>
        [](Match& _) { return Term << _(Val); },

      In(RuleArgs) *
          (T(Group) << (T(Subtract) * (T(JSONInt) / T(JSONFloat))[Val])) >>
        [](Match& _) { return Term << (Scalar << Resolver::negate(_(Val))); },

      In(ExprEvery) * (T(Group) << (T(IsIn) * ExprToken++[Expr] * End)) >>
        [](Match& _) { return IsIn << (Expr << _[Expr]); },

      In(Expr) * (T(UnifyBody) << (T(Group)[Group] * End)) >>
        [](Match& _) { return Set << _(Group); },

      In(VarSeq) * (T(Group) << T(Var)[Var]) >> [](Match& _) { return _(Var); },

      In(SomeDecl) * (T(VarSeq)[VarSeq] * (T(Group) << T(Undefined))) >>
        [](Match& _) { return _(VarSeq); },

      In(Literal) *
          (T(SomeDecl)
           << (T(VarSeq)[VarSeq] *
               (T(Group) << (T(IsIn) * ExprToken++[Expr] * End)))) >>
        [](Match& _) {
          if (_(VarSeq)->size() == 1)
          {
            return SomeExpr << Undefined << _(VarSeq)->front()
                            << (IsIn << (Expr << _[Expr]));
          }

          return SomeExpr << *_[VarSeq] << (IsIn << (Expr << _[Expr]));
        },

      In(SomeExpr) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      // errors

      In(Input) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid input term"); },

      In(VarSeq) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Expected a variable"); },

      In(Import) * T(ImportRef)[ImportRef] >>
        [](Match& _) { return err(_(ImportRef), "Invalid import reference"); },

      In(With) * T(WithRef)[WithRef] >>
        [](Match& _) { return err(_(WithRef), "Invalid with reference"); },

      In(Package) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid package name"); },

      (In(ExprCall) / In(ExprEvery) / In(SomeDecl)) *
          (T(VarSeq)[VarSeq] << End) >>
        [](Match& _) { return err(_(VarSeq), "Missing variables"); },

      In(DefaultRule) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid default rule"); },

      (In(ObjectCompr) / In(ArrayCompr) / In(SetCompr)) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid comprehension"); },

      In(Expr) * T(Contains)[Contains] >>
        [](Match& _) { return err(_(Contains), "Invalid set rule"); },

      In(Expr) * T(Paren)[Paren] >>
        [](Match& _) { return err(_(Paren), "Invalid sub-expressions"); },

      In(Expr) * T(With)[With] >>
        [](Match& _) { return err(_(With), "Invalid with"); },

      In(Expr) * T(Undefined)[Undefined] >>
        [](Match& _) { return err(_(Undefined), "Syntax error"); },

      In(ExprEvery) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid every sequence"); },

      T(UnifyBody)[UnifyBody] << End >>
        [](Match& _) { return err(_(UnifyBody), "Empty body"); },

      T(Expr)[Expr] << End >>
        [](Match& _) { return err(_(Expr), "Empty expression"); },

      (In(UnifyBody) / In(Expr)) * T(SomeDecl)[SomeDecl] >>
        [](Match& _) { return err(_(SomeDecl), "Invalid some"); },

      In(Expr) * T(UnifyBody)[UnifyBody] >>
        [](Match& _) { return err(_(UnifyBody), "Invalid body location"); },

      In(RefArgBrack) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid index"); },

      In(RuleArgs) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid argument"); },

      In(UnifyBody) * T(With)[With] >>
        [](Match& _) { return err(_(With), "Invalid with statement"); },

      In(Rego) * (T(Query)[Query] << End) >>
        [](Match& _) { return err(_(Query), "Must provide a query"); },

      In(Expr) * T(IsIn)[IsIn] >>
        [](Match& _) { return err(_(IsIn), "Invalid expression"); },

      In(SomeDecl) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid some declaration"); },

      In(RuleRef) * (Any * Any[Val]) >>
        [](Match& _) { return err(_(Val), "Invalid rule reference"); },

      In(RuleRef) * (T(Dot) / T(Array))[Val] >>
        [](Match& _) { return err(_(Val), "Invalid rule reference"); },
    };
  }
}