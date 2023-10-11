#include "internal.hh"

namespace rego
{
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
    // Set trieste LogLevel
    switch (level)
    {
      case LogLevel::None:
        trieste::logging::set_level<trieste::logging::None>();
        break;
      case LogLevel::Error:
        trieste::logging::set_level<trieste::logging::Error>();
        break;
      case LogLevel::Warn:
        trieste::logging::set_level<trieste::logging::Warn>();
        break;
      case LogLevel::Info:
        trieste::logging::set_level<trieste::logging::Info>();
        break;
      case LogLevel::Debug:
        trieste::logging::set_level<trieste::logging::Debug>();
        break;
      case LogLevel::Trace:
        trieste::logging::set_level<trieste::logging::Trace>();
        break;
    }
  }
}