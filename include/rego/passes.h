#pragma once

#include "lang.h"

namespace rego
{
  const inline auto ScalarToken =
    T(JSONInt) / T(JSONFloat) / T(JSONTrue) / T(JSONFalse) / T(JSONNull);
  const inline auto ArithToken =
    T(Add) / T(Subtract) / T(Multiply) / T(Divide) / T(Modulo);
  const inline auto ArithInfixArg = T(Expr) / T(NumTerm) / T(Ref) /
    T(UnaryExpr) / T(ArithInfix) / T(RefTerm) / T(ExprCall);
  const inline auto BinInfixArg = T(Expr) / T(Ref) / T(RefTerm) / T(ExprCall) /
    T(Set) / T(SetCompr) / T(BinInfix);
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack);
  const inline auto BoolToken = T(Equals) / T(NotEquals) / T(GreaterThan) /
    T(LessThan) / T(GreaterThanOrEquals) / T(LessThanOrEquals);
  const inline auto TermToken = T(Var) / T(Ref) / T(Array) / T(Object) /
    T(Set) / T(ArrayCompr) / T(ObjectCompr) / T(SetCompr);
  const inline auto StringToken = T(JSONString) / T(RawString);
  const inline auto ExprToken = T(Term) / ArithToken / BoolToken / StringToken /
    T(Expr) / ScalarToken / TermToken / T(JSONString) / T(Array) / T(Set) /
    T(Object) / T(Paren) / T(Not) / T(Dot) / T(MemberOf) / T(And) / T(Or);

  PassDef input_data();
  PassDef modules();
  PassDef imports();
  PassDef keywords();
  PassDef lists();
  PassDef ifs();
  PassDef elses();
  PassDef rules();
  PassDef build_calls();
  PassDef build_refs();
  PassDef structure();
  PassDef strings();
  PassDef merge_data();
  PassDef symbols();
  PassDef replace_argvals();
  PassDef lift_query();
  PassDef constants();
  PassDef explicit_enums();
  PassDef body_locals();
  PassDef value_locals();
  PassDef compr();
  PassDef absolute_refs();
  PassDef merge_modules();
  PassDef skips();
  PassDef multiply_divide();
  PassDef add_subtract();
  PassDef comparison();
  PassDef assign();
  PassDef skip_refs();
  PassDef simple_refs();
  PassDef implicit_enums();
  PassDef init();
  PassDef rulebody();
  PassDef lift_to_rule();
  PassDef functions();
  PassDef unify();
  PassDef query();
}