#include "passes.h"
#include "resolver.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const inline auto TermToken = T(Var) / T(Ref) / T(Array) / T(Object) / T(Set);
  const inline auto ExprToken = T(Term) / ArithToken / BoolToken / T(Expr) /
    ScalarToken / TermToken / T(JSONString) / T(Array) / T(Set) / T(Object) /
    T(Paren) / T(Not) / T(Dot);
  const inline auto StringToken = T(JSONString) / T(RawString);

  // clang-format off
  inline const auto wfi =
      (Top <<= Rego)
    | (Paren <<= (Group | List))
    ;
  // clang-format on
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

      (In(ObjectItemHead) / In(Expr)) * (T(Var)[Lhs] * T(Dot) * T(Var)[Rhs]) >>
        [](Match& _) {
          return Ref << _(Lhs) << (RefArgSeq << (RefArgDot << _(Rhs)));
        },

      (In(ObjectItemHead) / In(Expr)) * (T(Var)[Lhs] * T(Array)[Array]) >>
        [](Match& _) {
          return Ref << _(Lhs) << (RefArgSeq << (RefArgBrack << *_[Array]));
        },

      (In(ObjectItemHead) / In(Expr)) *
          ((T(Ref) << (T(Var)[Lhs] * T(RefArgSeq)[RefArgSeq])) * T(Dot) *
           T(Var)[Rhs]) >>
        [](Match& _) {
          return Ref << _(Lhs)
                     << (RefArgSeq << *_[RefArgSeq] << (RefArgDot << _(Rhs)));
        },

      (In(ObjectItemHead) / In(Expr)) *
          ((T(Ref) << (T(Var)[Lhs] * T(RefArgSeq)[RefArgSeq])) *
           T(Array)[Array]) >>
        [](Match& _) {
          return Ref << _(Lhs)
                     << (RefArgSeq << *_[RefArgSeq]
                                   << (RefArgBrack << *_[Array]));
        },

      (In(ObjectItemHead) / In(Expr)) *
          ((T(Ref) << (T(Var)[Lhs] * T(RefArgSeq)[RefArgSeq])) *
           T(Paren)[Paren]) >>
        [](Match& _) {
          Node refargcall = NodeDef::create(RefArgCall);
          Node paren = _(Paren);
          if (paren->at(wfi / Paren / Paren)->type() == List)
          {
            for (const auto& arg : *paren->at(wfi / Paren / Paren))
            {
              refargcall->push_back(arg);
            }
          }
          else
          {
            refargcall->push_back(paren->at(wfi / Paren / Paren));
          }

          return Ref << _(Lhs) << (RefArgSeq << *_[RefArgSeq] << refargcall);
        },

      (In(ObjectItemHead) / In(Expr)) * (T(Var)[Var] * T(Paren)[Paren]) >>
        [](Match& _) {
          Node refargcall = NodeDef::create(RefArgCall);
          Node paren = _(Paren);
          if (paren->at(wfi / Paren / Paren)->type() == List)
          {
            for (const auto& arg : *paren->at(wfi / Paren / Paren))
            {
              refargcall->push_back(arg);
            }
          }
          else
          {
            refargcall->push_back(paren->at(wfi / Paren / Paren));
          }

          return Ref << _(Var) << (RefArgSeq << refargcall);
        },

      In(RefArgCall) * T(Group)[Group] >>
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
           << (T(Var)[Id] * T(UnifyBody)[Head] * T(UnifyBody)++[Tail])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp
                              << (AssignOperator << Assign)
                              << (Expr << (Term << (Scalar << JSONTrue)))))
                      << (RuleBodySeq << _(Head) << _[Tail]);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Paren)[Paren] * T(UnifyBody)[Head] *
               T(UnifyBody)++[Tail])) >>
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

          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadFunc
                              << args << (AssignOperator << Assign)
                              << (Expr << (Term << (Scalar << JSONTrue)))))
                      << (RuleBodySeq << _(Head) << _[Tail]);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Paren)[Paren] * (T(Assign) / T(Unify)) *
               ExprToken[Head] * ExprToken++[Tail] * T(UnifyBody)[Lhs] *
               T(UnifyBody)++[Rhs])) >>
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

          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << (Expr << _(Head) << _[Tail])))
                      << (RuleBodySeq << _(Lhs) << _[Rhs]);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Assign) / T(Unify)) * ExprToken[Head] *
               ExprToken++[Tail] * T(UnifyBody)[Lhs] * T(UnifyBody)++[Rhs])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Expr << _(Head) << _[Tail])))
                      << (RuleBodySeq << _(Lhs) << _[Rhs]);
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
                      << (RuleBodySeq << Empty);
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

      In(SomeDecl) * (T(Group) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

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

      In(RefArgSeq) * (T(RefArgCall)[RefArgCall] << End) >>
        [](Match& _) { return err(_(RefArgCall), "Call has no arguments"); },

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

      T(Group)[Group] >> [](Match& _) { return err(_(Group), "Syntax error"); },
    };
  }
}