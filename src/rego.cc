#include "internal.hh"

namespace logging = trieste::logging;

namespace rego
{
  std::vector<Pass> passes(BuiltIns builtins)
  {
    auto reader_passes = file_to_rego().passes();
    auto bundle_passes = rego_to_bundle(builtins).passes();
    std::vector<Pass> all_passes;
    all_passes.insert(
      all_passes.end(), reader_passes.begin(), reader_passes.end());
    all_passes.insert(
      all_passes.end(), bundle_passes.begin(), bundle_passes.end());
    return all_passes;
  }

  LogLevel log_level_from_string(const std::string& s)
  {
    std::string name;
    name.resize(s.size());
    std::transform(s.begin(), s.end(), name.begin(), ::tolower);
    if (name == "none")
      return LogLevel::None;
    if (name == "error")
      return LogLevel::Error;
    if (name == "output")
      return LogLevel::Output;
    if (name == "warn")
      return LogLevel::Warn;
    if (name == "info")
      return LogLevel::Info;
    if (name == "debug")
      return LogLevel::Debug;
    if (name == "trace")
      return LogLevel::Trace;

    std::stringstream ss;
    ss << "Unknown log level: " << s
       << " should be on of None, Error, Output, Warn, Info, Debug, Trace";
    throw std::runtime_error(ss.str());
  }
}