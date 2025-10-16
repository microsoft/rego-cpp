#include "trieste/logging.h"

#include <CLI/CLI.hpp>
#include <rego/rego.hh>
#include <trieste/fuzzer.h>

using namespace trieste;

int main(int argc, char** argv)
{
  CLI::App app;

  app.set_help_all_flag("--help-all", "Expand all help");

  std::string transform;
  app.add_option("transform", transform, "Transform to test")
    ->check(CLI::IsMember(
      {"file_to_rego", "json_to_bundle", "rego_to_bundle", "bundle_to_json"}))
    ->required(true);

  uint32_t seed = std::random_device()();
  app.add_option("-s,--seed", seed, "Random seed");

  uint32_t count = 100;
  app.add_option("-c,--count", count, "Number of seed to test");

  bool failfast = false;
  app.add_flag("-f,--failfast", failfast, "Stop on first failure");

  std::string log_level;
  app
    .add_option(
      "-l,--log_level",
      log_level,
      "Set Log Level to one of "
      "Trace, Debug (includes log of unification),"
      "Info, Warning, Output, Error, "
      "None")
    ->check(logging::set_log_level_from_string);

  try
  {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  logging::Output() << "Testing x" << count << ", seed: " << seed << std::endl;

  Fuzzer fuzzer;
  Reader reader = rego::file_to_rego();
  if (transform == "file_to_rego")
  {
    fuzzer = Fuzzer(reader);
  }
  else if (transform == "json_to_bundle")
  {
    auto builtins = rego::BuiltInsDef::create();
    fuzzer = Fuzzer(rego::json_to_bundle(), reader.parser().generators());
  }
  else if (transform == "rego_to_bundle")
  {
    fuzzer = Fuzzer(rego::rego_to_bundle(), reader.parser().generators());
  }
  else if (transform == "bundle_to_json")
  {
    fuzzer = Fuzzer(rego::bundle_to_json(), reader.parser().generators());
  }

  return fuzzer.start_seed(seed).seed_count(count).failfast(failfast).test();
}
