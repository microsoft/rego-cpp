#include "internal.hh"

namespace rego
{
  LogLevel Logger::maximum_level = LogLevel::None;
  std::string Logger::indent = "";

  std::vector<Pass> passes(const BuiltIns& builtins)
  {
    return {
      input_data(),
      modules(),
      imports(),
      keywords(),
      lists(),
      ifs(),
      elses(),
      rules(),
      build_calls(),
      membership(),
      build_refs(),
      structure(),
      strings(),
      merge_data(),
      lift_refheads(),
      symbols(),
      replace_argvals(),
      lift_query(),
      expand_imports(),
      constants(),
      explicit_enums(),
      body_locals(builtins),
      value_locals(builtins),
      rules_to_compr(),
      compr(),
      absolute_refs(),
      merge_modules(),
      datarule(),
      skips(),
      unary(),
      multiply_divide(),
      add_subtract(),
      comparison(),
      assign(builtins),
      skip_refs(builtins),
      simple_refs(),
      init(),
      implicit_enums(),
      enum_locals(),
      rulebody(),
      lift_to_rule(),
      functions(),
      unify(builtins),
      query(),
    };
  }

  Driver& driver(const BuiltIns& builtins)
  {
    static Driver d("rego", nullptr, parser(), passes(builtins));
    return d;
  }

  void set_log_level(LogLevel level)
  {
    Logger::maximum_level = level;
  }
}