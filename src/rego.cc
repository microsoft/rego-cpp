#include "unify.hh"

namespace logging = trieste::logging;

namespace rego
{
  std::string set_log_level_from_string(const std::string& level)
  {
    return logging::set_log_level_from_string(level);
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
      default:
        throw std::runtime_error("Unknown log level");
    }
  }

  std::vector<Pass> passes(BuiltIns builtins)
  {
    auto reader_passes = reader().passes();
    auto unify_passes = unify(builtins).passes();
    std::vector<Pass> all_passes;
    all_passes.insert(
      all_passes.end(), reader_passes.begin(), reader_passes.end());
    all_passes.insert(
      all_passes.end(), unify_passes.begin(), unify_passes.end());
    return all_passes;
  }
}