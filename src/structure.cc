#include "passes.h"

namespace rego
{
  const inline auto TermToken = T(Var) / T(Ref) / T(Array) / T(Object) / T(Set);
  const inline auto ExprToken = T(Term) / ArithToken / BoolToken / T(Expr) /
    ScalarToken / TermToken / T(JSONString) / T(Array) / T(Set) / T(Object) /
    T(Paren) / T(Not) / T(Dot);
  const inline auto StringToken = T(JSONString) / T(RawString);

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

      In(Expr) * (T(Paren) << T(Group)[Group]) >>
        [](Match& _) { return Expr << *_[Group]; },

      In(Expr) * T(ObjectItemSeq)[ObjectItemSeq] >>
        [](Match& _) { return Object << *_[ObjectItemSeq]; },

      In(Array) * (T(Group) << T(Expr)[Expr]) >>
        [](Match& _) { return _(Expr); },

      In(RefArgBrack) * (T(Group) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

      In(RefArgBrack) * (T(Group) << ScalarToken[Value]) >>
        [](Match& _) { return Scalar << _(Value); },

      In(RefArgBrack) * (T(Group) << StringToken[Value]) >>
        [](Match& _) { return Scalar << (String << _(Value)); },

      In(RefArgBrack) * (T(Group) << T(Object)[Object]) >>
        [](Match& _) { return _(Object); },

      In(RefArgBrack) * (T(Group) << T(Array)[Array]) >>
        [](Match& _) { return _(Array); },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(RuleBody)[Head] * T(RuleBody)++[Tail])) >>
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
           << (T(Var)[Id] * T(Assign) * ExprToken[Head] * ExprToken++[Tail] *
               T(RuleBody)[Lhs] * T(RuleBody)++[Rhs])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Expr << _(Head) << _[Tail])))
                      << (RuleBodySeq << _(Lhs) << _[Rhs]);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Assign) * ExprToken[Head] * ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule
            << (RuleHead << _(Id)
                         << (RuleHeadComp << (AssignOperator << Assign)
                                          << (Expr << _(Head) << _[Tail])))
            << (RuleBodySeq
                << (RuleBody
                    << (Literal << (Expr << (Term << (Scalar << JSONTrue))))));
        },

      In(RuleBody) *
          (T(Group)
           << (T(Var)[Id] * T(Assign) * ExprToken[Head] * ExprToken++[Tail])) >>
        [](Match& _) {
          return RuleHead << _(Id)
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Expr << _(Head) << _[Tail]));
        },

      In(Policy) *
          (T(Group)
           << (T(Default) * T(Var)[Id] * T(Assign) * ScalarToken[Scalar])) >>
        [](Match& _) {
          return DefaultRule << _(Id) << (Term << (Scalar << _(Scalar)));
        },

      In(Policy) *
          (T(Group)
           << (T(Default) * T(Var)[Id] * T(Assign) * StringToken[String])) >>
        [](Match& _) {
          return DefaultRule << _(Id)
                             << (Term << (Scalar << (String << _(String))));
        },

      In(Policy) *
          (T(Group)
           << (T(Default) * T(Var)[Id] * T(Assign) *
               (T(Object) / T(Array) / T(Set))[Term])) >>
        [](Match& _) { return DefaultRule << _(Id) << (Term << _(Term)); },

      (In(Array) / In(Set)) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      In(RuleBody) * T(Group)[Group] >>
        [](Match& _) { return Literal << (Expr << *_[Group]); },

      T(Paren) << (T(Expr)[Expr] * End) >> [](Match& _) { return _(Expr); },

      In(Expr) * StringToken[String] >>
        [](Match& _) { return Term << (Scalar << (String << _(String))); },

      In(Expr) * ScalarToken[Value] >>
        [](Match& _) { return Term << (Scalar << _(Value)); },

      In(Expr) * TermToken[Value] >> [](Match& _) { return Term << _(Value); },

      // errors

      In(Policy) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Naked expression in policy"); },

      In(RefArgBrack) * (T(Expr) * T(Expr)[Expr]) >>
        [](Match& _) {
          return err(_(Expr), "Multi-dimensional indexing is not supported");
        },

      In(Expr) * (T(Assign) / T(Dot))[Value] >>
        [](Match& _) { return err(_(Value), "Invalid expression token"); },

      In(RefArgSeq) * (T(RefArgBrack)[RefArgBrack] << End) >>
        [](Match& _) { return err(_(RefArgBrack), "Missing index"); },

      In(Expr) * T(RuleBody)[RuleBody] >>
        [](Match& _) { return err(_(RuleBody), "Invalid expression"); },

      In(ObjectItemHead) * (!(T(Scalar) / T(Var) / T(Ref)))[Key] >>
        [](Match& _) { return err(_(Key), "Invalid object item key"); },

      In(Expr) * T(Default)[Default] >>
        [](Match& _) { return err(_(Default), "Invalid use of default"); },

      T(Group)[Group] >> [](Match& _) { return err(_(Group), "Syntax error"); },
    };
  }
}