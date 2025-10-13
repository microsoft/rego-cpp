#include "internal.hh"
#include "rego.hh"
#include "trieste/json.h"
#include "trieste/logging.h"
#include "trieste/wf.h"

#include <filesystem>

namespace
{
  using namespace rego;
  using namespace wf::ops;
  const auto wf_errors = rego::wf | (Error <<= ErrorMsg * ErrorAst * ErrorCode);

  logging::LocalLogLevel log_level(rego::LogLevel level)
  {
    switch (level)
    {
      case rego::LogLevel::Error:
        return logging::local_log_level<logging::Error>();

      case rego::LogLevel::Debug:
        return logging::local_log_level<logging::Debug>();

      case rego::LogLevel::Info:
        return logging::local_log_level<logging::Info>();

      case rego::LogLevel::None:
        return logging::local_log_level<logging::None>();

      case rego::LogLevel::Output:
        return logging::local_log_level<logging::Output>();

      case rego::LogLevel::Trace:
        return logging::local_log_level<logging::Trace>();

      case rego::LogLevel::Warn:
        return logging::local_log_level<logging::Warn>();
    }

    throw std::runtime_error("Unsupported log level");
  }
}

namespace rego
{
  using clock = std::chrono::high_resolution_clock;
  using duration = std::chrono::high_resolution_clock::duration;
  using timestamp = std::chrono::high_resolution_clock::time_point;

  Interpreter::Interpreter() :
    m_dataseq(NodeDef::create(DataSeq)),
    m_moduleseq(NodeDef::create(ModuleSeq)),
    m_input(Input << Undefined),
    m_query(NodeDef::create(Query)),
    m_debug_path(""),
    m_debug_enabled(false),
    m_wf_check_enabled(false),
    m_builtins(BuiltInsDef::create()),
    m_data_count(0),
    m_log_level(LogLevel::Output)
  {}

  Reader& Interpreter::reader()
  {
    if (m_reader == nullptr)
    {
      m_reader = std::make_unique<Reader>(file_to_rego());
    }

    return m_reader->debug_enabled(m_debug_enabled)
      .wf_check_enabled(m_wf_check_enabled);
  }

  Reader& Interpreter::json()
  {
    if (m_json == nullptr)
    {
      m_json = std::make_unique<Reader>(json::reader());
    }

    return m_json->debug_enabled(m_debug_enabled)
      .wf_check_enabled(m_wf_check_enabled)
      .debug_path(m_debug_path / "json");
  }

  Rewriter& Interpreter::data_from_json()
  {
    if (m_data_from_json == nullptr)
    {
      m_data_from_json = std::make_unique<Rewriter>(json_to_rego());
    }

    return m_data_from_json->debug_enabled(m_debug_enabled)
      .wf_check_enabled(m_wf_check_enabled);
  }

  Rewriter& Interpreter::input_from_json()
  {
    if (m_input_from_json == nullptr)
    {
      m_input_from_json = std::make_unique<Rewriter>(json_to_rego(true));
    }

    return m_input_from_json->debug_enabled(m_debug_enabled)
      .wf_check_enabled(m_wf_check_enabled);
  }

  Rewriter& Interpreter::to_input()
  {
    if (m_to_input == nullptr)
    {
      m_to_input = std::make_unique<Rewriter>(rego_to_input());
    }

    return m_to_input->debug_enabled(m_debug_enabled)
      .wf_check_enabled(m_wf_check_enabled)
      .debug_path(m_debug_path / "input");
  }

  Rewriter& Interpreter::bundle()
  {
    if (m_bundle == nullptr)
    {
      m_bundle = std::make_unique<Rewriter>(rego_to_bundle(m_builtins));
    }

    return m_bundle->debug_enabled(m_debug_enabled)
      .wf_check_enabled(m_wf_check_enabled)
      .debug_path(m_debug_path / "bundle");
  }

  Rewriter& Interpreter::write_bundle()
  {
    if (m_write_bundle == nullptr)
    {
      m_write_bundle = std::make_unique<Rewriter>(bundle_to_json());
    }

    return m_write_bundle->debug_enabled(m_debug_enabled)
      .wf_check_enabled(m_wf_check_enabled)
      .debug_path(m_debug_path / "bundle_to_json");
  }

  Rewriter& Interpreter::read_bundle()
  {
    if (m_read_bundle == nullptr)
    {
      m_read_bundle = std::make_unique<Rewriter>(json_to_bundle());
    }

    return m_read_bundle->debug_enabled(m_debug_enabled)
      .wf_check_enabled(m_wf_check_enabled)
      .debug_path(m_debug_path / "json_to_bundle");
  }

