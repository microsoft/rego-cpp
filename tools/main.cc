#include <CLI/CLI.hpp>
#include <rego/rego.hh>
#include <trieste/logging.h>

int main(int argc, char** argv)
{
  std::cout << "rego " << REGOCPP_VERSION << " (" << REGOCPP_BUILD_NAME << ", "
            << REGOCPP_BUILD_DATE << ")"
            << "[" << REGOCPP_BUILD_TOOLCHAIN << "] on " << REGOCPP_PLATFORM
            << std::endl;
  CLI::App app;

  app.set_help_all_flag("--help-all", "Expand all help");

  std::string query_expr;
  app.add_option("query,-q,--query", query_expr, "Query")->required();

  std::vector<std::filesystem::path> data_paths;
  app.add_option("-d,--data", data_paths, "Data/Policy files");

  std::filesystem::path input_path;
  app.add_option("-i,--input", input_path, "Input JSON file");

  bool wf_checks{false};
  app.add_flag("-w,--wf", wf_checks, "Enable well-formedness checks");

  std::filesystem::path output;
  app.add_option("-a,--ast", output, "Folder to use for AST output");

  std::string log_level;
  app
    .add_option(
      "-l,--log_level",
      log_level,
      "Set Log Level to one of "
      "Trace, Debug (includes log of unification),"
      "Info, Warning, Output, Error, "
      "None")
    ->check(rego::set_log_level_from_string);

  bool v1_compatible{false};
  app.add_flag(
    "-1,--v1-compatible", v1_compatible, "Use Rego v1 compatibility mode");

  try
  {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  auto interpreter = rego::Interpreter(v1_compatible);
  interpreter.wf_check_enabled(wf_checks);
  if (!output.empty())
  {
    interpreter.debug_enabled(true);
    interpreter.debug_path(output);
  }

  if (!input_path.empty())
  {
    if (!std::filesystem::exists(input_path))
    {
      trieste::logging::Error() << std::filesystem::weakly_canonical(input_path)
                                << " does not exist" << std::endl;
      return 1;
    }

    rego::Node result;
    try
    {
      result = interpreter.set_input_json_file(input_path);
    }
    catch (const std::exception& e)
    {
      trieste::logging::Error() << e.what() << std::endl;
      return 1;
    }

    if (result != nullptr)
    {
      trieste::logging::Error()
        << "Invalid input file: "
        << std::filesystem::weakly_canonical(input_path) << std::endl;
      return 1;
    }
  }

  for (auto& path : data_paths)
  {
    if (!std::filesystem::exists(path))
    {
      trieste::logging::Error() << std::filesystem::weakly_canonical(path)
                                << " does not exist" << std::endl;
      return 1;
    }

    try
    {
      rego::Node result;
      if (path.extension() == ".json")
      {
        result = interpreter.add_data_json_file(path);
      }
      else
      {
        result = interpreter.add_module_file(path);
      }

      if (result != nullptr)
      {
        trieste::logging::Error()
          << "Invalid data file:" << std::filesystem::weakly_canonical(path)
          << std::endl;
        return 1;
      }
    }
    catch (const std::exception& e)
    {
      trieste::logging::Error() << e.what() << std::endl;
      return 1;
    }
  }

  try
  {
    trieste::logging::Output() << interpreter.query(query_expr) << std::endl;
    return 0;
  }
  catch (const std::exception& e)
  {
    trieste::logging::Error() << e.what() << std::endl;
    return 1;
  }
}
