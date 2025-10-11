#include "test_case.h"
#include "trieste/logging.h"

#include <CLI/CLI.hpp>
#include <type_traits>

const std::string Green = "\x1b[32m";
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
      logging::Output() << file_or_dir.path();
      load_testcase_dir(file_or_dir, debug_path, testcases);
    }
    else if (std::filesystem::exists(file_or_dir))
    {
      logging::Output() << ".";
      load_testcases(file_or_dir, debug_path, testcases);
    }
    else
    {
      logging::Error() << "Not a file: " << file_or_dir.path();
    }
  }
}

int manual_construction_test()
{
  auto input = rego::object({
    rego::object_item(
      rego::scalar("a"), rego::scalar(rego::BigInt((size_t)10L))),
    rego::object_item(rego::scalar("b"), rego::scalar("20")),
    rego::object_item(rego::scalar("c"), rego::scalar(30.0)),
    rego::object_item(rego::scalar("d"), rego::scalar(true)),
  });

  std::string query = "[input.a, input.b, input.c, input.d]";
  std::string expected = R"({"expressions":[[10,"20",30,true]]})";

  rego::Interpreter rego;

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

  if (note_match == "manual")
  {
    total++;
    if (manual_construction_test() != 0)
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

        if (result.passed)
        {
          logging::Output()
            << Green << "  PASS: " << Reset << note << std::fixed
            << std::setw(62 - note.length()) << std::internal
            << std::setprecision(3) << elapsed.count() << " sec";
        }
        else
        {
          failures++;
          logging::Error() << Red << "  FAIL: " << Reset << note << std::fixed
                           << std::setw(62 - note.length()) << std::internal
                           << std::setprecision(3) << elapsed.count() << " sec"
                           << std::endl
                           << "  " << result.error << std::endl
                           << "(from " << testcase.filename() << ")";
          if (fail_first)
          {
            break;
          }
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
                       << (total - failures) << " / " << total << " passed"
                       << std::endl;
    }
    else
    {
      logging::Output() << std::endl
                        << total << " / " << total << " passed" << std::endl;
    }
  }

  return failures;
}