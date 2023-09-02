#include "test_case.h"

#include <CLI/CLI.hpp>
#include <type_traits>

const std::string Green = "\x1b[32m";
const std::string Reset = "\x1b[0m";
const std::string Red = "\x1b[31m";
const std::string White = "\x1b[37m";

using TestCases = std::map<std::string, std::vector<rego_test::TestCase>>;

void load_testcases(
  const std::filesystem::path& path,
  const std::filesystem::path& debug_path,
  TestCases& testcases)
{
  std::vector<rego_test::TestCase> test_cases =
    rego_test::TestCase::load(path, debug_path);
  for (auto test_case : test_cases)
  {
    if (!testcases.contains(test_case.category()))
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
      std::cout << std::endl << file_or_dir << std::endl;
      load_testcase_dir(file_or_dir, debug_path, testcases);
    }
    else if (std::filesystem::exists(file_or_dir))
    {
      std::cout << ".";
      load_testcases(file_or_dir, debug_path, testcases);
    }
    else
    {
      std::cerr << "Not a file: " << file_or_dir << std::endl;
    }
  }
}

int main(int argc, char** argv)
{
  CLI::App app;

  app.set_help_all_flag("--help-all", "Expand all help");

  std::vector<std::filesystem::path> case_paths;
  app.add_option(
    "case,-c,--case", case_paths, "Test case YAML files or directories");

  std::filesystem::path debug_path;
  app.add_option(
    "-a,--ast", debug_path, "Output the AST (debugging for test case parser)");

  bool enable_logging{false};
  app.add_flag("-l,--logging", enable_logging, "Enable logging");

  bool fail_first{false};
  app.add_flag(
    "-f,--fail-first", fail_first, "Stop after first test case failure");

  std::string note_match;
  app.add_option(
    "-n,--note",
    note_match,
    "Note (or note substring) of specific test to run");

  try
  {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  rego::Logger::enabled = enable_logging;

  std::cout << "Loading test cases:";
  TestCases all_testcases;
  for (auto file_or_dir : case_paths)
  {
    if (std::filesystem::is_directory(file_or_dir))
    {
      std::cout << std::endl << file_or_dir << std::endl;
      load_testcase_dir(file_or_dir, debug_path, all_testcases);
    }
    else if (std::filesystem::exists(file_or_dir))
    {
      std::cout << ".";
      load_testcases(file_or_dir, debug_path, all_testcases);
    }
    else
    {
      std::cerr << "Not a file: " << file_or_dir << std::endl;
      return 1;
    }
  }

  std::cout << std::endl << "Done" << std::endl;

  int total = 0;
  int failures = 0;
  for (auto& [category, cat_cases] : all_testcases)
  {
    std::cout << White << category << std::endl;
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
        auto start = std::chrono::steady_clock::now();
        auto result = testcase.run(argv[0], debug_path);
        auto end = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = end - start;

        if (result.passed)
        {
          std::cout << Green << "  PASS: " << Reset << note << std::fixed
                    << std::setw(62 - note.length()) << std::internal
                    << std::setprecision(2) << elapsed.count() << " sec"
                    << std::endl;
        }
        else
        {
          failures++;
          std::cout << Red << "  FAIL: " << Reset << note << std::fixed
                    << std::setw(62 - note.length()) << std::internal
                    << std::setprecision(2) << elapsed.count() << " sec"
                    << std::endl;
          std::cout << "  " << result.error << std::endl;
          std::cout << "(from " << testcase.filename() << ")" << std::endl;
          if (fail_first)
          {
            break;
          }
        }
      }
      catch (const std::exception& e)
      {
        failures++;
        std::cout << Red << "  EXCEPTION: " << Reset << note << std::endl;
        std::cout << "  " << e.what() << std::endl;
        std::cout << "(from " << testcase.filename() << ")" << std::endl;
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

    std::cout << std::endl;
    std::cout << (total - failures) << " / " << total << " passed" << std::endl;

    std::cout << std::endl;
  }

  return failures;
}