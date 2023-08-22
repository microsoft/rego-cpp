#pragma once

#include <trieste/pass.h>

namespace rego
{
  PassDef input_data();
  PassDef modules();
  PassDef imports();
  PassDef keywords();
  PassDef lists();
  PassDef ifs();
  PassDef elses();
  PassDef rules();
  PassDef build_calls();
  PassDef membership();
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
  PassDef unary();
  PassDef multiply_divide();
  PassDef add_subtract();
  PassDef comparison();
  PassDef assign(const BuiltIns& builtins);
  PassDef skip_refs(const BuiltIns& builtins);
  PassDef simple_refs();
  PassDef implicit_enums();
  PassDef init();
  PassDef rulebody();
  PassDef lift_to_rule();
  PassDef functions();
  PassDef unify(const BuiltIns& builtins);
  PassDef query();
}