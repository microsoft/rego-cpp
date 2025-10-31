#include "rego/rego.hh"
#include "rego/rego_c.h"
#include "test_case.h"
#include "trieste/logging.h"

#include <CLI/CLI.hpp>
#include <stdexcept>

const std::string Green = "\x1b[32m";
const std::string Cyan = "\x1b[36m";
const std::string Reset = "\x1b[0m";
const std::string Red = "\x1b[31m";
const std::string White = "\x1b[37m";

using TestCases = std::map<std::string, std::vector<rego_test::TestCase>>;
namespace logging = trieste::logging;

void load_testcases(
  const std::filesystem::path& path,
  const std::filesystem::path& debug_path,
  TestCases& testcases)
{
  std::vector<rego_test::TestCase> test_cases =
    rego_test::TestCase::load(path, debug_path);
  for (auto test_case : test_cases)
  {
    if (testcases.find(test_case.category()) == testcases.end())
    {
      testcases[test_case.category()] = std::vector<rego_test::TestCase>();
    }
    testcases[test_case.category()].push_back(test_case);
  }
}

void load_testcase_dir(
  const std::filesystem::path& dir,
  const std::filesystem::path& debug_path,
  TestCases& testcases)
{
  for (auto& file_or_dir : std::filesystem::directory_iterator(dir))
  {
    if (std::filesystem::is_directory(file_or_dir))
    {
      std::cout << std::endl << file_or_dir.path();
      load_testcase_dir(file_or_dir, debug_path, testcases);
    }
    else if (std::filesystem::exists(file_or_dir))
    {
      std::cout << ".";
      load_testcases(file_or_dir, debug_path, testcases);
    }
    else
    {
      logging::Error() << "Not a file: " << file_or_dir.path();
    }
  }
}

int manual_construction_test(
  const std::filesystem::path& debug_path,
  bool wf_checks,
  rego::LogLevel log_level)
{
  rego::Interpreter rego;
  rego.log_level(log_level).wf_check_enabled(wf_checks);
  if (!debug_path.empty())
  {
    rego.debug_enabled(true).debug_path(debug_path);
  }
  auto input = rego::object({
    rego::object_item(rego::string("a"), rego::number(10)),
    rego::object_item(rego::string("b"), rego::string("20")),
    rego::object_item(rego::string("c"), rego::number(30.0)),
    rego::object_item(rego::string("d"), rego::boolean(true)),
  });

  std::string query = "[input.a, input.b, input.c, input.d]";
  std::string expected = R"({"expressions":[[10,"20",30,true]]})";

  auto start = std::chrono::steady_clock::now();
  rego.set_input(input);
  std::string actual = rego.query(query);
  auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double> elapsed = end - start;
  std::string note = "manual construction test";

  if (actual == expected)
  {
    logging::Output() << Green << "  PASS: " << Reset << note << std::fixed
                      << std::setw(62 - note.length()) << std::internal
                      << std::setprecision(3) << elapsed.count() << " sec";
    return 0;
  }

  logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                   << std::setw(62 - note.length()) << std::internal
                   << std::setprecision(3) << elapsed.count() << " sec"
                   << std::endl
                   << "  Expected: " << expected << std::endl
                   << "  Actual: " << actual << std::endl;
  return 1;
}

