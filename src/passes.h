#pragma once

#include "lang.h"

namespace rego
{
  const inline auto ScalarToken =
    T(JSONInt) / T(JSONFloat) / T(JSONTrue) / T(JSONFalse) / T(JSONNull);
  const inline auto ArithToken =
    T(Add) / T(Subtract) / T(Multiply) / T(Divide) / T(Modulo);
  const inline auto BoolToken = T(Equals) / T(NotEquals) / T(GreaterThan) /
    T(LessThan) / T(GreaterThanOrEquals) / T(LessThanOrEquals);
  const inline auto ArithInfixArg =
    T(Expr) / T(NumTerm) / T(Ref) / T(UnaryExpr) / T(ArithInfix) / T(RefTerm);

  PassDef input_data();
  PassDef modules();
  PassDef lists();
  PassDef structure();
  PassDef strings();
  PassDef symbols();
  PassDef multiply_divide();
  PassDef add_subtract();
  PassDef comparison();
  PassDef merge_data();
  PassDef merge_modules();
  PassDef rules();
  PassDef query();
}