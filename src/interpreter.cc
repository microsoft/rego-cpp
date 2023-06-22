#include "interpreter.h"

#include "lang.h"
#include "wf.h"

#include <iostream>
#include <sstream>

namespace rego
{
  Interpreter::Interpreter(bool disable_well_formed_checks) :
    m_parser(parser()),
    m_wf_parser(wf_parser),
    m_passes(passes()),
    m_module_seq(NodeDef::create(ModuleSeq)),
    m_data_seq(NodeDef::create(DataSeq)),
    m_input(NodeDef::create(Input)),
    m_debug_path("."),
    m_debug_enabled(false),
    m_well_formed_checks_enabled(!disable_well_formed_checks)
  {}

  void Interpreter::add_module_file(const std::filesystem::path& path)
  {
    auto file_ast = m_parser.sub_parse(path);
    m_module_seq->push_back(file_ast);
  }

  void Interpreter::add_module(
    const std::string& name, const std::string& contents)
  {
    auto module_source = SourceDef::synthetic(contents);
    auto module = m_parser.sub_parse(name, File, module_source);
    m_module_seq->push_back(module);
  }

  void Interpreter::add_data_json_file(const std::filesystem::path& path)
  {
    auto file_ast = m_parser.sub_parse(path);
    m_data_seq->push_back(file_ast);
  }

  void Interpreter::add_data_json(const std::string& json)
  {
    auto data_source = SourceDef::synthetic(json);
    auto data = m_parser.sub_parse("data", File, data_source);
    m_data_seq->push_back(data);
  }

  void Interpreter::add_input_json_file(const std::filesystem::path& path)
  {
    if (m_input->size() > 0)
    {
      throw std::runtime_error("Input already set");
    }

    auto file_ast = m_parser.sub_parse(path);
    m_input->push_back(file_ast);
  }

  void Interpreter::add_input_json(const std::string& json)
  {
    if (m_input->size() > 0)
    {
      throw std::runtime_error("Input already set");
    }

    auto input_source = SourceDef::synthetic(json);
    auto input = m_parser.sub_parse("input", File, input_source);
    m_input->push_back(input);
  }

  std::string Interpreter::query(const std::string& query_expr) const
  {
    auto ast = NodeDef::create(Top);
    auto rego = NodeDef::create(rego::Rego);
    auto query_src = SourceDef::synthetic(query_expr);
    auto query = m_parser.sub_parse("query", rego::Query, query_src);
    if (m_input->size() == 0)
    {
      m_input->push_back(NodeDef::create(Undefined));
    }

    rego->push_back(query);
    rego->push_back(m_input);
    rego->push_back(m_data_seq);
    rego->push_back(m_module_seq);
    ast->push_back(rego);

    bool ok = m_wf_parser.build_st(ast, std::cerr);
    if (m_well_formed_checks_enabled)
    {
      ok = m_wf_parser.check(ast, std::cerr) && ok;
    }

    write_ast(0, "parse", ast);
    if (!ok)
    {
      std::stringstream buf;
      ast->errors(buf);
      throw std::runtime_error(buf.str());
    }

    for (std::size_t i = 0; i < m_passes.size(); ++i)
    {
      auto& [pass_name, pass, wf] = m_passes[i];
      auto [new_ast, count, changes] = pass->run(ast);
      ast = new_ast;

      write_ast(i + 1, pass_name, ast);

      ok = wf.build_st(ast, std::cout);
      if (m_well_formed_checks_enabled)
      {
        ok = wf.check(ast, std::cout) && ok;
      }

      if (!ok)
      {
        std::stringstream buf;
        buf << "Failed at pass " << pass_name << std::endl;
        ast->errors(buf);
        throw std::runtime_error(buf.str());
      }
    }

    std::stringstream result_buf;
    for (auto result : *ast)
    {
      result_buf << rego::to_json(result) << std::endl;
    }

    return result_buf.str();
  }

  Interpreter& Interpreter::debug_path(const std::filesystem::path& path)
  {
    m_debug_path = path;
    return *this;
  }

  const std::filesystem::path& Interpreter::debug_path() const
  {
    return m_debug_path;
  }

  Interpreter& Interpreter::debug_enabled(bool enabled)
  {
    m_debug_enabled = enabled;
    return *this;
  }

  bool Interpreter::debug_enabled() const
  {
    return m_debug_enabled;
  }

  Interpreter& Interpreter::well_formed_checks_enabled(bool enabled)
  {
    m_well_formed_checks_enabled = enabled;
    return *this;
  }

  bool Interpreter::well_formed_checks_enabled() const
  {
    return m_well_formed_checks_enabled;
  }

  void Interpreter::write_ast(
    std::size_t index, const std::string& pass, const Node& ast) const
  {
    if (!m_debug_enabled)
    {
      return;
    }

    if (!std::filesystem::is_directory(m_debug_path))
    {
      std::filesystem::create_directory(m_debug_path);
    }

    std::filesystem::path output;
    if (index < 10)
    {
      output =
        m_debug_path / ("0" + std::to_string(index) + "_" + pass + ".trieste");
    }
    else
    {
      output = m_debug_path / (std::to_string(index) + "_" + pass + ".trieste");
    }
    std::ofstream f(output, std::ios::binary | std::ios::out);

    if (f)
    {
      // Write the AST to the output file.
      f << "rego" << std::endl << pass << std::endl << ast;
      return;
    }

    std::cerr << "Could not open " << output << " for writing." << std::endl;
  }

  Interpreter& Interpreter::executable(const std::filesystem::path& path)
  {
    m_parser.executable(path);
    return *this;
  }

  const std::filesystem::path& Interpreter::executable() const
  {
    return m_parser.executable();
  }
}
