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
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack);
  const inline auto BoolToken = T(Equals) / T(NotEquals) / T(GreaterThan) /
    T(LessThan) / T(GreaterThanOrEquals) / T(LessThanOrEquals);
  const inline auto TermToken = T(Var) / T(Ref) / T(Array) / T(Object) / T(Set);
  const inline auto StringToken = T(JSONString) / T(RawString);
  const inline auto ExprToken = T(Term) / ArithToken / BoolToken / StringToken /
    T(Expr) / ScalarToken / TermToken / T(JSONString) / T(Array) / T(Set) /
    T(Object) / T(Paren) / T(Not) / T(Dot);

  PassDef input_data();
  PassDef modules();
  PassDef lists();
  PassDef imports();
  PassDef keywords();
  PassDef if_else();
  PassDef structure();
  PassDef strings();
  PassDef symbols();
  PassDef locals();
  PassDef multiply_divide();
  PassDef add_subtract();
  PassDef comparison();
  PassDef assign();
  PassDef refs();
  PassDef rulebody();
  PassDef functions();
  PassDef merge_data();
  PassDef merge_modules();
  PassDef rules();
  PassDef query();
}