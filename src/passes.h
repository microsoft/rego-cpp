#pragma once

#include "lang.h"

namespace rego
{
  const inline auto ArithToken = T(Add) / T(Subtract) / T(Multiply) / T(Divide);
  const inline auto BoolToken = T(Equals) / T(NotEquals) / T(GreaterThan) /
    T(LessThan) / T(GreaterThanOrEquals) / T(LessThanOrEquals);

  const inline auto ArithInfixArg =
    T(Expr) / T(NumTerm) / T(Ref) / T(UnaryExpr) / T(ArithInfix) / T(RefTerm);

  // const inline auto LookupLhs = T(Ident) / T(Lookup);
  // const inline auto LookupToken = T(Ident) / T(Square) / T(Dot);
  // const inline auto Number = T(Int) / T(Float);
  // const inline auto NotANumber =
  //   T(Object) / T(Array) / T(Bool) / T(String) / T(Null);
  // const inline auto Result = Number / NotANumber / T(Undefined);
  // const inline auto MathOp = T(Add) / T(Subtract) / T(Multiply) / T(Divide);
  // const inline auto IndexArg = T(Expression) / T(Int) / T(Ref);

  PassDef input_data();
  PassDef modules();
  PassDef lists();
  PassDef structure();
  PassDef symbols();
  PassDef multiply_divide();
  PassDef add_subtract();
  PassDef comparison();
  PassDef merge_data();
  PassDef merge_modules();
  PassDef rules();
  PassDef query();
}