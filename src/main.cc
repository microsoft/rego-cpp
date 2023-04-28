#include "lang.h"
#include "wf.h"

using namespace trieste;

int write_ast(
  std::filesystem::path output,
  std::size_t index,
  const std::string& pass,
  const trieste::Node& ast)
{
  if (output.empty())
  {
    return 0;
  }

  if (!std::filesystem::is_directory(output))
  {
    std::filesystem::create_directory(output);
  }

  if (index < 10)
  {
    output = output / ("0" + std::to_string(index) + "_" + pass + ".trieste");
  }
  else
  {
    output = output / (std::to_string(index) + "_" + pass + ".trieste");
  }
  std::ofstream f(output, std::ios::binary | std::ios::out);

  if (f)
  {
    // Write the AST to the output file.
    f << "rego" << std::endl << pass << std::endl << ast;

    return 0;
  }

  std::cout << "Could not open " << output << " for writing." << std::endl;
  return -1;
}

int main(int argc, char** argv)
{
  CLI::App app;

  auto parser = rego::parser();
  auto wfParser = rego::wf_parser;
  std::vector<rego::PassCheck> passes(rego::passes());

  parser.executable(argv[0]);

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

  auto ast = NodeDef::create(Top);
  auto policy = NodeDef::create(rego::Policy);
  auto query_src = SourceDef::synthetic(query_expr);
  auto query = parser.sub_parse("query", rego::Query, query_src);
  policy->push_back(query);
  auto input = NodeDef::create(rego::Input);
  if (!input_path.empty())
  {
    auto file_ast = parser.sub_parse(input_path);
    input->push_back(file_ast);
  }
  else
  {
    input->push_back(NodeDef::create(rego::Undefined));
  }
  policy->push_back(input);

  auto modules = NodeDef::create(rego::ModuleSeq);
  auto data = NodeDef::create(rego::DataSeq);
  for (auto& path : data_paths)
  {
    auto file_ast = parser.sub_parse(path);
    if (path.extension() == ".json")
    {
      data->push_back(file_ast);
    }
    else
    {
      modules->push_back(file_ast);
    }
  }
  policy->push_back(data);
  policy->push_back(modules);
  ast->push_back(policy);

  auto ok = wfParser.build_st(ast, std::cout);
  ok = wfParser.check(ast, std::cout) && ok;

  if (!ok)
  {
    ast->errors(std::cout);
    write_ast(output, 0, "parse", ast);
    return -1;
  }

  for (std::size_t i = 0; i < passes.size(); ++i)
  {
    auto& [pass_name, pass, wf] = passes[i];
    auto [new_ast, count, changes] = pass->run(ast);
    ast = new_ast;

    ok = wf.build_st(ast, std::cout);
    ok = wf.check(ast, std::cout) && ok;

    write_ast(output, i + 1, pass_name, ast);

    if (!ok)
    {
      std::cout << "Failed at pass " << pass_name << std::endl;
      ast->errors(std::cout);
      return -1;
    }
  }

  std::cout << rego::to_json(ast->front()) << std::endl;
  return write_ast(output, passes.size(), std::get<0>(passes.back()), ast);
}
