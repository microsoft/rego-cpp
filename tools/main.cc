#include <CLI/CLI.hpp>
#include <rego/interpreter.h>

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
  app.add_option("-a,--ast", output, "Output the AST");

  bool enable_logging{false};
  app.add_flag("-l,--logging", enable_logging, "Enable logging");

  try
  {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  rego::Logger::enabled = enable_logging;

  auto interpreter = rego::Interpreter();
  interpreter.executable(argv[0]);
  interpreter.well_formed_checks_enabled(wf_checks);

  if (!input_path.empty())
  {
    if (!std::filesystem::exists(input_path))
    {
      std::cerr << std::filesystem::weakly_canonical(input_path)
                << " does not exist" << std::endl;
      return 1;
    }

    interpreter.set_input_json_file(input_path);
  }

  for (auto& path : data_paths)
  {
    if (!std::filesystem::exists(path))
    {
      std::cerr << std::filesystem::weakly_canonical(path) << " does not exist"
                << std::endl;
      return 1;
    }

    if (path.extension() == ".json")
    {
      interpreter.add_data_json_file(path);
    }
    else
    {
      interpreter.add_module_file(path);
    }
  }

  if (!output.empty())
  {
    interpreter.debug_enabled(true);
    interpreter.debug_path(output);
  }

  try
  {
    std::cout << interpreter.query(query_expr) << std::endl;
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
