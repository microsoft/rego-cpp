#include "interpreter.h"

int main(int argc, char** argv)
{
  CLI::App app;

  auto interpreter = rego::Interpreter();

  interpreter.executable(argv[0]);

  app.set_help_all_flag("--help-all", "Expand all help");

  std::vector<std::filesystem::path> data_paths;
  app.add_option("-d,--data", data_paths, "Data/Policy files");

  std::filesystem::path input_path;
  app.add_option("-i,--input", input_path, "Input JSON file");

  std::filesystem::path output;
  app.add_option("-a,--ast", output, "Output the AST");

  std::string query_expr;
  app.add_option("-q,--query", query_expr, "Query")->required();

  try
  {
    app.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  if (!input_path.empty())
  {
        if(!std::filesystem::exists(input_path))
    {
      std::cerr << std::filesystem::weakly_canonical(input_path) << " does not exist" << std::endl;
      return 1;
    }

    interpreter.add_input_json_file(input_path);
  }

  for (auto& path : data_paths)
  {
    if(!std::filesystem::exists(path))
    {
      std::cerr << std::filesystem::weakly_canonical(path) << " does not exist" << std::endl;
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