  Node Interpreter::add_module_file(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("Module file does not exist");
    }

    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Adding module file: " << path;
    std::string debug = "module" + std::to_string(m_data_count++);
    auto result = reader().file(path).debug_path(m_debug_path / debug).read();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_moduleseq << result.ast->front();
    return nullptr;
  }

  Node Interpreter::add_module(
    const std::string& name, const std::string& contents)
  {
    auto loglevel = ::log_level(m_log_level);
    std::string debug = "module" + std::to_string(m_data_count++);
    auto result = reader()
                    .synthetic(contents, name)
                    .debug_path(m_debug_path / debug)
                    .read();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_moduleseq << result.ast->front();
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

    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Adding data file: " << path;
    std::string debug = "data" + std::to_string(m_data_count++);
    auto result =
      json().file(path) >> data_from_json().debug_path(m_debug_path / debug);
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_dataseq << (Data << result.ast->front());
    return nullptr;
  }

  Node Interpreter::add_data_json(const std::string& contents)
  {
    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Adding data (" << contents.size() << " bytes)";
    std::string debug = "data" + std::to_string(m_data_count++);
    auto result = json().synthetic(contents, debug + ".json") >>
      data_from_json().debug_path(m_debug_path / debug);

    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_dataseq << (Data << result.ast->front());
    return nullptr;
  }

  Node Interpreter::add_data(const Node& node)
  {
    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Adding data from JSON AST";
    std::string debug = "data" + std::to_string(m_data_count++);
    auto result =
      (Top << node) >> data_from_json().debug_path(m_debug_path / debug);
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_dataseq << (Data << result.ast->front());
    return nullptr;
  }

  Node Interpreter::set_input_json_file(const std::filesystem::path& path)
  {
    if (!std::filesystem::exists(path))
    {
      throw std::runtime_error("Input file does not exist");
    }

    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Setting input from file: " << path;
    auto result =
      json().file(path) >> input_from_json().debug_path(m_debug_path / "input");
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_input = Input << result.ast->front();
    return nullptr;
  }

  Node Interpreter::set_input_term(const std::string& term)
  {
    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Setting input (" << term.size() << " bytes)";
    auto result = reader().synthetic(term) >> to_input();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_input = Input << result.ast->front();
    return nullptr;
  }

  Node Interpreter::set_input_json(const std::string& contents)
  {
    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Setting input (" << contents.size() << " bytes)";
    auto result = json().synthetic(contents) >>
      input_from_json().debug_path(m_debug_path / "input");
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_input = Input << result.ast->front();
    return nullptr;
  }

  Node Interpreter::set_input(const Node& node)
  {
    auto loglevel = ::log_level(m_log_level);
    Node input_node;
    if (node->in(
          {json::Object,
           json::Array,
           json::String,
           json::Number,
           json::True,
           json::False,
           json::Null}))
    {
      logging::Info() << "Setting input from JSON AST";
      auto result =
        (Top << node) >> input_from_json().debug_path(m_debug_path / "input");
      if (!result.ok)
      {
        logging::Error err;
        result.print_errors(err);
        return ErrorSeq << result.errors;
      }

      input_node = Input << result.ast->front();
    }
    else if (node->in({Term, Object, Array, Set, Scalar}))
    {
      logging::Info() << "Setting input from Rego AST";
      input_node = Input << Resolver::to_term(node);
    }
    else if (node == Input)
    {
      input_node = node;
    }
    else
    {
      return err(node, "Invalid input node");
    }

    m_input = input_node;
    return nullptr;
  }

  Node Interpreter::set_query(const std::string& query)
  {
    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Setting query: " << query;
    auto result =
      reader().synthetic(query).debug_path(m_debug_path / "query").read();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    m_query = result.ast->front();
    return nullptr;
  }

  Interpreter& Interpreter::entrypoints(
    const std::initializer_list<std::string>& entrypoints)
  {
    m_entrypoints = entrypoints;
    return *this;
  }

  Interpreter& Interpreter::entrypoints(
    const std::vector<std::string>& entrypoints)
  {
    m_entrypoints = entrypoints;
    return *this;
  }

  const std::vector<std::string>& Interpreter::entrypoints() const
  {
    return m_entrypoints;
  }

  std::vector<std::string>& Interpreter::entrypoints()
  {
    return m_entrypoints;
  }

  Node Interpreter::raw_query(const std::string& query_expr)
  {
    set_query(query_expr);
    return raw_query();
  }

  Node Interpreter::raw_query()
  {
    auto loglevel = ::log_level(m_log_level);
    logging::Info() << "Query";

    m_builtins->clear();

    Node ast = Top
      << (RegoBundle << EntryPointSeq << m_dataseq << m_moduleseq << m_query);

    auto result = ast >> bundle();

    PRINT_ACTION_METRICS();

    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    Bundle bundle = BundleDef::from_node(result.ast->front());
    return query_bundle(bundle);
  }

  Node Interpreter::query_bundle(const Bundle& bundle)
  {
    auto loglevel = ::log_level(m_log_level);
    WFContext context(wf_bundle);
    try
    {
      return m_vm.bundle(bundle).builtins(m_builtins).run_query(m_input);
    }
    catch (const std::exception& e)
    {
      return err(m_input, e.what());
    }
  }

  Node Interpreter::query_bundle(
    const Bundle& bundle, const std::string& entrypoint)
  {
    auto loglevel = ::log_level(m_log_level);
    WFContext context(wf_bundle);
    try
    {
      return m_vm.bundle(bundle)
        .builtins(m_builtins)
        .run_entrypoint({entrypoint}, m_input);
    }
    catch (const std::exception& e)
    {
      return err(m_input, e.what());
    }
  }

  std::string Interpreter::query()
  {
    return output_to_string(raw_query());
  }

  std::string Interpreter::query(const std::string& query_expr)
  {
    return output_to_string(raw_query(query_expr));
  }

  std::string Interpreter::output_to_string(const Node& ast) const
  {
    auto loglevel = ::log_level(m_log_level);
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

    return *this;
  }

  Node Interpreter::build()
  {
    auto loglevel = ::log_level(m_log_level);
    m_builtins->clear();

    Node entrypointseq = NodeDef::create(EntryPointSeq);
    for (auto& entrypoint : m_entrypoints)
    {
      entrypointseq << (IRString ^ entrypoint);
    }

    Node ast = Top
      << (RegoBundle << entrypointseq << m_dataseq << m_moduleseq << m_query);

    auto result = ast >> bundle();

    PRINT_ACTION_METRICS();

    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    return result.ast->front();
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

  Interpreter& Interpreter::wf_check_enabled(bool enabled)
  {
    m_wf_check_enabled = enabled;
    return *this;
  }

  bool Interpreter::wf_check_enabled() const
  {
    return m_wf_check_enabled;
  }

  BuiltIns Interpreter::builtins() const
  {
    return m_builtins;
  }

  Node Interpreter::save_bundle(
    const std::filesystem::path& dir, const Node& bundle)
  {
    auto loglevel = ::log_level(m_log_level);
    if (std::filesystem::exists(dir))
    {
      std::filesystem::remove_all(dir);
    }

    auto result = (rego::Top << bundle) >> write_bundle();

    if (!result.ok)
    {
      trieste::logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    auto write_result = (Top << result.ast->at(0)) >>
      trieste::json::writer("data.json")
        .dir(dir)
        .debug_enabled(debug_enabled())
        .debug_path(debug_path() / "bundle_data_json")
        .wf_check_enabled(wf_check_enabled());
    if (!write_result.ok)
    {
      trieste::logging::Error err;
      write_result.print_errors(err);
      return ErrorSeq << write_result.errors;
    }

    write_result = (Top << result.ast->at(1)) >>
      trieste::json::writer("plan.json")
        .dir(dir)
        .debug_enabled(debug_enabled())
        .debug_path(debug_path() / "bundle_plan_json")
        .wf_check_enabled(wf_check_enabled());
    ;
    if (!write_result.ok)
    {
      trieste::logging::Error err;
      write_result.print_errors(err);
      return ErrorSeq << write_result.errors;
    }

    {
      WFContext context(wf_bundle);
      for (auto& file : *(bundle / ModuleFileSeq))
      {
        std::string_view name = (file / Name)->location().view();
        std::string_view contents = (file / Contents)->location().view();

        std::filesystem::path modulepath = dir / name;
        std::ofstream stream(modulepath);
        stream << contents;
      }
    }

    return nullptr;
  }

  Node Interpreter::load_bundle(const std::filesystem::path& dir)
  {
    auto loglevel = ::log_level(m_log_level);
    std::filesystem::path data_path = dir / "data.json";
    if (!std::filesystem::exists(data_path))
    {
      logging::Error() << data_path << " does not exist";
      return nullptr;
    }

    Node data;
    auto result = json().file(data_path).read();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    data = result.ast->front();

    std::filesystem::path plan_path = dir / "plan.json";
    if (!std::filesystem::exists(plan_path))
    {
      logging::Error() << plan_path << " does not exist";
      return nullptr;
    }

    Node plan;
    result = json().file(plan_path).read();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    plan = result.ast->front();

    Node bundle_json = Top
      << (json::Object << (json::Member << (json::Key ^ "data") << data)
                       << (json::Member << (json::Key ^ "plan") << plan));

    result = bundle_json >> read_bundle();
    if (!result.ok)
    {
      logging::Error err;
      result.print_errors(err);
      return ErrorSeq << result.errors;
    }

    Node bundle = result.ast->front();

    WFContext context(wf_bundle);
    for (std::filesystem::directory_iterator next(dir), end; next != end;
         ++next)
    {
      auto const& path = next->path();
      if (path.extension().string() == ".rego")
      {
        std::string name = path.filename().string();
        std::ifstream stream(path);
        std::ostringstream contents;
        contents << stream.rdbuf();
        bundle / ModuleFileSeq
          << (ModuleFile << (IRString ^ name) << (IRString ^ contents.str()));
      }
    }

    return result.ast->front();
  }

  Interpreter& Interpreter::log_level(LogLevel level)
  {
    m_log_level = level;
    return *this;
  }

  Interpreter& Interpreter::log_level(const std::string& level)
  {
    m_log_level = log_level_from_string(level);
    return *this;
  }

  LogLevel Interpreter::log_level() const
  {
    return m_log_level;
  }

  const std::string& Interpreter::c_error() const
  {
    return m_c_error;
  }

  Interpreter& Interpreter::c_error(const std::string& error)
  {
    m_c_error = error;
    return *this;
  }
}
