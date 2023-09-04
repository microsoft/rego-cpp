#include "errors.h"
#include "helpers.h"
#include "passes.h"
#include "resolver.h"

namespace
{
  using namespace rego;

  const inline std::set<Token> Ops = {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Equals,
    NotEquals,
    GreaterThan,
    LessThan,
    GreaterThanOrEquals,
    LessThanOrEquals,
    And,
    Or,
    Assign,
    Unify};
}

namespace rego
{
  // Modify the AST to resemble the target Rego syntax as much as possible.
  // At this point "parsing" is done, i.e. the AST resembles the syntactical
  // form of Rego code. From here, passes modify the AST to prepare it for
  // unification.
  PassDef structure()
  {
    PassDef structure = {
      In(UnifyBody) *
          (T(Group)[Lhs]([](auto& n) {
             // test if there was a newline inside an expression
             Node node = *n.first;
             return Ops.contains(node->back()->type());
           }) *
           T(Group)[Rhs]) >>
        [](Match& _) { return Group << *_[Lhs] << *_[Rhs]; },

      In(Query) * T(Group)[Group] >>
        [](Match& _) { return Literal << (Expr << *_[Group]); },

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

      In(Rule) *
          (T(RuleHead)
           << ((T(RuleRef) << (T(Ref)[Ref] * (T(Array) << T(Group)[Key]))) *
               (T(RuleHeadComp)
                << (T(AssignOperator)[AssignOperator] * T(Group)[Val])))) >>
        [](Match& _) {
          return RuleHead << (RuleRef << _(Ref))
                          << (RuleHeadObj << _(Key) << _(AssignOperator)
                                          << _(Val));
        },

      (In(RuleHeadComp) / In(RuleHeadFunc) / In(RuleHeadSet) /
       In(RuleHeadObj)) *
          T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

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

      In(Import) *
          ((T(ImportRef) << (T(Group) << (T(Ref)[Ref] * End))) * T(As) *
           T(Undefined)) >>
        [](Match& _) {
          Node refhead = _(Ref) / RefHead;
          Node refargseq = _(Ref) / RefArgSeq;
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

      In(RefArgBrack) * (T(Group) << (T(Var)[Var] * End)) >>
        [](Match& _) { return _(Var); },

      In(RefArgBrack) * (T(Group) << (ScalarToken[Val] * End)) >>
        [](Match& _) { return Scalar << _(Val); },

      In(RefArgBrack) * (T(Group) << (StringToken[Val] * End)) >>
        [](Match& _) { return Scalar << (String << _(Val)); },

      In(RefArgBrack) * (T(Group) << (T(Object)[Object] * End)) >>
        [](Match& _) { return _(Object); },

      In(RefArgBrack) * (T(Group) << (T(Array)[Array] * End)) >>
        [](Match& _) { return _(Array); },

      In(RefArgBrack) * (T(Group) << (T(Set)[Set] * End)) >>
        [](Match& _) { return _(Set); },

      In(RefArgBrack) * (T(Group) << (T(UnifyBody)[UnifyBody] * End)) >>
        [](Match& _) { return Set << *_[UnifyBody]; },

      In(RefArgBrack) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      (In(Array) / In(Set)) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      In(UnifyBody) *
          (T(Group) << (T(SomeDecl)[SomeDecl] * T(With)++[WithSeq])) >>
        [](Match& _) {
          if (_[WithSeq].first == _[WithSeq].second)
          {
            return Literal << _(SomeDecl);
          }

          return LiteralWith << (UnifyBody << (Literal << _(SomeDecl)))
                             << (WithSeq << _[WithSeq]);
        },

      In(UnifyBody) * T(Group)[Group] >>
        [](Match& _) {
          Node withseq = NodeDef::create(WithSeq);
          Node maybe_with = _(Group)->back();
          while (maybe_with->type() == With)
          {
            _(Group)->pop_back();
            withseq->push_front(maybe_with);
            if (_(Group)->size() == 0)
            {
              break;
            }

            maybe_with = _(Group)->back();
          }

          if (_(Group)->size() == 0)
          {
            return withseq;
          }

          Node expr = NodeDef::create(Expr);
          expr->insert(expr->end(), _(Group)->begin(), _(Group)->end());

          if (withseq->size() > 0)
          {
            return LiteralWith << (UnifyBody << (Literal << expr)) << withseq;
          }

          return Literal << expr;
        },

      In(UnifyBody) * (T(Literal)[Literal] * T(WithSeq)[WithSeq]) >>
        [](Match& _) {
          return LiteralWith << (UnifyBody << _(Literal)) << _(WithSeq);
        },

      In(UnifyBody) *
          ((T(LiteralWith) << (T(UnifyBody)[UnifyBody] * T(WithSeq)[Head])) *
           T(WithSeq)[Tail]) >>
        [](Match& _) {
          return LiteralWith << _(UnifyBody)
                             << (WithSeq << *_[Head] << *_[Tail]);
        },

      In(UnifyBody) *
          ((T(LiteralWith) << (T(UnifyBody)[UnifyBody] * T(WithSeq)[WithSeq])) *
           T(With)[With]) >>
        [](Match& _) {
          return LiteralWith << _(UnifyBody)
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
          else if (_(VarSeq)->size() == 2)
          {
            return SomeExpr << *_[VarSeq] << (IsIn << (Expr << _[Expr]));
          }
          else
          {
            return err(_(VarSeq), "Invalid some expression");
          }
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

      In(ExprEvery) * T(EverySeq)[EverySeq] >>
        [](Match& _) { return err(_(EverySeq), "Invalid every sequence"); },

      T(UnifyBody)[UnifyBody] << End >>
        [](Match& _) { return err(_(UnifyBody), "Empty body"); },

      T(Expr)[Expr] << End >>
        [](Match& _) { return err(_(Expr), "Empty expression"); },

      (In(UnifyBody) / In(Expr)) * T(SomeDecl)[SomeDecl] >>
        [](Match& _) { return err(_(SomeDecl), "Invalid some"); },

      In(Expr) * T(UnifyBody)[UnifyBody] >>
        [](Match& _) { return err(_(UnifyBody), "Invalid body location"); },

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

      In(UnifyBody) * T(WithSeq)[WithSeq] >>
        [](Match& _) { return err(_(WithSeq), "Invalid with statement"); },

      In(UnifyBody) * T(SomeDecl)[SomeDecl] >>
        [](Match& _) { return err(_(SomeDecl), "Invalid some declaration"); },

      In(Set) * T(SomeDecl)[SomeDecl] >>
        [](Match& _) { return err(_(SomeDecl), "Invalid some declaration"); },
    };

    return structure;
  }
}