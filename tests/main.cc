#include "test_case.h"

const std::string Green = "\x1b[32m";
const std::string Reset = "\x1b[0m";
const std::string Red = "\x1b[31m";

int main(int argc, char** argv)
{
  CLI::App app;

  app.set_help_all_flag("--help-all", "Expand all help");

  std::vector<std::filesystem::path> case_paths;
  app.add_option("case,-c,--case", case_paths, "Test case YAML files");

  std::filesystem::path debug_path;
  app.add_option(
    "-a,--ast", debug_path, "Output the AST (debugging for test case parser)");

  try
  {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  int failures = 0;
  for (auto path : case_paths)
  {
    std::vector<rego_test::TestCase> test_cases;
    auto maybe_test_cases = rego_test::TestCase::load(path, debug_path);
    if (maybe_test_cases.has_value())
    {
      test_cases = *maybe_test_cases;
    }
    else
    {
      std::cout << "Unable to parse test cases in " << path << std::endl;
      continue;
    }

    std::cout << path << std::endl;

    for (auto& test_case : test_cases)
    {
      try
      {
        auto start = std::chrono::steady_clock::now();
        auto result = test_case.run(argv[0]);
        auto end = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = end - start;

        if (result.passed)
        {
          std::cout << Green << "  PASS: " << Reset << test_case.note()
                    << std::fixed << std::setw(62 - test_case.note().length())
                    << std::internal << std::setprecision(2) << elapsed.count()
                    << " sec" << std::endl;
        }
        else
        {
          failures++;
          std::cout << std::setw(70) << Red << "  FAIL: " << Reset
                    << test_case.note() << std::fixed
                    << std::setw(62 - test_case.note().length())
                    << std::internal << std::setprecision(2) << elapsed.count()
                    << " sec" << std::endl;
          std::cout << "  " << result.error << std::endl;
        }
      }
      catch (const std::exception& e)
      {
        failures++;
        std::cout << std::setw(70) << Red << "  FAIL: " << Reset
                  << test_case.note() << std::endl;
        std::cout << "  " << e.what() << std::endl;
        continue;
      }
    }

    std::cout << std::endl;
  }

  return failures;
}