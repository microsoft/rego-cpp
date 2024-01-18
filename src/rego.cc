#include "internal.hh"

namespace logging = trieste::logging;

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
      compr_locals(builtins),
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

  void set_log_level(LogLevel level)
  {
    // Set trieste LogLevel
    switch (level)
    {
      case LogLevel::None:
        logging::set_level<logging::None>();
        break;
      case LogLevel::Error:
        logging::set_level<logging::Error>();
        break;
      case LogLevel::Output:
        logging::set_level<logging::Output>();
        break;
      case LogLevel::Warn:
        logging::set_level<logging::Warn>();
        break;
      case LogLevel::Info:
        logging::set_level<logging::Info>();
        break;
      case LogLevel::Debug:
        logging::set_level<logging::Debug>();
        break;
      case LogLevel::Trace:
        logging::set_level<logging::Trace>();
        break;
    }
  }
}