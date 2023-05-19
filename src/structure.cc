#include "passes.h"

namespace rego
{
  const inline auto ScalarToken =
    T(JSONInt) / T(JSONFloat) / T(JSONTrue) / T(JSONFalse) / T(JSONNull);
  const inline auto TermToken = T(Var) / T(Ref) / T(Array) / T(Object) / T(Set);
  const inline auto ExprToken = T(Term) / ArithToken / BoolToken / T(Expr) /
    ScalarToken / TermToken / T(JSONString) / T(Array) / T(Set) / T(Object) /
    T(Paren);

  PassDef structure()
  {
    return {
      In(Query) * T(Group)[Group] >>
        [](Match& _) { return Literal << (Expr << *_[Group]); },

      (In(ObjectItemList) / In(Object)) *
          (T(ObjectItem)
           << ((T(Group) << T(JSONString)[JSONString]) * T(Group)[Group])) >>
        [](Match& _) {
          return ObjectItem << (Scalar << (String << _(JSONString)))
                            << (Expr << *_[Group]);
        },

      In(Expr) * (T(Var)[Lhs] * T(Dot) * T(Var)[Rhs]) >>
        [](Match& _) {
          return Ref << _(Lhs) << (RefArgSeq << (RefArgDot << _(Rhs)));
        },

      In(Expr) * (T(Var)[Lhs] * T(Array)[Array]) >>
        [](Match& _) {
          return Ref << _(Lhs) << (RefArgSeq << (RefArgBrack << *_[Array]));
        },

      In(Expr) *
          ((T(Ref) << (T(Var)[Lhs] * T(RefArgSeq)[RefArgSeq])) * T(Dot) *
           T(Var)[Rhs]) >>
        [](Match& _) {
          return Ref << _(Lhs)
                     << (RefArgSeq << *_[RefArgSeq] << (RefArgDot << _(Rhs)));
        },

      In(Expr) *
          ((T(Ref) << (T(Var)[Lhs] * T(RefArgSeq)[RefArgSeq])) *
           T(Array)[Array]) >>
        [](Match& _) {
          return Ref << _(Lhs)
                     << (RefArgSeq << *_[RefArgSeq]
                                   << (RefArgBrack << *_[Array]));
        },

      In(Expr) * (T(Paren) << T(Group)[Group]) >>
        [](Match& _) { return Expr << *_[Group]; },

      In(Expr) * T(ObjectItemList)[ObjectItemList] >>
        [](Match& _) { return Object << *_[ObjectItemList]; },

      In(Array) * (T(Group) << T(Expr)[Expr]) >>
        [](Match& _) { return _(Expr); },

      In(RefArgBrack) * (T(Group) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

      In(RefArgBrack) * (T(Group) << ScalarToken[Value]) >>
        [](Match& _) { return Scalar << _(Value); },

      In(RefArgBrack) * (T(Group) << T(JSONString)[Value]) >>
        [](Match& _) { return String << _(Value); },

      In(RefArgBrack) * (T(Group) << T(Object)[Object]) >>
        [](Match& _) { return _(Object); },

      In(RefArgBrack) * (T(Group) << T(Array)[Array]) >>
        [](Match& _) { return _(Array); },

      (In(Policy) / In(RuleBody)) *
          (T(Group) << (T(Var)[Id] * T(RuleBody)[RuleBody])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp
                              << (AssignOperator << Assign)
                              << (Expr << (Term << (Scalar << JSONTrue)))))
                      << (RuleBodySeq << _(RuleBody));
        },

      (In(Policy) / In(RuleBody)) *
          (T(Group)
           << (T(Var)[Id] * T(Assign) * ExprToken[Head] * ExprToken++[Tail] *
               T(RuleBody)[RuleBody])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Expr << _(Head) << _[Tail])))
                      << (RuleBodySeq << _(RuleBody));
        },

      (In(Policy) / In(RuleBody)) *
          (T(Group)
           << (T(Var)[Id] * T(Assign) * ExprToken[Head] * ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Expr << _(Head) << _[Tail])))
                      << (RuleBodySeq
                          << (RuleBody
                              << (Expr << (Term << (Scalar << JSONTrue)))));
        },

      (In(Array) / In(Set) / In(RuleBody)) * T(Group)[Group] >>
        [](Match& _) { return Expr << *_[Group]; },

      T(Paren) << (T(Expr)[Expr] * End) >> [](Match& _) { return _(Expr); },

      In(Expr) * T(JSONString)[JSONString] >>
        [](Match& _) { return Term << (Scalar << (String << _(JSONString))); },

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

      (In(Data) / In(Input) / In(DataSeq) / In(Policy) / In(Module) /
       In(ModuleSeq) / In(Group) / In(RefArgBrack)) *
          (Any * T(Error)[Error]) >>
        [](Match& _) { return _(Error); },

      (T(Data) / T(Input) / T(DataSeq) / T(Policy) / T(Module) / T(ModuleSeq) /
       T(Group) / T(RefArgBrack))
          << T(Error)[Error] >>
        [](Match& _) { return _(Error); },

      In(Expr) * (T(Assign) / T(Dot))[Value] >>
        [](Match& _) { return err(_(Value), "Invalid expression token"); },

      In(RefArgSeq) * (T(RefArgBrack)[RefArgBrack] << End) >>
        [](Match& _) { return err(_(RefArgBrack), "Missing index"); },

      In(Expr) * T(RuleBody)[RuleBody] >>
        [](Match& _) { return err(_(RuleBody), "Invalid expression"); },

      T(Group)[Group] >> [](Match& _) { return err(_(Group), "Syntax error"); },
    };
  }
}