rego_test::TestResult output_test(rego::Interpreter& rego)
{
  rego::Output output = rego.query_output("invalid^@rego");
  if (output.ok())
  {
    return {rego_test::Outcome::Fail, "Expected output to be invalid"};
  }

  try
  {
    output.expressions();
    return {rego_test::Outcome::Fail, "Expected logic_error"};
  }
  catch (const std::logic_error&)
  {}

  try
  {
    output.binding("x");
    return {rego_test::Outcome::Fail, "Expected logic_error"};
  }
  catch (const std::logic_error&)
  {}

  output = rego.query_output("a = [1, 2, 3, 4]; x = a[_]; x % 2 == 0; x / 2");
  std::ostringstream error;
  if (output.size() != 2)
  {
    error << "Output is not the correct size: " << output.size() << " != " << 2;
    return {rego_test::Outcome::Fail, error.str()};
  }

  try
  {
    output.expressions_at(2);
    return {rego_test::Outcome::Fail, "expected an out_of_range error"};
  }
  catch (const std::out_of_range&)
  {}

  rego::Node expressions0 = output.expressions();
  if (expressions0->size() != 4)
  {
    error << "Expressions@0 is the wrong size: " << expressions0->size()
          << " != " << 4;
    return {rego_test::Outcome::Fail, error.str()};
  }

  rego::Node expressions1 = output.expressions_at(1);
  if (expressions1->size() != 4)
  {
    error << "Expressions@1 is the wrong size: " << expressions0->size()
          << " != " << 4;
    return {rego_test::Outcome::Fail, error.str()};
  }

  rego::Node last0 = expressions0->back();
  std::string actual = rego::to_key(last0);
  std::string expected = "1";
  if (actual != expected)
  {
    error << "Expressions@0[3]: " << actual << " != " << expected;
    return {rego_test::Outcome::Fail, error.str()};
  }

  rego::Node last1 = expressions1->back();
  actual = rego::to_key(last1);
  expected = "2";
  if (actual != expected)
  {
    error << "Expressions@1[3]: " << actual << " != " << expected;
    return {rego_test::Outcome::Fail, error.str()};
  }

  try
  {
    output.binding_at(2, "a");
    return {rego_test::Outcome::Fail, "expected an out_of_range error"};
  }
  catch (const std::out_of_range&)
  {}

  try
  {
    output.binding("b");
    return {rego_test::Outcome::Fail, "expected an invalid_argument error"};
  }
  catch (const std::invalid_argument&)
  {}

  rego::Node a = output.binding("a");
  actual = rego::to_key(a);
  expected = "[1,2,3,4]";
  if (actual != expected)
  {
    error << "a@0: " << actual << " != " << expected;
    return {rego_test::Outcome::Fail, error.str()};
  }

  rego::Node x_node = output.binding("x");
  auto maybe_int = rego::try_get_int(x_node);
  if (!maybe_int.has_value())
  {
    error << "x@0 is not an integer: " << x_node;
    return {rego_test::Outcome::Fail, error.str()};
  }

  if (maybe_int.value().to_int() != 2)
  {
    error << "x@0: " << maybe_int.value() << " != " << 2;
    return {rego_test::Outcome::Fail, error.str()};
  }

  x_node = output.binding_at(1, "x");
  maybe_int = rego::try_get_int(x_node);
  if (!maybe_int.has_value())
  {
    error << "x@1 is not an integer: " << x_node;
    return {rego_test::Outcome::Fail, error.str()};
  }

  if (maybe_int.value().to_int() != 4)
  {
    error << "x@1: " << maybe_int.value() << " != " << 4;
    return {rego_test::Outcome::Fail, error.str()};
  }

  return {rego_test::Outcome::Pass, ""};
}

int manual_output_test(
  const std::filesystem::path& debug_path,
  bool wf_checks,
  rego::LogLevel log_level)
{
  rego::Interpreter rego;
  rego.log_level(log_level).wf_check_enabled(wf_checks);
  if (!debug_path.empty())
  {
    rego.debug_enabled(true).debug_path(debug_path);
  }

  auto start = std::chrono::steady_clock::now();
  auto result = output_test(rego);
  auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double> elapsed = end - start;
  std::string note = "manual output test";

  if (result.outcome == rego_test::Outcome::Pass)
  {
    logging::Output() << Green << "  PASS: " << Reset << note << std::fixed
                      << std::setw(62 - note.length()) << std::internal
                      << std::setprecision(3) << elapsed.count() << " sec";
    return 0;
  }

  logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                   << std::setw(62 - note.length()) << std::internal
                   << std::setprecision(3) << elapsed.count() << " sec"
                   << std::endl
                   << "  Error: " << result.error;
  return 1;
}

int manual_timeout_test(
  const std::filesystem::path& debug_path,
  bool wf_checks,
  rego::LogLevel log_level)
{
  rego::Interpreter rego;
  rego.log_level(log_level).wf_check_enabled(wf_checks);
  if (!debug_path.empty())
  {
    rego.debug_enabled(true).debug_path(debug_path);
  }

  rego.stmt_limit(10);

  auto start = std::chrono::steady_clock::now();
  auto output =
    rego.query_output("a = [1, 2, 3, 4]; x = a[_]; x % 2 == 0; x / 2");
  auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double> elapsed = end - start;
  std::string note = "manual timeout test";

  if (output.ok())
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  timeout did not fire";
    return 1;
  }

  std::string message = output.errors().front();
  if (message.find("Maximum statement count reached") == std::string::npos)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  Incorrect error: " << message;
    return 1;
  }

  logging::Output() << Green << "  PASS: " << Reset << note << std::fixed
                    << std::setw(62 - note.length()) << std::internal
                    << std::setprecision(3) << elapsed.count() << " sec";
  return 0;
}

