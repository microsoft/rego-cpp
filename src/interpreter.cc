#include "rego.hh"
#include "trieste/json.h"
#include "trieste/wf.h"
#include "unify.hh"

namespace
{
  using namespace rego;
  using namespace wf::ops;
  const auto wf_errors = rego::wf | (Error <<= ErrorMsg * ErrorAst * ErrorCode);
}

namespace rego
{
  using clock = std::chrono::high_resolution_clock;
  using duration = std::chrono::high_resolution_clock::duration;
  using timestamp = std::chrono::high_resolution_clock::time_point;

  Interpreter::Interpreter(bool v1_compatible) :
    m_reader(reader(v1_compatible)),
    m_debug_path("."),
    m_builtins(BuiltInsDef::create()),
    m_unify(unify(m_builtins)),
    m_json(json::reader()),
    m_from_json(from_json()),
    m_to_input(to_input()),
    m_data_count(0)
  {
    m_ast = Top << (Rego << Query << Input << DataSeq << ModuleSeq);
  }

  void Interpreter::merge(const Node& node)
  {
    WFContext context(wf_unify_input);
    Node rego = m_ast / Rego;
    if (node == Input)
    {
      rego->replace(rego / Input, node);
    }
    else if (node == Data)
    {
      rego / DataSeq << node;
    }
    else if (node == Module)
    {
      // insert the module to maintain a sorted list (by package name).
      // This allows us to merge modules with the same name after their
      // imports are resolved.
      Node moduleseq = rego / ModuleSeq;
      auto pos = std::upper_bound(
        moduleseq->begin(), moduleseq->end(), node, [](auto& a, auto& b) {
          auto a_pkg = a->front();
          auto b_pkg = b->front();
          auto a_str = std::string(a_pkg->location().view());
          auto b_str = std::string(b_pkg->location().view());
          return a_pkg->location() < b_pkg->location();
        });

      moduleseq->insert(pos, node);
    }
    else if (node == Query)
    {
      rego->replace(rego / Query, node);
    }
    else
    {
      logging::Error() << node;
      throw std::runtime_error("Invalid node type");
    }
  }

  Node Interpreter::add_module_file(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("Module file does not exist");
    }

    logging::Info() << "Adding module file: " << path;
    std::string debug = "module" + std::to_string(m_data_count++);
    auto result = m_reader.file(path).debug_path(m_debug_path / debug).read();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    merge(result.ast->front());
    return nullptr;
  }

  Node Interpreter::add_module(
    const std::string& name, const std::string& contents)
  {
    std::string debug = "module" + std::to_string(m_data_count++);
    auto result =
      m_reader.synthetic(contents).debug_path(m_debug_path / debug).read();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    merge(result.ast->front());
    logging::Info() << "Adding module: " << name << "(" << contents.size()
                    << " bytes)";
    return nullptr;
  }

  Node Interpreter::add_data_json_file(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("Data file does not exist");
    }

    logging::Info() << "Adding data file: " << path;
    std::string debug = "data" + std::to_string(m_data_count++);
    auto result =
      m_json.file(path) >> m_from_json.debug_path(m_debug_path / debug);
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    merge(Data << result.ast->front());
    return nullptr;
  }

  Node Interpreter::add_data_json(const std::string& json)
  {
    logging::Info() << "Adding data (" << json.size() << " bytes)";
    std::string debug = "data" + std::to_string(m_data_count++);
    auto result =
      m_json.synthetic(json) >> m_from_json.debug_path(m_debug_path / debug);

    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    merge(Data << result.ast->front());
    return nullptr;
  }

  Node Interpreter::add_data(const Node& node)
  {
    logging::Info() << "Adding data from JSON AST";
    std::string debug = "data" + std::to_string(m_data_count++);
    auto result = node >> m_from_json.debug_path(m_debug_path / debug);
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    merge(Data << result.ast->front());
    return nullptr;
  }

  Node Interpreter::set_input_json_file(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("Input file does not exist");
    }

    logging::Info() << "Setting input from file: " << path;
    auto result =
      m_json.file(path) >> m_from_json.debug_path(m_debug_path / "input");
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    merge(Input << result.ast->front());
    return nullptr;
  }

  Node Interpreter::set_input_term(const std::string& term)
  {
    logging::Info() << "Setting input (" << term.size() << " bytes)";
    auto result = m_reader.synthetic(term) >> m_to_input;
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    merge(result.ast->front());
    return nullptr;
  }

  Node Interpreter::set_input(const Node& node)
  {
    logging::Info() << "Setting input from JSON AST";
    auto result = node >> m_from_json.debug_path(m_debug_path / "input");
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
      ;
    }

    merge(Input << result.ast->front());
    return nullptr;
  }

  Node Interpreter::raw_query(const std::string& query_expr)
  {
    logging::Info() << "Query: " << query_expr;
    auto query_src = SourceDef::synthetic(query_expr);
    auto result =
      m_reader.synthetic(query_expr).debug_path(m_debug_path / "query").read();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    merge(result.ast->front());

    {
      WFContext context(wf_unify_input);
      Node input = m_ast / Rego / Input;
      if (input->empty())
      {
        input << Undefined;
      }
    }

    m_builtins->clear();

    result = m_ast >> m_unify;
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    return result.ast->front();
  }

  std::string Interpreter::query(const std::string& query_expr)
  {
    return output_to_string(raw_query(query_expr));
  }

  std::string Interpreter::output_to_string(const Node& ast) const
  {
    std::ostringstream output_buf;
    if (ast->type() == ErrorSeq)
    {
      WFContext context(wf_errors);
      output_buf << "errors:" << std::endl;

      for (auto& error : *ast)
      {
        Node error_ast = error / ErrorAst;
        output_buf << "---" << std::endl;
        output_buf << "error: " << (error / ErrorMsg)->location().view()
                   << std::endl;
        auto error_code = error->find_first(ErrorCode, error->begin());
        if (error_code != error->end())
        {
          output_buf << "code: " << (*error_code)->location().view()
                     << std::endl;
        }
        output_buf << error_ast;
      }
    }
    else
    {
      WFContext context(rego::wf_result);
      output_buf << rego::to_key(ast, true);
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

    m_json.debug_path(m_debug_path / "json");
    m_unify.debug_path(m_debug_path / "unify");
    m_to_input.debug_path(m_debug_path / "input");

    return *this;
  }

  const std::filesystem::path& Interpreter::debug_path() const
  {
    return m_debug_path;
  }

  Interpreter& Interpreter::debug_enabled(bool enabled)
  {
    m_reader.debug_enabled(enabled);
    m_json.debug_enabled(enabled);
    m_unify.debug_enabled(enabled);
    m_from_json.debug_enabled(enabled);
    m_to_input.debug_enabled(enabled);
    return *this;
  }

  bool Interpreter::debug_enabled() const
  {
    return m_reader.debug_enabled();
  }

  Interpreter& Interpreter::wf_check_enabled(bool enabled)
  {
    m_reader.wf_check_enabled(enabled);
    m_json.wf_check_enabled(enabled);
    m_unify.wf_check_enabled(enabled);
    m_from_json.wf_check_enabled(enabled);
    m_to_input.wf_check_enabled(enabled);
    return *this;
  }

  bool Interpreter::wf_check_enabled() const
  {
    return m_reader.wf_check_enabled();
  }

  BuiltInsDef& Interpreter::builtins() const
  {
    return *m_builtins;
  }
}
