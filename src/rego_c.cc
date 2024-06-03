#include "internal.hh"

namespace logging = trieste::logging;

namespace rego
{
  void setError(regoInterpreter* rego, const std::string& error)
  {
    reinterpret_cast<rego::Interpreter*>(rego)->m_c_error = error;
  }

  struct regoOutput
  {
    Node node;
    std::string value;
  };
}

extern "C"
{
  const char* regoGetError(regoInterpreter* rego)
  {
    return reinterpret_cast<rego::Interpreter*>(rego)->m_c_error.c_str();
  }

  regoEnum regoSetLogLevel(regoEnum level)
  {
    switch (level)
    {
      case REGO_LOG_LEVEL_NONE:
        rego::set_log_level(rego::LogLevel::None);
        break;

      case REGO_LOG_LEVEL_ERROR:
        rego::set_log_level(rego::LogLevel::Error);
        break;

      case REGO_LOG_LEVEL_OUTPUT:
        rego::set_log_level(rego::LogLevel::Output);
        break;

      case REGO_LOG_LEVEL_WARN:
        rego::set_log_level(rego::LogLevel::Warn);
        break;

      case REGO_LOG_LEVEL_INFO:
        rego::set_log_level(rego::LogLevel::Info);
        break;

      case REGO_LOG_LEVEL_DEBUG:
        rego::set_log_level(rego::LogLevel::Debug);
        break;

      case REGO_LOG_LEVEL_TRACE:
        rego::set_log_level(rego::LogLevel::Trace);
        break;

      default:
        return REGO_ERROR_INVALID_LOG_LEVEL;
    }

    return REGO_OK;
  }

  regoEnum regoSetLogLevelFromString(const char* level)
  {
    std::string error = rego::set_log_level_from_string(level);
    if (error.empty())
    {
      return REGO_OK;
    }

    return REGO_ERROR_INVALID_LOG_LEVEL;
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
      reinterpret_cast<rego::Interpreter*>(rego)->add_module_file(path);
      return REGO_OK;
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
      reinterpret_cast<rego::Interpreter*>(rego)->add_module(name, contents);
      return REGO_OK;
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
      reinterpret_cast<rego::Interpreter*>(rego)->add_data_json_file(path);
      return REGO_OK;
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
      reinterpret_cast<rego::Interpreter*>(rego)->add_data_json(contents);
      return REGO_OK;
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
      reinterpret_cast<rego::Interpreter*>(rego)->set_input_json_file(path);
      return REGO_OK;
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
      reinterpret_cast<rego::Interpreter*>(rego)->set_input_term(contents);
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

  // Output functions
  regoBoolean regoOutputOk(regoOutput* output)
  {
    logging::Debug() << "regoOutputOk";
    return reinterpret_cast<rego::regoOutput*>(output)->node->type() !=
      rego::ErrorSeq;
  }

  regoNode* regoOutputNode(regoOutput* output)
  {
    logging::Debug() << "regoOutputNode";
    return reinterpret_cast<regoNode*>(
      reinterpret_cast<rego::regoOutput*>(output)->node.get());
  }

  regoNode* regoOutputBinding(regoOutput* output, const char* name)
  {
    logging::Debug() << "regoOutputBinding: " << name;
    auto output_ptr = reinterpret_cast<rego::regoOutput*>(output);
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(output_ptr->node.get());
    if (node_ptr->type() == rego::ErrorSeq)
    {
      return nullptr;
    }

    assert(node_ptr->type() == rego::Result);
    trieste::WFContext context(rego::wf_result);
    auto defs = node_ptr->lookdown({name});
    if (defs.empty())
    {
      return nullptr;
    }

    rego::Node val = defs[0] / rego::Term;
    return reinterpret_cast<regoNode*>(val.get());
  }

  const char* regoOutputString(regoOutput* output)
  {
    logging::Debug() << "regoOutputString";
    return reinterpret_cast<rego::regoOutput*>(output)->value.c_str();
  }

  void regoFreeOutput(regoOutput* output)
  {
    logging::Debug() << "regoFreeOutput: " << output;
    delete reinterpret_cast<rego::regoOutput*>(output);
  }

  // Node functions
  regoEnum regoNodeType(regoNode* node_ptr)
  {
    logging::Debug() << "regoNodeType";
    auto node = reinterpret_cast<trieste::NodeDef*>(node_ptr);

    if (node->type() == rego::Binding)
    {
      return REGO_NODE_BINDING;
    }

    if (node->type() == rego::Var)
    {
      return REGO_NODE_VAR;
    }

    if (node->type() == rego::Term)
    {
      return REGO_NODE_TERM;
    }

    if (node->type() == rego::Scalar)
    {
      return REGO_NODE_SCALAR;
    }

    if (node->type() == rego::Array)
    {
      return REGO_NODE_ARRAY;
    }

    if (node->type() == rego::Set)
    {
      return REGO_NODE_SET;
    }

    if (node->type() == rego::Object)
    {
      return REGO_NODE_OBJECT;
    }

    if (node->type() == rego::ObjectItem)
    {
      return REGO_NODE_OBJECT_ITEM;
    }

    if (node->type() == rego::Int)
    {
      return REGO_NODE_INT;
    }

    if (node->type() == rego::Float)
    {
      return REGO_NODE_FLOAT;
    }

    if (node->type() == rego::JSONString)
    {
      return REGO_NODE_STRING;
    }

    if (node->type() == rego::True)
    {
      return REGO_NODE_TRUE;
    }

    if (node->type() == rego::False)
    {
      return REGO_NODE_FALSE;
    }

    if (node->type() == rego::Null)
    {
      return REGO_NODE_NULL;
    }

    if (node->type() == rego::Undefined)
    {
      return REGO_NODE_UNDEFINED;
    }

    if (node->type() == rego::Error)
    {
      return REGO_NODE_ERROR;
    }

    if (node->type() == rego::ErrorMsg)
    {
      return REGO_NODE_ERROR_MESSAGE;
    }

    if (node->type() == rego::ErrorAst)
    {
      return REGO_NODE_ERROR_AST;
    }

    if (node->type() == rego::ErrorCode)
    {
      return REGO_NODE_ERROR_CODE;
    }

    if (node->type() == rego::ErrorSeq)
    {
      return REGO_NODE_ERROR_SEQ;
    }

    return REGO_NODE_INTERNAL;
  }

  const char* regoNodeTypeName(regoNode* node)
  {
    logging::Debug() << "regoNodeTypeName";
    return reinterpret_cast<trieste::NodeDef*>(node)->type().str();
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
    logging::Debug() << "regoNodeValue: " << buffer << "[" << size << "]";
    std::string_view view =
      reinterpret_cast<trieste::NodeDef*>(node)->location().view();
    if (size < view.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    view.copy(buffer, size);
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
    logging::Debug() << "regoNodeGet: " << index;
    trieste::NodeDef* node = reinterpret_cast<trieste::NodeDef*>(node_ptr);
    if (index >= node->size())
    {
      return nullptr;
    }
    trieste::NodeDef* child = node->at(index).get();
    return reinterpret_cast<regoNode*>(child);
  }

  regoSize regoNodeJSONSize(regoNode* node)
  {
    logging::Debug() << "regoNodeJSONSize";
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(node);
    trieste::WFContext context(rego::wf_result);
    std::string json = rego::to_key(node_ptr->shared_from_this(), true);
    return static_cast<regoSize>(json.size() + 1);
  }

  regoEnum regoNodeJSON(regoNode* node, char* buffer, regoSize size)
  {
    logging::Debug() << "regoNodeJSON: " << buffer << "[" << size << "]";

    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(node);
    trieste::WFContext context(rego::wf_result);
    std::string json = rego::to_key(node_ptr->shared_from_this(), true);
    if (size < json.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    json.copy(buffer, size);
    buffer[json.size()] = '\0';
    return REGO_OK;
  }
}