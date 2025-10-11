#include "rego_c.h"

#include "internal.hh"
#include "rego.hh"

namespace logging = trieste::logging;

namespace
{
  regoEnum ok_or_error(const rego::Node& result)
  {
    if (result == nullptr)
    {
      return REGO_OK;
    }

    std::ostringstream output_buf;
    output_buf << result;
    throw std::runtime_error(output_buf.str());
  }
}

namespace rego
{
  void setError(regoInterpreter* rego, const std::string& error)
  {
    reinterpret_cast<rego::Interpreter*>(rego)->c_error(error);
  }

  struct regoOutput
  {
    Node node;
    Nodes expressions;
    std::string value;
  };

  struct regoInput
  {
    Nodes stack;
    regoEnum status;

    regoInput() : status(REGO_OK) {}
  };

  struct regoBundle
  {
    Node node;
    Bundle bundle;

    regoEnum node_to_bundle(regoInterpreter* rego)
    {
      if (bundle != nullptr)
      {
        return REGO_OK;
      }

      if (node == nullptr)
      {
        rego::setError(
          rego, "No objects in bundle (likely due to an error during build)");
        return REGO_ERROR;
      }

      try
      {
        bundle = rego::BundleDef::from_node(node);
      }
      catch (std::exception& e)
      {
        rego::setError(rego, e.what());
        return REGO_ERROR;
      }

      return REGO_OK;
    }

    regoEnum bundle_to_node(regoInterpreter* rego)
    {
      if (node != nullptr)
      {
        return REGO_OK;
      }

      if (bundle == nullptr)
      {
        rego::setError(
          rego, "No objects in bundle (likely due to an error during build)");
        return REGO_ERROR;
      }

      // TODO add code to convert bundle to node
      rego::setError(
        rego,
        "No AST node in bundle (binary conversion to JSON is not supported)");
      return REGO_ERROR;
    }
  };
}

