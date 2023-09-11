#include "internal.hh"

namespace rego
{
  using clock = std::chrono::high_resolution_clock;
  using duration = std::chrono::high_resolution_clock::duration;
  using timestamp = std::chrono::high_resolution_clock::time_point;

  Interpreter::Interpreter() :
    m_parser(parser()),
    m_wf_parser(wf_parser),
    m_module_seq(NodeDef::create(ModuleSeq)),
    m_data_seq(NodeDef::create(DataSeq)),
    m_input(NodeDef::create(Input)),
    m_debug_path("."),
    m_debug_enabled(false),
    m_well_formed_checks_enabled(false)
  {
    wf::push_back(&wf_parser);
    m_builtins.register_standard_builtins();
  }

  Interpreter::~Interpreter()
  {
    wf::pop_front();
  }

  void Interpreter::insert_module(const Node& module)
  {
    // sort the modules by their package name. This allows us to merge
    // modules with the same name after their imports are resolved.
    auto pos = std::upper_bound(
      m_module_seq->begin(), m_module_seq->end(), module, [](auto& a, auto& b) {
        auto a_pkg = a->front();
        auto b_pkg = b->front();
        auto a_str = std::string(a_pkg->location().view());
        auto b_str = std::string(b_pkg->location().view());
        return a_pkg->location() < b_pkg->location();
      });

    m_module_seq->insert(pos, module);
  }

  void Interpreter::add_module_file(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("Module file does not exist");
    }

    LOG("Adding module file: ", path);
    auto file_ast = m_parser.sub_parse(path);
    insert_module(file_ast);
  }

  void Interpreter::add_module(
    const std::string& name, const std::string& contents)
  {
    auto module_source = SourceDef::synthetic(contents);
    auto module = m_parser.sub_parse(name, File, module_source);
    insert_module(module);
    LOG("Adding module: ", name, "(", contents.size(), " bytes)");
  }

  void Interpreter::add_data_json_file(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("Data file does not exist");
    }

    LOG("Adding data file: ", path);
    auto file_ast = m_parser.sub_parse(path);
    m_data_seq->push_back(file_ast);
  }

  void Interpreter::add_data_json(const std::string& json)
  {
    auto data_source = SourceDef::synthetic(json);
    auto data = m_parser.sub_parse("data", File, data_source);
    m_data_seq->push_back(data);
    LOG("Adding data (", json.size(), " bytes)");
  }

  void Interpreter::add_data(const Node& node)
  {
    m_data_seq->push_back(node);
    LOG("Adding data AST");
  }

  void Interpreter::set_input_json_file(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("Input file does not exist");
    }

    LOG("Setting input from file: ", path);
    auto file_ast = m_parser.sub_parse(path);
    m_input = Input << file_ast;
  }

  void Interpreter::set_input_json(const std::string& json)
  {
    LOG("Setting input (", json.size(), " bytes)");
    auto input_source = SourceDef::synthetic(json);
    auto ast = m_parser.sub_parse("input", File, input_source);
    m_input = Input << ast;
  }

  void Interpreter::set_input(const Node& node)
  {
    LOG("Setting input AST");
    m_input = Input << node;
  }

  Node Interpreter::get_errors(const Node& node) const
  {
    if (node->type() == Error)
    {
      return node->clone();
    }

    Node errorseq = NodeDef::create(ErrorSeq);
    for (auto& child : *node)
    {
      Node error = get_errors(child);
      if (error->type() == Error)
      {
        errorseq->push_back(error);
      }
      else if (error->size() > 0)
      {
        errorseq->insert(errorseq->end(), error->begin(), error->end());
      }
    }

    return errorseq;
  }

  Node Interpreter::raw_query(const std::string& query_expr) const
  {
    LOG("Query: ", query_expr);
    auto ast = NodeDef::create(Top);
    auto rego = NodeDef::create(rego::Rego);
    auto query_src = SourceDef::synthetic(query_expr);
    auto query = m_parser.sub_parse("query", rego::Query, query_src);
    if (m_input->size() == 0)
    {
      m_input->push_back(NodeDef::create(Undefined));
    }

    rego->push_back(query);
    rego->push_back(m_input->clone());
    rego->push_back(m_data_seq->clone());
    rego->push_back(m_module_seq->clone());
    ast->push_back(rego);

    bool ok = m_wf_parser.build_st(ast, std::cerr);
    if (m_well_formed_checks_enabled)
    {
      ok = m_wf_parser.check(ast, std::cerr) && ok;
    }

    write_ast(0, "parse", ast);
    if (!ok)
    {
      return get_errors(ast);
    }

    LOG("Name", "Passes", "Changes", "Time(ms)");
    auto passes = rego::passes(m_builtins);
    for (std::size_t i = 0; i < passes.size(); ++i)
    {
      timestamp start = clock::now();
      auto& [pass_name, pass, wf] = passes[i];
      wf::push_back(wf);
      auto [new_ast, count, changes] = pass->run(ast);
      wf::pop_front();
      ast = new_ast;

      ok = wf->build_st(ast, std::cout);
      write_ast(i + 1, pass_name, ast);

      if (m_well_formed_checks_enabled)
      {
        ok = wf->check(ast, std::cout) && ok;
      }

      duration elapsed = clock::now() - start;
      LOG(pass_name, count, changes, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

      Node errors = get_errors(ast);
      if (errors->size() > 0)
      {
        ok = false;
      }

      if (!ok)
      {
        if (errors->size() == 0)
        {
          std::ostringstream error;
          error << "Failed at pass " << pass_name << std::endl;
          ast->errors(error);
          errors->push_back(err(ast, error.str(), "well_formed_error"));
          LOG(error.str());
        }

        return errors;
      }
    }

    LOG("Query result: ", ast);
    return ast;
  }

  std::string Interpreter::query(const std::string& query_expr) const
  {
    return output_to_string(raw_query(query_expr));
  }

  std::string Interpreter::output_to_string(const Node& ast) const
  {
    std::ostringstream output_buf;
    if (ast->type() == ErrorSeq)
    {
      output_buf << "errors:" << std::endl;

      for (auto& error : *ast)
      {
        Node error_ast = error / ErrorAst;
        output_buf << "---" << std::endl;
        output_buf << "error: " << (error / ErrorMsg)->location().view()
                   << std::endl;
        output_buf << "code: " << (error / ErrorCode)->location().view()
                   << std::endl;
        output_buf << error_ast;
      }
    }
    else
    {
      for (auto result : *ast)
      {
        output_buf << rego::to_json(result) << std::endl;
      }
    }

    return output_buf.str();
  }

  Interpreter& Interpreter::debug_path(const std::filesystem::path& path)
  {
    m_debug_path = path;
    if (!m_debug_path.empty())
    {
      if (std::filesystem::is_directory(m_debug_path))
      {
        std::filesystem::remove_all(m_debug_path);
      }

      std::filesystem::create_directory(m_debug_path);
    }
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

  BuiltIns& Interpreter::builtins()
  {
    return m_builtins;
  }

  const BuiltIns& Interpreter::builtins() const
  {
    return m_builtins;
  }
}
