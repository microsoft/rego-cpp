#include "rego_c.h"

#include "helpers.h"
#include "interpreter.h"
#include "log.h"

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

  void regoSetLoggingEnabled(regoBoolean enabled)
  {
    rego::Logger::enabled = enabled;
  }

  regoInterpreter* regoNew()
  {
    return reinterpret_cast<regoInterpreter*>(new rego::Interpreter());
  }

  void regoFree(regoInterpreter* rego)
  {
    delete reinterpret_cast<rego::Interpreter*>(rego);
  }

  regoEnum regoAddModuleFile(regoInterpreter* rego, const char* path)
  {
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
    try
    {
      reinterpret_cast<rego::Interpreter*>(rego)->set_input_json(contents);
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
    reinterpret_cast<rego::Interpreter*>(rego)->debug_enabled(enabled);
  }

  regoBoolean regoGetDebugEnabled(regoInterpreter* rego)
  {
    return reinterpret_cast<rego::Interpreter*>(rego)->debug_enabled();
  }

  const char* regoGetDebugPath(regoInterpreter* rego)
  {
    return reinterpret_cast<rego::Interpreter*>(rego)->debug_path().c_str();
  }

  regoEnum regoSetDebugPath(regoInterpreter* rego, const char* path)
  {
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
    reinterpret_cast<rego::Interpreter*>(rego)->well_formed_checks_enabled(
      enabled);
  }

  regoBoolean regoGetWellFormedChecksEnabled(regoInterpreter* rego)
  {
    return reinterpret_cast<rego::Interpreter*>(rego)
      ->well_formed_checks_enabled();
  }

  regoOutput* regoQuery(regoInterpreter* rego, const char* query_expr)
  {
    try
    {
      auto interpreter = reinterpret_cast<rego::Interpreter*>(rego);
      rego::regoOutput* output = new rego::regoOutput();
      output->node = interpreter->raw_query(query_expr);
      output->value = interpreter->output_to_string(output->node);

      return reinterpret_cast<regoOutput*>(output);
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return nullptr;
    }
  }

  void regoSetStrictBuiltInErrors(regoInterpreter* rego, regoBoolean enabled)
  {
    reinterpret_cast<rego::Interpreter*>(rego)->builtins().strict_errors(enabled);
  }

  regoBoolean regoGetStrictBuiltInErrors(regoInterpreter* rego)
  {
    return reinterpret_cast<rego::Interpreter*>(rego)
      ->builtins()
      .strict_errors();
  }

  // Output functions
  regoBoolean regoOutputOk(regoOutput* output)
  {
    return reinterpret_cast<rego::regoOutput*>(output)->node->type() !=
      rego::ErrorSeq;
  }

  regoNode* regoOutputNode(regoOutput* output)
  {
    return reinterpret_cast<regoNode*>(
      reinterpret_cast<rego::regoOutput*>(output)->node.get());
  }

  regoNode* regoOutputBinding(regoOutput* output, const char* name)
  {
    auto output_ptr = reinterpret_cast<rego::regoOutput*>(output);
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(output_ptr->node.get());
    if (node_ptr->type() == rego::ErrorSeq)
    {
      return nullptr;
    }

    for (auto binding : *node_ptr)
    {
      rego::Node var = binding / rego::Var;
      if (var->location().view() == name)
      {
        rego::Node val = binding / rego::Term;
        return reinterpret_cast<regoNode*>(val.get());
      }
    }

    return nullptr;
  }

  const char* regoOutputString(regoOutput* output)
  {
    return reinterpret_cast<rego::regoOutput*>(output)->value.c_str();
  }

  void regoFreeOutput(regoOutput* output)
  {
    delete reinterpret_cast<rego::regoOutput*>(output);
  }

  // Node functions
  regoEnum regoNodeType(regoNode* node_ptr)
  {
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

    if (node->type() == rego::JSONInt)
    {
      return REGO_NODE_INT;
    }

    if (node->type() == rego::JSONFloat)
    {
      return REGO_NODE_FLOAT;
    }

    if (node->type() == rego::JSONString)
    {
      return REGO_NODE_STRING;
    }

    if (node->type() == rego::JSONTrue)
    {
      return REGO_NODE_TRUE;
    }

    if (node->type() == rego::JSONFalse)
    {
      return REGO_NODE_FALSE;
    }

    if (node->type() == rego::JSONNull)
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
    return reinterpret_cast<trieste::NodeDef*>(node)->type().str();
  }

  regoSize regoNodeValueSize(regoNode* node)
  {
    std::size_t size =
      reinterpret_cast<trieste::NodeDef*>(node)->location().view().size();
    return static_cast<regoSize>(size + 1);
  }

  regoEnum regoNodeValue(regoNode* node, char* buffer, regoSize size)
  {
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
    std::size_t size = reinterpret_cast<trieste::NodeDef*>(node)->size();
    return static_cast<regoSize>(size);
  }

  regoNode* regoNodeGet(regoNode* node_ptr, regoSize index)
  {
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
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(node);
    std::string json = rego::to_json(node_ptr->shared_from_this());
    return static_cast<regoSize>(json.size() + 1);
  }

  regoEnum regoNodeJSON(regoNode* node, char* buffer, regoSize size)
  {
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(node);
    std::string json = rego::to_json(node_ptr->shared_from_this());
    if (size < json.size() + 1)
    {
      return REGO_ERROR_BUFFER_TOO_SMALL;
    }

    json.copy(buffer, size);
    return REGO_OK;
  }
}