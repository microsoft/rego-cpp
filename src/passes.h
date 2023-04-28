#pragma once

#include "lang.h"

namespace rego
{
  const inline auto BraceListItem = T(Group) / T(Assign);
  const inline auto LookupLhs = T(Ident) / T(Lookup);
  const inline auto LookupToken = T(Ident) / T(Square) / T(Dot);
  const inline auto Number = T(Int) / T(Float);
  const inline auto NotANumber =
    T(Object) / T(Array) / T(Bool) / T(String) / T(Null);
  const inline auto Result = Number / NotANumber / T(Undefined);
  inline const auto ExpressionArg = T(Expression) / Number / T(Ref) / T(Math);
  const inline auto CompOp = T(Equals) / T(NotEquals) / T(LessThan) /
    T(LessThanOrEquals) / T(GreaterThan) / T(GreaterThanOrEquals);
  const inline auto MathOp = T(Add) / T(Subtract) / T(Multiply) / T(Divide);
  const inline auto Truthy =
    Number / T(String) / T(Object) / T(Array) / T(Null);
  const inline auto Falsy = T(Undefined);
  const inline auto IndexArg = T(Expression) / T(Int) / T(Ref);

  PassDef input_data();
  PassDef modules();
  PassDef lists();
  PassDef structure();
  PassDef multiply_divide();
  PassDef add_subtract();
  PassDef comparison();
  PassDef merge_data();
  PassDef merge_modules();
  PassDef rules();
  PassDef convert_modules();
  PassDef query();
}