extern "C"
{
  regoSize regoErrorSize(regoInterpreter* rego)
  {
    logging::Debug() << "regoErrorSize: " << rego;
    return reinterpret_cast<rego::Interpreter*>(rego)->c_error().size() + 1;
  }

  regoEnum regoError(regoInterpreter* rego, char* buffer, regoSize size)
  {
    logging::Debug() << "regoGetError: " << (void*)buffer << "[" << size << "]";

    const std::string& error_str =
      reinterpret_cast<rego::Interpreter*>(rego)->c_error();
    if (size < error_str.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    error_str.copy(buffer, error_str.size());
    buffer[error_str.size()] = '\0';
    return REGO_OK;
  }

  regoSize regoBuildInfoSize(void)
  {
    logging::Debug() << "regoBuildInfoSize";
    return strlen(REGO_BUILD_INFO) + 1;
  }

  regoEnum regoBuildInfo(char* buffer, regoSize size)
  {
    logging::Debug() << "regoBuildInfo: " << (void*)buffer << "[" << size
                     << "]";

    std::string_view build_info(REGO_BUILD_INFO);
    if (size < build_info.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    build_info.copy(buffer, build_info.size());
    buffer[build_info.size()] = '\0';
    return REGO_OK;
  }

  regoSize regoVersionSize(void)
  {
    logging::Debug() << "regoVersionSize";
    return strlen(REGOCPP_VERSION) + 1;
  }

  regoEnum regoVersion(char* buffer, regoSize size)
  {
    logging::Debug() << "regoVersion: " << (void*)buffer << "[" << size << "]";

    std::string_view version(REGOCPP_VERSION);
    if (size < version.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    version.copy(buffer, version.size());
    buffer[version.size()] = '\0';
    return REGO_OK;
  }

  regoEnum regoLogLevelFromString(const char* value)
  {
    rego::LogLevel loglevel;
    try
    {
      return (regoEnum)rego::log_level_from_string(value);
    }
    catch (const std::exception&)
    {
      return REGO_LOG_LEVEL_UNSUPPORTED;
    }
  }

  regoEnum regoSetLogLevel(regoInterpreter* rego, regoEnum level)
  {
    rego::Interpreter* r = reinterpret_cast<rego::Interpreter*>(rego);
    switch (level)
    {
      case REGO_LOG_LEVEL_NONE:
        r->log_level(rego::LogLevel::None);
        break;

      case REGO_LOG_LEVEL_ERROR:
        r->log_level(rego::LogLevel::Error);
        break;

      case REGO_LOG_LEVEL_OUTPUT:
        r->log_level(rego::LogLevel::Output);
        break;

      case REGO_LOG_LEVEL_WARN:
        r->log_level(rego::LogLevel::Warn);
        break;

      case REGO_LOG_LEVEL_INFO:
        r->log_level(rego::LogLevel::Info);
        break;

      case REGO_LOG_LEVEL_DEBUG:
        r->log_level(rego::LogLevel::Debug);
        break;

      case REGO_LOG_LEVEL_TRACE:
        r->log_level(rego::LogLevel::Trace);
        break;

      default:
        return REGO_ERROR_INVALID_LOG_LEVEL;
    }

    return REGO_OK;
  }

  regoEnum regoSetDefaultLogLevel(regoEnum level)
  {
    switch (level)
    {
      case REGO_LOG_LEVEL_NONE:
        logging::set_level<logging::None>();
        break;

      case REGO_LOG_LEVEL_ERROR:
        logging::set_level<logging::Error>();
        break;

      case REGO_LOG_LEVEL_OUTPUT:
        logging::set_level<logging::Output>();
        break;

      case REGO_LOG_LEVEL_WARN:
        logging::set_level<logging::Warn>();
        break;

      case REGO_LOG_LEVEL_INFO:
        logging::set_level<logging::Info>();
        break;

      case REGO_LOG_LEVEL_DEBUG:
        logging::set_level<logging::Debug>();
        break;

      case REGO_LOG_LEVEL_TRACE:
        logging::set_level<logging::Trace>();
        break;

      default:
        return REGO_ERROR_INVALID_LOG_LEVEL;
    }

    return REGO_OK;
  }

  regoEnum regoGetLogLevel(regoInterpreter* rego)
  {
    return (regoEnum) reinterpret_cast<rego::Interpreter*>(rego)->log_level();
  }

  regoInterpreter* regoNew()
  {
    auto ptr = reinterpret_cast<regoInterpreter*>(new rego::Interpreter());
    logging::Debug() << "regoNew: " << ptr;
    return ptr;
  }

  void regoFree(regoInterpreter* rego)
  {
    logging::Debug() << "regoFree: " << rego;
    delete reinterpret_cast<rego::Interpreter*>(rego);
  }

  regoEnum regoAddModuleFile(regoInterpreter* rego, const char* path)
  {
    logging::Debug() << "regoAddModuleFile: " << path;
    try
    {
      return ok_or_error(
        reinterpret_cast<rego::Interpreter*>(rego)->add_module_file(path));
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoAddModule(
    regoInterpreter* rego, const char* name, const char* contents)
  {
    logging::Debug() << "regoAddModule: " << name;
    try
    {
      return ok_or_error(
        reinterpret_cast<rego::Interpreter*>(rego)->add_module(name, contents));
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoAddDataJSONFile(regoInterpreter* rego, const char* path)
  {
    logging::Debug() << "regoAddDataJSONFile: " << path;
    try
    {
      return ok_or_error(
        reinterpret_cast<rego::Interpreter*>(rego)->add_data_json_file(path));
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoAddDataJSON(regoInterpreter* rego, const char* contents)
  {
    logging::Debug() << "regoAddDataJSON: " << contents;
    try
    {
      return ok_or_error(
        reinterpret_cast<rego::Interpreter*>(rego)->add_data_json(contents));
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoSetInputJSONFile(regoInterpreter* rego, const char* path)
  {
    logging::Debug() << "regoSetInputJSONFile: " << path;
    try
    {
      return ok_or_error(
        reinterpret_cast<rego::Interpreter*>(rego)->set_input_json_file(path));
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoSetInputJSON(regoInterpreter* rego, const char* contents)
  {
    logging::Warn()
      << "regoSetInputJSON is deprecated. Please use regoSetInputTerm instead.";
    logging::Debug() << "regoSetInputJSON: " << contents;
    return regoSetInputTerm(rego, contents);
  }

  regoEnum regoSetInputTerm(regoInterpreter* rego, const char* contents)
  {
    logging::Debug() << "regoSetInputTerm: " << contents;
    try
    {
      ok_or_error(
        reinterpret_cast<rego::Interpreter*>(rego)->set_input_term(contents));
      return REGO_OK;
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoSetInput(regoInterpreter* rego, regoInput* input)
  {
    logging::Debug() << "regoSetInput: interp=" << rego << " input=" << input;
    try
    {
      rego::regoInput* ri = reinterpret_cast<rego::regoInput*>(input);
      if (ri->stack.empty())
      {
        rego::setError(rego, "Empty input");
        return REGO_ERROR;
      }

      rego::Node value = ri->stack.back()->clone();

      ok_or_error(reinterpret_cast<rego::Interpreter*>(rego)->set_input(value));
      return REGO_OK;
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  void regoSetDebugEnabled(regoInterpreter* rego, regoBoolean enabled)
  {
    logging::Debug() << "regoSetDebugEnabled: " << enabled;
    reinterpret_cast<rego::Interpreter*>(rego)->debug_enabled(enabled);
  }

  regoBoolean regoGetDebugEnabled(regoInterpreter* rego)
  {
    logging::Debug() << "regoGetDebugEnabled";
    return reinterpret_cast<rego::Interpreter*>(rego)->debug_enabled();
  }

  regoEnum regoSetDebugPath(regoInterpreter* rego, const char* path)
  {
    logging::Debug() << "regoSetDebugPath: " << path;
    try
    {
      reinterpret_cast<rego::Interpreter*>(rego)->debug_path(path);
      return REGO_OK;
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  void regoSetWellFormedChecksEnabled(
    regoInterpreter* rego, regoBoolean enabled)
  {
    logging::Debug() << "regoSetWellFormedChecksEnabled: " << enabled;
    reinterpret_cast<rego::Interpreter*>(rego)->wf_check_enabled(enabled);
  }

  regoBoolean regoGetWellFormedChecksEnabled(regoInterpreter* rego)
  {
    logging::Debug() << "regoGetWellFormedChecksEnabled";
    return reinterpret_cast<rego::Interpreter*>(rego)->wf_check_enabled();
  }

  regoOutput* regoQuery(regoInterpreter* rego, const char* query_expr)
  {
    logging::Debug() << "regoQuery: " << query_expr;
    try
    {
      auto interpreter = reinterpret_cast<rego::Interpreter*>(rego);
      rego::regoOutput* output = new rego::regoOutput();
      output->node = interpreter->raw_query(query_expr);
      if (output->node == rego::Term)
      {
        output->node = output->node->front();
      }

      output->value = interpreter->output_to_string(output->node);
      auto ptr = reinterpret_cast<regoOutput*>(output);
      logging::Debug() << "regoQuery output: " << ptr;
      return ptr;
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return nullptr;
    }
  }

  void regoSetStrictBuiltInErrors(regoInterpreter* rego, regoBoolean enabled)
  {
    logging::Debug() << "regoSetStrictBuiltInErrors: " << enabled;
    reinterpret_cast<rego::Interpreter*>(rego)->builtins()->strict_errors(
      enabled);
  }

  regoBoolean regoGetStrictBuiltInErrors(regoInterpreter* rego)
  {
    logging::Debug() << "regoGetStrictBuiltInErrors";
    return reinterpret_cast<rego::Interpreter*>(rego)
      ->builtins()
      ->strict_errors();
  }

  regoBoolean regoIsAvailableBuiltIn(regoInterpreter* rego, const char* name)
  {
    logging::Debug() << "regoIsBuiltIn: " << name;

    rego::Location loc(name);
    auto builtins = reinterpret_cast<rego::Interpreter*>(rego)->builtins();

    if (!builtins->is_builtin(loc))
    {
      return false;
    }

    auto builtin = builtins->at(loc);

    return builtin->available;
  }

  regoBundle* regoBuild(regoInterpreter* rego)
  {
    logging::Debug() << "regoBuild";
    rego::regoBundle* bundle = new rego::regoBundle();
    bundle->bundle = nullptr;
    bundle->node = reinterpret_cast<rego::Interpreter*>(rego)->build();
    return reinterpret_cast<regoBundle*>(bundle);
  }

  regoBundle* regoBundleLoad(regoInterpreter* rego, const char* dir)
  {
    logging::Debug() << "regoBundleLoad";
    rego::regoBundle* bundle = new rego::regoBundle();
    bundle->bundle = nullptr;
    bundle->node = reinterpret_cast<rego::Interpreter*>(rego)->load_bundle(
      std::filesystem::path(dir));
    return reinterpret_cast<regoBundle*>(bundle);
  }

  regoBundle* regoBundleLoadBinary(regoInterpreter* rego, const char* path)
  {
    logging::Debug() << "regoBundleLoadBinary";
    rego::regoBundle* bundle = new rego::regoBundle();
    bundle->bundle = nullptr;
    bundle->node = nullptr;
    try
    {
      bundle->bundle = rego::BundleDef::load(std::filesystem::path(path));
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
    }

    return reinterpret_cast<regoBundle*>(bundle);
  }

  regoBoolean regoBundleOk(regoBundle* bundle)
  {
    logging::Debug() << "regoBundleOk";
    if (bundle == nullptr)
    {
      return false;
    }

    rego::regoBundle* rb = reinterpret_cast<rego::regoBundle*>(bundle);
    if (rb->node == nullptr && rb->bundle == nullptr)
    {
      return false;
    }

    if (rb->node == nullptr)
    {
      return true;
    }

    if (rb->node == rego::ErrorSeq)
    {
      return false;
    }

    return true;
  }

  regoNode* regoBundleNode(regoBundle* bundle)
  {
    logging::Debug() << "regoBundleNode";
    return reinterpret_cast<regoNode*>(
      reinterpret_cast<rego::regoBundle*>(bundle)->node.get());
  }

  regoEnum regoBundleSave(
    regoInterpreter* rego, const char* dir, regoBundle* bundle)
  {
    logging::Debug() << "regoBundleSave: " << dir;
    try
    {
      rego::regoBundle* rb = reinterpret_cast<rego::regoBundle*>(bundle);
      regoEnum err = rb->bundle_to_node(rego);
      if (err != REGO_OK)
      {
        return err;
      }

      return ok_or_error(
        reinterpret_cast<rego::Interpreter*>(rego)->save_bundle(dir, rb->node));
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoBundleSaveBinary(
    regoInterpreter* rego, const char* path, regoBundle* bundle)
  {
    logging::Debug() << "regoBundleSaveBinary";
    try
    {
      rego::regoBundle* rb = reinterpret_cast<rego::regoBundle*>(bundle);
      regoEnum err = rb->node_to_bundle(rego);
      if (err != REGO_OK)
      {
        return err;
      }

      rb->bundle->save(path);
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }

    return REGO_OK;
  }

  regoEnum regoSetQuery(regoInterpreter* rego, const char* query_expr)
  {
    logging::Debug() << "regoSetQuery: " << query_expr;
    try
    {
      return ok_or_error(
        reinterpret_cast<rego::Interpreter*>(rego)->set_query(query_expr));
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoAddEntrypoint(regoInterpreter* rego, const char* entrypoint)
  {
    logging::Debug() << "regoAddEntrypoint: " << entrypoint;
    try
    {
      reinterpret_cast<rego::Interpreter*>(rego)->entrypoints().push_back(
        entrypoint);
      return REGO_OK;
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoOutput* regoBundleQuery(regoInterpreter* rego, regoBundle* bundle)
  {
    logging::Debug() << "regoBundleQuery rego(" << rego << ") bundle(" << bundle
                     << ")";
    try
    {
      rego::regoBundle* rb = reinterpret_cast<rego::regoBundle*>(bundle);
      if (rb->node_to_bundle(rego) != REGO_OK)
      {
        return nullptr;
      }

      auto interpreter = reinterpret_cast<rego::Interpreter*>(rego);
      rego::regoOutput* output = new rego::regoOutput();
      output->node = interpreter->query_bundle(rb->bundle);
      output->value = interpreter->output_to_string(output->node);
      auto ptr = reinterpret_cast<regoOutput*>(output);
      logging::Debug() << "regoBundleQuery output: " << ptr;
      return ptr;
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return nullptr;
    }
  }

  regoOutput* regoBundleQueryEntrypoint(
    regoInterpreter* rego, regoBundle* bundle, const char* entrypoint)
  {
    logging::Debug() << "regoBundleQueryEntrypoint: rego(" << rego
                     << ") bundle(" << bundle << ") " << entrypoint;
    try
    {
      rego::regoBundle* rb = reinterpret_cast<rego::regoBundle*>(bundle);
      if (rb->node_to_bundle(rego) != REGO_OK)
      {
        return nullptr;
      }

      auto interpreter = reinterpret_cast<rego::Interpreter*>(rego);
      rego::regoOutput* output = new rego::regoOutput();
      output->node = interpreter->query_bundle(rb->bundle, entrypoint);
      output->value = interpreter->output_to_string(output->node);
      auto ptr = reinterpret_cast<regoOutput*>(output);
      logging::Debug() << "regoBundleQueryEntrypoint output: " << ptr;
      return ptr;
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return nullptr;
    }
  }

  void regoFreeBundle(regoBundle* bundle)
  {
    logging::Debug() << "regoFreeBundle: " << bundle;
    delete reinterpret_cast<rego::regoBundle*>(bundle);
  }

  // Output functions
  regoBoolean regoOutputOk(regoOutput* output)
  {
    logging::Debug() << "regoOutputOk";
    if (output == nullptr)
    {
      return false;
    }

    return reinterpret_cast<rego::regoOutput*>(output)->node->type() !=
      rego::ErrorSeq;
  }

  regoSize regoOutputSize(regoOutput* output)
  {
    logging::Debug() << "regoOutputSize";
    auto output_ptr = reinterpret_cast<rego::regoOutput*>(output);
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(output_ptr->node.get());
    if (node_ptr->type() == rego::ErrorSeq)
    {
      return 0;
    }

    assert(node_ptr->type() == rego::Results);
    return static_cast<regoSize>(node_ptr->size());
  }

  regoNode* regoOutputExpressionsAtIndex(regoOutput* output, regoSize index)
  {
    logging::Debug() << "regoOutputExpressionsAtIndex: " << index;
    auto output_ptr = reinterpret_cast<rego::regoOutput*>(output);
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(output_ptr->node.get());
    if (node_ptr->type() == rego::ErrorSeq)
    {
      return nullptr;
    }

    assert(node_ptr->type() == rego::Results);
    trieste::WFContext context(rego::wf_result);

    for (auto result : *node_ptr)
    {
    }

    rego::Node result = node_ptr->at(index);
    rego::Node terms = result / rego::Terms;
    return reinterpret_cast<regoNode*>(terms.get());
  }

  regoNode* regoOutputExpressions(regoOutput* output)
  {
    logging::Debug() << "regoOutputExpressions";
    return regoOutputExpressionsAtIndex(output, 0);
  }

  regoNode* regoOutputNode(regoOutput* output)
  {
    logging::Debug() << "regoOutputNode";
    return reinterpret_cast<regoNode*>(
      reinterpret_cast<rego::regoOutput*>(output)->node.get());
  }

  regoNode* regoOutputBindingAtIndex(
    regoOutput* output, regoSize index, const char* name)
  {
    logging::Debug() << "regoOutputBindingAtIndex: " << name;
    auto output_ptr = reinterpret_cast<rego::regoOutput*>(output);
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(output_ptr->node.get());
    if (node_ptr->type() == rego::ErrorSeq)
    {
      return nullptr;
    }

    assert(node_ptr->type() == rego::Results);
    trieste::WFContext context(rego::wf_result);
    auto result = node_ptr->at(index);
    auto defs = result->lookdown({name});
    if (defs.empty())
    {
      return nullptr;
    }

    rego::Node val = defs[0] / rego::Val;
    if (val == rego::Term)
    {
      val = val->front();
    }

    return reinterpret_cast<regoNode*>(val.get());
  }

  regoNode* regoOutputBinding(regoOutput* output, const char* name)
  {
    logging::Debug() << "regoOutputBinding: " << name;
    return regoOutputBindingAtIndex(output, 0, name);
  }

  regoSize regoOutputJSONSize(regoOutput* output)
  {
    logging::Debug() << "regoOutputJSONSize";
    return static_cast<regoSize>(
      reinterpret_cast<rego::regoOutput*>(output)->value.size() + 1);
  }

  regoEnum regoOutputJSON(regoOutput* output, char* buffer, regoSize size)
  {
    logging::Debug() << "regoOutputJSON: " << (void*)buffer << "[" << size
                     << "]";
    auto output_ptr = reinterpret_cast<rego::regoOutput*>(output);
    auto& value = output_ptr->value;
    if (size < value.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    value.copy(buffer, value.size());
    buffer[value.size()] = '\0';
    return REGO_OK;
  }

  void regoFreeOutput(regoOutput* output)
  {
    logging::Debug() << "regoFreeOutput: " << output;
    delete reinterpret_cast<rego::regoOutput*>(output);
  }

  // Node functions
  regoType regoNodeType(regoNode* node_ptr)
  {
    logging::Debug() << "regoNodeType";
    auto node = reinterpret_cast<trieste::NodeDef*>(node_ptr);

    if (node == rego::Binding)
    {
      return REGO_NODE_BINDING;
    }

    if (node == rego::Var)
    {
      return REGO_NODE_VAR;
    }

    if (node == rego::Term)
    {
      return REGO_NODE_TERM;
    }

    if (node == rego::Scalar)
    {
      return REGO_NODE_SCALAR;
    }

    if (node == rego::Array)
    {
      return REGO_NODE_ARRAY;
    }

    if (node == rego::Set)
    {
      return REGO_NODE_SET;
    }

    if (node == rego::Object)
    {
      return REGO_NODE_OBJECT;
    }

    if (node == rego::ObjectItem)
    {
      return REGO_NODE_OBJECT_ITEM;
    }

    if (node == rego::Int)
    {
      return REGO_NODE_INT;
    }

    if (node == rego::Float)
    {
      return REGO_NODE_FLOAT;
    }

    if (node == rego::JSONString)
    {
      return REGO_NODE_STRING;
    }

    if (node == rego::True)
    {
      return REGO_NODE_TRUE;
    }

    if (node == rego::False)
    {
      return REGO_NODE_FALSE;
    }

    if (node == rego::Null)
    {
      return REGO_NODE_NULL;
    }

    if (node == rego::Undefined)
    {
      return REGO_NODE_UNDEFINED;
    }

    if (node == rego::Terms)
    {
      return REGO_NODE_TERMS;
    }

    if (node == rego::Bindings)
    {
      return REGO_NODE_BINDINGS;
    }

    if (node == rego::Results)
    {
      return REGO_NODE_RESULTS;
    }

    if (node == rego::Result)
    {
      return REGO_NODE_RESULT;
    }

    if (node == rego::Error)
    {
      return REGO_NODE_ERROR;
    }

    if (node == rego::ErrorMsg)
    {
      return REGO_NODE_ERROR_MESSAGE;
    }

    if (node == rego::ErrorAst)
    {
      return REGO_NODE_ERROR_AST;
    }

    if (node == rego::ErrorCode)
    {
      return REGO_NODE_ERROR_CODE;
    }

    if (node == rego::ErrorSeq)
    {
      return REGO_NODE_ERROR_SEQ;
    }

    return REGO_NODE_INTERNAL;
  }

  regoSize regoNodeTypeNameSize(regoNode* node)
  {
    logging::Debug() << "regoNodeTypeNameSize";
    return strlen(reinterpret_cast<trieste::NodeDef*>(node)->type().str());
  }

  regoEnum regoNodeTypeName(regoNode* node, char* buffer, regoSize size)
  {
    logging::Debug() << "regoNodeTypeName" << (void*)buffer << "[" << size
                     << "]";

    std::string_view view(
      reinterpret_cast<trieste::NodeDef*>(node)->type().str());
    if (size < view.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    view.copy(buffer, view.size());
    buffer[view.size()] = '\0';
    return REGO_OK;
  }

  regoSize regoNodeValueSize(regoNode* node)
  {
    logging::Debug() << "regoNodeValueSize";
    std::size_t size =
      reinterpret_cast<trieste::NodeDef*>(node)->location().view().size();
    return static_cast<regoSize>(size + 1);
  }

  regoEnum regoNodeValue(regoNode* node, char* buffer, regoSize size)
  {
    logging::Debug() << "regoNodeValue: " << (void*)buffer << "[" << size
                     << "]";
    std::string_view view =
      reinterpret_cast<trieste::NodeDef*>(node)->location().view();
    if (size < view.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    view.copy(buffer, view.size());
    buffer[view.size()] = '\0';
    return REGO_OK;
  }

  regoSize regoNodeSize(regoNode* node)
  {
    logging::Debug() << "regoNodeSize";
    std::size_t size = reinterpret_cast<trieste::NodeDef*>(node)->size();
    return static_cast<regoSize>(size);
  }

  regoNode* regoNodeGet(regoNode* node_ptr, regoSize index)
  {
    trieste::NodeDef* node = reinterpret_cast<trieste::NodeDef*>(node_ptr);
    logging::Debug() << "regoNodeGet: " << index << " of " << node->type().str()
                     << "{" << node->size() << "}";
    if (index >= node->size())
    {
      return nullptr;
    }

    rego::Node child = node->at(index);
    if (child == rego::Term)
    {
      child = child->front();
    }

    if (child == rego::Scalar)
    {
      child = child->front();
    }

    return reinterpret_cast<regoNode*>(child.get());
  }

  regoSize regoNodeJSONSize(regoNode* node)
  {
    logging::Debug() << "regoNodeJSONSize";
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(node);
    trieste::WFContext context(rego::wf_result);
    std::string json = rego::to_key(node_ptr->intrusive_ptr_from_this(), true);
    return static_cast<regoSize>(json.size() + 1);
  }

  regoEnum regoNodeJSON(regoNode* node, char* buffer, regoSize size)
  {
    logging::Debug() << "regoNodeJSON: " << (void*)buffer << "[" << size << "]";

    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(node);
    trieste::WFContext context(rego::wf_result);
    std::string json = rego::to_key(node_ptr->intrusive_ptr_from_this(), true);
    if (size < json.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    json.copy(buffer, json.size());
    buffer[json.size()] = '\0';
    return REGO_OK;
  }

  regoInput* regoNewInput()
  {
    logging::Debug() << "regoNewInput";
    return new rego::regoInput();
  }

  regoEnum regoInputInt(regoInput* input, regoInt value)
  {
    logging::Debug() << "regoInputInt: " << input << " << " << value;
    rego::regoInput* ri;
    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    ri->stack.push_back(rego::Scalar << (rego::Int ^ std::to_string(value)));
    return REGO_OK;
  }

  regoEnum regoInputFloat(regoInput* input, double value)
  {
    logging::Debug() << "regoInputFloat: " << input << " << " << value;
    rego::regoInput* ri;
    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    ri->stack.push_back(rego::Scalar << (rego::Float ^ std::to_string(value)));
    return REGO_OK;
  }

  regoEnum regoInputString(regoInput* input, const char* value)
  {
    logging::Debug() << "regoInputString: " << input << " << " << value;
    rego::regoInput* ri;
    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    ri->stack.push_back(rego::Scalar << (rego::JSONString ^ value));
    return REGO_OK;
  }

  regoEnum regoInputBoolean(regoInput* input, regoBoolean value)
  {
    logging::Debug() << "regoInputBoolean: " << input << " << " << value;
    rego::regoInput* ri;
    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    if (value)
    {
      ri->stack.push_back(rego::Scalar << (rego::True ^ "true"));
    }
    else
    {
      ri->stack.push_back(rego::Scalar << (rego::False ^ "false"));
    }

    return REGO_OK;
  }

  regoEnum regoInputNull(regoInput* input)
  {
    logging::Debug() << "regoInputNull: " << input;
    rego::regoInput* ri;
    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    ri->stack.push_back(rego::Scalar << (rego::Null ^ "null"));
    return REGO_OK;
  }

  regoEnum regoInputObjectItem(regoInput* input)
  {
    logging::Debug() << "regoInputObjectItem: " << input;
    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    rego::regoInput* ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->stack.size() < 2)
    {
      ri->status = REGO_ERROR_INPUT_MISSING_ARGUMENTS;
    }

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    rego::Node value = ri->stack.back();
    ri->stack.pop_back();
    rego::Node key = ri->stack.back();
    ri->stack.pop_back();
    ri->stack.push_back(
      rego::ObjectItem << rego::Resolver::to_term(key)
                       << rego::Resolver::to_term(value));
    return REGO_OK;
  }

  regoEnum regoInputObject(regoInput* input, regoSize count)
  {
    logging::Debug() << "regoInputObject: " << input << " << " << count
                     << " items";

    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    rego::regoInput* ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->stack.size() < count)
    {
      ri->status = REGO_ERROR_INPUT_MISSING_ARGUMENTS;
    }

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    auto start = ri->stack.end() - count;
    auto end = ri->stack.end();

    for (auto it = start; it != end; ++it)
    {
      if (*it != rego::ObjectItem)
      {
        ri->status = REGO_ERROR_INPUT_OBJECT_ITEM;
        return ri->status;
      }
    }

    rego::Node object = rego::NodeDef::create(rego::Object);
    object->push_back({start, end});
    ri->stack.erase(start, end);
    ri->stack.push_back(object);
    return REGO_OK;
  }

  regoEnum regoInputArray(regoInput* input, regoSize count)
  {
    logging::Debug() << "regoInputArray: " << input << " << " << count
                     << " items";

    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    rego::regoInput* ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->stack.size() < count)
    {
      ri->status = REGO_ERROR_INPUT_MISSING_ARGUMENTS;
    }

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    auto start = ri->stack.end() - count;
    auto end = ri->stack.end();
    rego::Node array = rego::NodeDef::create(rego::Array);
    array->push_back({start, end});
    ri->stack.erase(start, end);
    ri->stack.push_back(array);
    return REGO_OK;
  }

  regoEnum regoInputSet(regoInput* input, regoSize count)
  {
    logging::Debug() << "regoInputSet: " << input << " << " << count
                     << " items";

    if (input == NULL)
    {
      return REGO_ERROR_INPUT_NULL;
    }

    rego::regoInput* ri = reinterpret_cast<rego::regoInput*>(input);

    if (ri->stack.size() < count)
    {
      ri->status = REGO_ERROR_INPUT_MISSING_ARGUMENTS;
    }

    if (ri->status != REGO_OK)
    {
      return ri->status;
    }

    auto start = ri->stack.end() - count;
    auto end = ri->stack.end();
    rego::Node set = rego::NodeDef::create(rego::Set);
    set->push_back({start, end});
    set = rego::Resolver::set(set);
    ri->stack.erase(start, end);
    ri->stack.push_back(set);
    return REGO_OK;
  }

  regoNode* regoInputNode(regoInput* input)
  {
    logging::Debug() << "regoInputNode";
    rego::regoInput* ri = reinterpret_cast<rego::regoInput*>(input);
    if (ri->stack.empty())
    {
      return NULL;
    }

    return reinterpret_cast<regoNode*>(ri->stack.back().get());
  }

  regoEnum regoInputValidate(regoInput* input)
  {
    logging::Debug() << "regoInputValidate";
    rego::regoInput* ri = reinterpret_cast<rego::regoInput*>(input);
    if (ri->stack.empty())
    {
      return REGO_ERROR_INPUT_NULL;
    }

    return ri->status;
  }

  regoSize regoInputSize(regoInput* input)
  {
    logging::Debug() << "regoInputSize";
    return reinterpret_cast<rego::regoInput*>(input)->stack.size();
  }

  void regoFreeInput(regoInput* input)
  {
    logging::Debug() << "regoFreeInput: " << input;
    delete reinterpret_cast<rego::regoInput*>(input);
  }
}