int manual_whitelist_test()
{
  auto start = std::chrono::steady_clock::now();
  rego::BuiltIns builtins = rego::BuiltInsDef::create();
  builtins->whitelist({"plus", "minus"});
  auto plus_builtin = builtins->at({"plus"});
  auto mul_builtin = builtins->at({"mul"});
  std::vector<rego::Location> allowed({{"plus"}, {"minus"}});
  builtins->whitelist(allowed.begin(), allowed.end());
  auto minus_builtin = builtins->at({"plus"});
  auto div_builtin = builtins->at({"mul"});
  auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double> elapsed = end - start;
  std::string note = "manual whitelist test";

  if (
    plus_builtin == nullptr || minus_builtin == nullptr ||
    div_builtin == nullptr || mul_builtin == nullptr)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  builtin lookup returning nullptr for valid builtins";
    return 1;
  }

  if (!plus_builtin->available)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  plus builtin not available";
    return 1;
  }

  if (!minus_builtin->available)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << " minus builtin not available";
    return 1;
  }

  if (mul_builtin->available)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  mul builtin available";
    return 1;
  }

  if (div_builtin->available)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  div builtin available";
    return 1;
  }

  logging::Output() << Green << "  PASS: " << Reset << note << std::fixed
                    << std::setw(62 - note.length()) << std::internal
                    << std::setprecision(3) << elapsed.count() << " sec";
  return 0;
}

int manual_blacklist_test()
{
  auto start = std::chrono::steady_clock::now();
  rego::BuiltIns builtins = rego::BuiltInsDef::create();
  builtins->blacklist({"plus", "minus"});
  auto plus_builtin = builtins->at({"plus"});
  auto mul_builtin = builtins->at({"mul"});
  std::vector<rego::Location> allowed({{"plus"}, {"minus"}});
  builtins->blacklist(allowed.begin(), allowed.end());
  auto minus_builtin = builtins->at({"plus"});
  auto div_builtin = builtins->at({"mul"});
  auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double> elapsed = end - start;
  std::string note = "manual blacklist test";

  if (
    plus_builtin == nullptr || minus_builtin == nullptr ||
    div_builtin == nullptr || mul_builtin == nullptr)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  builtin lookup returning nullptr for valid builtins";
    return 1;
  }

  if (plus_builtin->available)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  plus builtin available";
    return 1;
  }

  if (minus_builtin->available)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << " minus builtin available";
    return 1;
  }

  if (!mul_builtin->available)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  mul builtin not available";
    return 1;
  }

  if (!div_builtin->available)
  {
    logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                     << std::setw(62 - note.length()) << std::internal
                     << std::setprecision(3) << elapsed.count() << " sec"
                     << std::endl
                     << "  div builtin not available";
    return 1;
  }

  logging::Output() << Green << "  PASS: " << Reset << note << std::fixed
                    << std::setw(62 - note.length()) << std::internal
                    << std::setprecision(3) << elapsed.count() << " sec";
  return 0;
}

