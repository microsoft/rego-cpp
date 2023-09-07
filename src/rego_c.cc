#include "rego_c.h"

#include "interpreter.h"

namespace rego
{
  void setError(regoInterpreter* rego, const std::string& error)
  {
    reinterpret_cast<rego::Interpreter*>(rego)->m_c_error = error;
  }
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

  void regoDelete(regoInterpreter* rego)
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

  regoEnum regoAddInputJSONFile(regoInterpreter* rego, const char* path)
  {
    try
    {
      reinterpret_cast<rego::Interpreter*>(rego)->add_input_json_file(path);
      return REGO_OK;
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return REGO_ERROR;
    }
  }

  regoEnum regoAddInputJSON(regoInterpreter* rego, const char* contents)
  {
    try
    {
      reinterpret_cast<rego::Interpreter*>(rego)->add_input_json(contents);
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

  void regoSetDebugPath(regoInterpreter* rego, const char* path)
  {
    reinterpret_cast<rego::Interpreter*>(rego)->debug_path(path);
  }

  const char* regoGetDebugPath(regoInterpreter* rego)
  {
    return reinterpret_cast<rego::Interpreter*>(rego)->debug_path().c_str();
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

  void regoSetExecutable(regoInterpreter* rego, const char* path)
  {
    reinterpret_cast<rego::Interpreter*>(rego)->executable(path);
  }

  const char* regoGetExecutable(regoInterpreter* rego)
  {
    return reinterpret_cast<rego::Interpreter*>(rego)->executable().c_str();
  }

  regoNode* regoRawQuery(regoInterpreter* rego, const char* query_expr)
  {
    try
    {
      rego::Node result =
        reinterpret_cast<rego::Interpreter*>(rego)->raw_query(query_expr);
      return reinterpret_cast<regoNode*>(result.get());
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return nullptr;
    }
  }

  const char* regoQuery(regoInterpreter* rego, const char* query_expr)
  {
    try
    {
      auto interpreter = reinterpret_cast<rego::Interpreter*>(rego);
      interpreter->m_c_result = interpreter->query(query_expr);
      return interpreter->m_c_result.c_str();
    }
    catch (const std::exception& e)
    {
      rego::setError(rego, e.what());
      return nullptr;
    }
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

    return REGO_NODE_INTERNAL;
  }

  const char* regoNodeValue(regoNode* node)
  {
    return reinterpret_cast<trieste::NodeDef*>(node)->location().view().data();
  }

  regoSize regoNodeSize(regoNode* node)
  {
    return reinterpret_cast<trieste::NodeDef*>(node)->size();
  }

  regoNode* regoNodeGet(regoNode* node, regoSize index)
  {
    trieste::NodeDef* child =
      reinterpret_cast<trieste::NodeDef*>(node)->at(index).get();
    return reinterpret_cast<regoNode*>(child);
  }

  void regoNodeToJSON(regoNode* node, char* buffer, regoSize size)
  {
    auto node_ptr = reinterpret_cast<trieste::NodeDef*>(node);
    std::string json = rego::to_json(node_ptr->shared_from_this());
    json.copy(buffer, size);
  }
}