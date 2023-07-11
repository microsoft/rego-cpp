#include "passes.h"
#include "resolver.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const inline auto RefHeadToken = T(Var) / T(ExprCall);

  // clang-format off
  inline const auto wfi =
      (Top <<= Rego)
    | (Paren <<= (Group | List))
    | (RefArgDot <<= Var)
    | (Else <<= (Val >>= Undefined | Group) * UnifyBody)
    ;
  // clang-format on

  Node to_elseseq(const NodeRange& elses, const Node& init_value)
  {
    Node value = init_value;
    Node elseseq = NodeDef::create(ElseSeq);
    for (auto it = elses.first; it < elses.second; ++it)
    {
      Node n = *it;
      if (n->type() == UnifyBody)
      {
        n = Else << init_value->clone() << n;
      }
      else if (n->type() == Else)
      {
        Node else_value = n->at(wfi / Else / Val);
        if (else_value->type() == Undefined)
        {
          n->replace(else_value, value->clone());
        }
        else
        {
          value = else_value;
        }
      }
      elseseq->push_back(n);
    }
    return elseseq;
  }
}

namespace rego
{
  PassDef structure()
  {
    return {
      In(Query) * T(Group)[Group] >>
        [](Match& _) { return Literal << (Expr << *_[Group]); },

      (In(ObjectItemSeq) / In(Object)) *
          (T(ObjectItem) << T(Group)[ObjectItemHead] * T(Group)[Expr]) >>
        [](Match& _) {
          return ObjectItem << (ObjectItemHead << *_[ObjectItemHead])
                            << (Expr << *_[Expr]);
        },

      In(ObjectItemHead) * StringToken[String] >>
        [](Match& _) { return Scalar << (String << _(String)); },

      In(ObjectItemHead) * ScalarToken[Scalar] >>
        [](Match& _) { return Scalar << _(Scalar); },

      (In(ObjectItemHead) / In(Expr)) *
          (RefHeadToken[RefHead] * T(Dot) * T(Var)[Rhs]) >>
        [](Match& _) {
          return Ref << (RefHead << _(RefHead))
                     << (RefArgSeq << (RefArgDot << _(Rhs)));
        },

      (In(ObjectItemHead) / In(Expr)) *
          (RefHeadToken[RefHead] * T(Array)[Array]) >>
        [](Match& _) {
          return Ref << (RefHead << _(RefHead))
                     << (RefArgSeq << (RefArgBrack << *_[Array]));
        },

      (In(ObjectItemHead) / In(Expr)) *
          ((T(Ref) << (T(RefHead)[RefHead] * T(RefArgSeq)[RefArgSeq])) *
           T(Dot) * T(Var)[Rhs]) >>
        [](Match& _) {
          return Ref << _(RefHead)
                     << (RefArgSeq << *_[RefArgSeq] << (RefArgDot << _(Rhs)));
        },

      (In(ObjectItemHead) / In(Expr)) *
          ((T(Ref) << (T(RefHead)[RefHead] * T(RefArgSeq)[RefArgSeq])) *
           T(Array)[Array]) >>
        [](Match& _) {
          return Ref << _(RefHead)
                     << (RefArgSeq << *_[RefArgSeq]
                                   << (RefArgBrack << *_[Array]));
        },

      (In(ObjectItemHead) / In(Expr)) *
          ((T(Ref)
            << ((T(RefHead) << T(Var)[RefHead]) *
                (T(RefArgSeq) << (T(RefArgDot)++[RefArgDot] * End)))) *
           T(Paren)[Paren]) >>
        [](Match& _) {
          Node varseq = VarSeq << _(RefHead);
          NodeRange dots = _[RefArgDot];
          for (auto it = dots.first; it < dots.second; ++it)
          {
            Node n = *it;
            varseq->push_back(n->at(wfi / RefArgDot / Var));
          }

          Node argseq = NodeDef::create(ArgSeq);
          Node paren = _(Paren);
          if (paren->at(wfi / Paren / Paren)->type() == List)
          {
            for (const auto& arg : *paren->at(wfi / Paren / Paren))
            {
              argseq->push_back(arg);
            }
          }
          else
          {
            argseq->push_back(paren->at(wfi / Paren / Paren));
          }

          return ExprCall << varseq << argseq;
        },

      (In(ObjectItemHead) / In(Expr)) * (T(Var)[Var] * T(Paren)[Paren]) >>
        [](Match& _) {
          Node argseq = NodeDef::create(ArgSeq);
          Node paren = _(Paren);
          if (paren->at(wfi / Paren / Paren)->type() == List)
          {
            for (const auto& arg : *paren->at(wfi / Paren / Paren))
            {
              argseq->push_back(arg);
            }
          }
          else
          {
            argseq->push_back(paren->at(wfi / Paren / Paren));
          }

          return ExprCall << (VarSeq << _(Var)) << argseq;
        },

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

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node value = Expr << (Term << (Scalar << JSONTrue));
          return Rule << (RuleHead << _(Id)
                                   << (RuleHeadComp
                                       << (AssignOperator << Assign) << value))
                      << _(UnifyBody) << to_elseseq(_[Else], _(UnifyBody));
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Assign) / T(Unify)) * ExprToken[Head] *
               ExprToken++[Tail] * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node value = (Expr << _(Head) << _[Tail]);
          return Rule << (RuleHead << _(Id)
                                   << (RuleHeadComp
                                       << (AssignOperator << Assign) << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Assign) / T(Unify)) * ExprToken[Head] *
               ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Expr << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Paren)[Paren] * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node args = NodeDef::create(RuleArgs);
          Node paren = _(Paren);
          if (paren->at(wfi / Paren / Paren)->type() == List)
          {
            for (const auto& arg : *paren->at(wfi / Paren / Paren))
            {
              args->push_back(arg);
            }
          }
          else
          {
            args->push_back(paren->at(wfi / Paren / Paren));
          }

          Node value = Expr << (Term << (Scalar << JSONTrue));
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Paren)[Paren] * (T(Assign) / T(Unify)) *
               ExprToken[Head] * ExprToken++[Tail] * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node args = NodeDef::create(RuleArgs);
          Node paren = _(Paren);
          if (paren->at(wfi / Paren / Paren)->type() == List)
          {
            for (const auto& arg : *paren->at(wfi / Paren / Paren))
            {
              args->push_back(arg);
            }
          }
          else
          {
            args->push_back(paren->at(wfi / Paren / Paren));
          }

          Node value = Expr << _(Head) << _[Tail];
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Contains) * ExprToken[Head] * ExprToken++[Tail] *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadSet << (Expr << _(Head) << _[Tail])))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Contains) * ExprToken[Head] *
               ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadSet << (Expr << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Key] * End)) *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadObj
                              << (Expr << *_[Key]) << (AssignOperator << Assign)
                              << (Expr << (Term << (Scalar << JSONTrue)))))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Key] * End)) *
               (T(Assign) / T(Unify)) * ExprToken[Head] * ExprToken++[Tail] *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadObj << (Expr << *_[Key])
                                          << (AssignOperator << Assign)
                                          << (Expr << _(Head) << _[Tail])))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Key] * End)) *
               (T(Assign) / T(Unify)) * ExprToken[Head] * ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadObj << (Expr << *_[Key])
                                          << (AssignOperator << Assign)
                                          << (Expr << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Default) * T(Var)[Id] * (T(Assign) / T(Unify)) *
               ScalarToken[Scalar])) >>
        [](Match& _) {
          return DefaultRule << _(Id) << (Term << (Scalar << _(Scalar)));
        },

      In(Policy) *
          (T(Group)
           << (T(Default) * T(Var)[Id] * (T(Assign) / T(Unify)) *
               StringToken[String])) >>
        [](Match& _) {
          return DefaultRule << _(Id)
                             << (Term << (Scalar << (String << _(String))));
        },

      In(Policy) *
          (T(Group)
           << (T(Default) * T(Var)[Id] * (T(Assign) / T(Unify)) *
               (T(Object) / T(Array) / T(Set))[Term])) >>
        [](Match& _) { return DefaultRule << _(Id) << (Term << _(Term)); },

      (In(Array) / In(Set)) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      In(UnifyBody) * (T(Group) << T(SomeDecl)[SomeDecl]) >>
        [](Match& _) { return Literal << _(SomeDecl); },

      In(UnifyBody) * T(Group)[Group] >>
        [](Match& _) { return Literal << (Expr << *_[Group]); },

      T(Paren) << (T(Expr)[Expr] * End) >> [](Match& _) { return _(Expr); },

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

      In(SomeDecl) * (T(Group) << T(Default)) >>
        [](Match&) { return InSome << Undefined; },

      In(SomeDecl) * (T(Group) << (T(InSome) * ExprToken++[Expr] * End)) >>
        [](Match& _) { return InSome << (Expr << _(Expr)); },

      In(VarSeq) * (T(Group) << (T(Var)[Var] * End)) >>
        [](Match& _) { return _(Var); },

      In(Expr) * T(InSome)[InSome] >>
        [](Match& _) { return IsIn ^ _(InSome)->location(); },

      // errors

      In(Policy) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Naked expression in policy"); },

      In(RefArgBrack) * (T(Expr) * T(Expr)[Expr]) >>
        [](Match& _) {
          return err(_(Expr), "Multi-dimensional indexing is not supported");
        },

      In(Expr) * T(Dot)[Dot] >>
        [](Match& _) { return err(_(Dot), "Invalid expression token"); },

      In(RefArgSeq) * (T(RefArgBrack)[RefArgBrack] << End) >>
        [](Match& _) { return err(_(RefArgBrack), "Missing index"); },

      In(Expr) * T(UnifyBody)[UnifyBody] >>
        [](Match& _) { return err(_(UnifyBody), "Invalid expression"); },

      In(ObjectItemHead) * (!(T(Scalar) / T(Var) / T(Ref)))[Key] >>
        [](Match& _) { return err(_(Key), "Invalid object item key"); },

      In(Expr) * T(Default)[Default] >>
        [](Match& _) { return err(_(Default), "Invalid use of default"); },

      (In(Group) / In(Expr)) * T(Paren)[Paren] >>
        [](Match& _) { return err(_(Paren), "Invalid function call"); },

      (In(Expr) / In(UnifyBody)) * T(SomeDecl)[SomeDecl] >>
        [](Match& _) { return err(_(SomeDecl), "Invalid some declaration"); },

      In(SomeDecl) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid some declaration"); },

      In(Policy) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid rule"); },

      T(Query)[Query] << End >>
        [](Match& _) { return err(_(Query), "Missing query"); },

      T(UnifyBody)[UnifyBody] << End >>
        [](Match& _) { return err(_(UnifyBody), "Missing statements"); },

      T(RefArgBrack)[RefArgBrack] << (Any * Any) >>
        [](Match& _) { return err(_(RefArgBrack), "Invalid index"); },

      In(Expr) * T(Else)[Else] >>
        [](Match& _) { return err(_(Else), "Invalid else statement"); },

      In(Expr) * T(Contains)[Contains] >>
        [](Match& _) { return err(_(Contains), "Invalid contains statement"); },

      In(ExprCall) * (T(ArgSeq)[ArgSeq] << End) >>
        [](Match& _) {
          return err(_(ArgSeq), "No arguments in function call");
        },

      T(Group)[Group] >> [](Match& _) { return err(_(Group), "Syntax error"); },
    };
  }
}