int main(int argc, char** argv)
{
  CLI::App app;

  app.set_help_all_flag("--help-all", "Expand all help");

  std::vector<std::filesystem::path> case_paths;
  app.add_option(
    "case,-c,--case", case_paths, "Test case YAML files or directories");

  std::filesystem::path debug_path;
  app.add_option("-a,--ast", debug_path, "Folder to use for AST output");

  std::string log_level_str;
  app.add_option("-l,--log_level", log_level_str, "Set Log Level")
    ->check(CLI::IsMember(
      {"Trace", "Debug", "Info", "Warning", "Output", "Error", "None"}));

  bool wf_checks{false};
  app.add_flag("-w,--wf", wf_checks, "Enable well-formedness checks (slow)");

  bool fail_first{false};
  app.add_flag(
    "-f,--fail-first", fail_first, "Stop after first test case failure");

  std::string note_match;
  app.add_option(
    "-n,--note",
    note_match,
    "Note (or note substring) of specific test to run");

  std::string roundtrip_str = "none";
  std::set<std::string> roundtrip_values({"none", "json", "binary"});
  app
    .add_option(
      "-r,--roundtrip",
      roundtrip_str,
      "Whether to perform a serialization before performing the test (slow)")
    ->check(CLI::IsMember(roundtrip_values));

  try
  {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  rego_test::RoundTrip roundtrip;
  if (roundtrip_str == "none")
  {
    roundtrip = rego_test::RoundTrip::None;
  }
  else if (roundtrip_str == "json")
  {
    roundtrip = rego_test::RoundTrip::JSON;
  }
  else
  {
    roundtrip = rego_test::RoundTrip::Binary;
  }

  rego::LogLevel log_level = rego::LogLevel::Output;
  if (!log_level_str.empty())
  {
    log_level = rego::log_level_from_string(log_level_str);
  }

  trieste::logging::set_log_level_from_string(log_level_str);

  logging::Output() << "Loading test cases:";
  TestCases all_testcases;
  for (auto file_or_dir : case_paths)
  {
    if (std::filesystem::is_directory(file_or_dir))
    {
      logging::Output() << file_or_dir;
      load_testcase_dir(file_or_dir, debug_path, all_testcases);
    }
    else if (std::filesystem::exists(file_or_dir))
    {
      logging::Output() << ".";
      load_testcases(file_or_dir, debug_path, all_testcases);
    }
    else
    {
      logging::Error() << "Not a file: " << file_or_dir;
      return 1;
    }
  }

  logging::Output() << "Done";

  int total = 0;
  int failures = 0;
  int skipped = 0;

  if (note_match == "manual")
  {
    total += 5;
    if (manual_construction_test(debug_path, wf_checks, log_level) != 0)
    {
      failures++;
    }

    if (manual_output_test(debug_path, wf_checks, log_level) != 0)
    {
      failures++;
    }

    if (manual_whitelist_test())
    {
      failures++;
    }

    if (manual_blacklist_test())
    {
      failures++;
    }

    if (manual_timeout_test(debug_path, wf_checks, log_level))
    {
      failures++;
    }
  }

  for (auto& [category, cat_cases] : all_testcases)
  {
    logging::Output() << White << category << std::endl;
    for (auto& testcase : cat_cases)
    {
      if (
        !note_match.empty() &&
        testcase.note().find(note_match) == std::string::npos)
      {
        continue;
      }

      total++;
      std::string note = testcase.note();
      if (category.size() > 0)
      {
        note = note.substr(category.size() + 1);
      }

      try
      {
        logging::Info() << "Test " << testcase.note() << " from "
                        << testcase.filename();
        auto start = std::chrono::steady_clock::now();
        auto result = testcase.run(debug_path, wf_checks, roundtrip, log_level);
        auto end = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = end - start;

        switch (result.outcome)
        {
          case rego_test::Outcome::Pass:
            logging::Output()
              << Green << "  PASS: " << Reset << note << std::fixed
              << std::setw(62 - note.length()) << std::internal
              << std::setprecision(3) << elapsed.count() << " sec";
            break;

          case rego_test::Outcome::Skip:
            skipped++;
            total--;
            logging::Output()
              << Cyan << "  SKIP: " << Reset << note << " (" << result.error
              << ")" << std::fixed << std::setw(62 - note.length())
              << std::internal << std::setprecision(3) << elapsed.count()
              << " sec";
            break;

          case rego_test::Outcome::Fail:
            failures++;
            logging::Error()
              << Red << "  FAIL: " << Reset << note << std::fixed
              << std::setw(62 - note.length()) << std::internal
              << std::setprecision(3) << elapsed.count() << " sec" << std::endl
              << "  " << result.error << std::endl
              << "(from " << testcase.filename() << ")";
            break;
        }

        if (result.outcome == rego_test::Outcome::Fail && fail_first)
        {
          break;
        }
      }
      catch (const std::exception& e)
      {
        failures++;
        logging::Error() << Red << "  EXCEPTION: " << Reset << note << std::endl
                         << "  " << e.what() << std::endl
                         << "(from " << testcase.filename() << ")" << std::endl;
        if (fail_first)
        {
          break;
        }
      }
    }
    if (failures > 0 && fail_first)
    {
      break;
    }

    if (failures != 0)
    {
      logging::Error() << std::endl
                       << (total - failures) << " / " << total << " passed";
    }
    else
    {
      logging::Output() << std::endl << total << " / " << total << " passed";
    }

    if (skipped != 0)
    {
      logging::Output() << '(' << skipped << " skipped)" << std::endl;
    }
  }

  return failures;
}