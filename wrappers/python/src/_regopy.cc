#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <rego/rego_c.h>

using namespace pybind11::literals;

namespace {
  std::string get_value(regoNode* node)
  {
    regoSize size = regoNodeValueSize(node);
    std::vector<char> value(size);
    regoNodeValue(node, value.data(), size);
    return std::string(value.begin(), value.end() - 1);
  }

  std::string get_json(regoNode* node)
  {
    regoSize size = regoNodeJSONSize(node);
    std::vector<char> json(size);
    regoNodeJSON(node, json.data(), size);
    return std::string(json.begin(), json.end() - 1);
  }
}

PYBIND11_MODULE(_regopy, m)
{
  m.attr("REGOCPP_VERSION") = REGOCPP_VERSION;
  m.attr("REGOCPP_OPA_VERSION") = REGOCPP_OPA_VERSION;
  m.attr("REGOCPP_GIT_HASH") = REGOCPP_GIT_HASH;
  m.attr("REGOCPP_BUILD_NAME") = REGOCPP_BUILD_NAME;
  m.attr("REGOCPP_BUILD_DATE") = REGOCPP_BUILD_DATE;
  m.attr("REGOCPP_BUILD_TOOLCHAIN") = REGOCPP_BUILD_TOOLCHAIN;
  m.attr("REGOCPP_PLATFORM") = REGOCPP_PLATFORM;

  // error codes
  m.attr("REGO_OK") = REGO_OK;
  m.attr("REGO_ERROR") = REGO_ERROR;

  // term node types
  m.attr("REGO_NODE_BINDING") = REGO_NODE_BINDING;
  m.attr("REGO_NODE_VAR") = REGO_NODE_VAR;
  m.attr("REGO_NODE_TERM") = REGO_NODE_TERM;
  m.attr("REGO_NODE_SCALAR") = REGO_NODE_SCALAR;
  m.attr("REGO_NODE_ARRAY") = REGO_NODE_ARRAY;
  m.attr("REGO_NODE_SET") = REGO_NODE_SET;
  m.attr("REGO_NODE_OBJECT") = REGO_NODE_OBJECT;
  m.attr("REGO_NODE_OBJECT_ITEM") = REGO_NODE_OBJECT_ITEM;
  m.attr("REGO_NODE_INT") = REGO_NODE_INT;
  m.attr("REGO_NODE_FLOAT") = REGO_NODE_FLOAT;
  m.attr("REGO_NODE_STRING") = REGO_NODE_STRING;
  m.attr("REGO_NODE_TRUE") = REGO_NODE_TRUE;
  m.attr("REGO_NODE_FALSE") = REGO_NODE_FALSE;
  m.attr("REGO_NODE_NULL") = REGO_NODE_NULL;
  m.attr("REGO_NODE_UNDEFINED") = REGO_NODE_UNDEFINED;

  m.attr("REGO_NODE_ERROR") = REGO_NODE_ERROR;
  m.attr("REGO_NODE_ERROR_MESSAGE") = REGO_NODE_ERROR_MESSAGE;
  m.attr("REGO_NODE_ERROR_AST") = REGO_NODE_ERROR_AST;
  m.attr("REGO_NODE_ERROR_CODE") = REGO_NODE_ERROR_CODE;
  m.attr("REGO_NODE_ERROR_SEQ") = REGO_NODE_ERROR_SEQ;

  m.attr("REGO_NODE_INTERNAL") = REGO_NODE_INTERNAL;

  // log levels
  m.attr("REGO_LOG_LEVEL_NONE") = REGO_LOG_LEVEL_NONE;
  m.attr("REGO_LOG_LEVEL_ERROR") = REGO_LOG_LEVEL_ERROR;
  m.attr("REGO_LOG_LEVEL_WARN") = REGO_LOG_LEVEL_WARN;
  m.attr("REGO_LOG_LEVEL_INFO") = REGO_LOG_LEVEL_INFO;
  m.attr("REGO_LOG_LEVEL_DEBUG") = REGO_LOG_LEVEL_DEBUG;
  m.attr("REGO_LOG_LEVEL_TRACE") = REGO_LOG_LEVEL_TRACE;

  // Interpreter functions
  m.def(
    "regoSetLogLevel",
    &regoSetLogLevel,
    "Sets the level of logging.",
    "level"_a);
  m.def("regoNew", &regoNew, "Returns a pointer to a new rego instance.");
  m.def("regoFree", &regoFree, "Deletes a rego instance.");
  m.def(
    "regoAddModuleFile",
    &regoAddModuleFile,
    "Adds a module file to the rego instance.",
    "rego"_a,
    "filename"_a);
  m.def(
    "regoAddModule",
    &regoAddModule,
    "Adds a module to the rego instance.",
    "rego"_a,
    "name"_a,
    "contents"_a);
  m.def(
    "regoAddDataJSONFile",
    &regoAddDataJSONFile,
    "Adds a base document file to the rego instance.",
    "rego"_a,
    "filename"_a);
  m.def(
    "regoAddDataJSON",
    &regoAddDataJSON,
    "Adds a base document to the rego instance.",
    "rego"_a,
    "contents"_a);
  m.def(
    "regoSetInputJSONFile",
    &regoSetInputJSONFile,
    "Sets the input document file for the rego instance.",
    "rego"_a,
    "filename"_a);
  m.def(
    "regoSetInputJSON",
    &regoSetInputJSON,
    "Sets the input document for the rego instance.",
    "rego"_a,
    "contents"_a);
  m.def(
    "regoSetDebugEnabled",
    &regoSetDebugEnabled,
    "Sets the debug enabled flag for the rego instance.",
    "rego"_a,
    "enabled"_a);
  m.def(
    "regoGetDebugEnabled",
    &regoGetDebugEnabled,
    "Gets the debug enabled flag for the rego instance.",
    "rego"_a);
  m.def(
    "regoSetDebugPath",
    &regoSetDebugPath,
    "Sets the debug path for the rego instance.",
    "rego"_a,
    "path"_a);
  m.def(
    "regoSetWellFormedChecksEnabled",
    &regoSetWellFormedChecksEnabled,
    "Sets the well formed checks enabled flag for the rego instance.",
    "rego"_a,
    "enabled"_a);
  m.def(
    "regoGetWellFormedChecksEnabled",
    &regoGetWellFormedChecksEnabled,
    "Gets the well formed checks enabled flag for the rego instance.",
    "rego"_a);
  m.def(
    "regoQuery",
    &regoQuery,
    "Queries the rego instance.",
    "rego"_a,
    "query_expr"_a);
  m.def(
    "regoSetStrictBuiltInErrors",
    &regoSetStrictBuiltInErrors,
    "Sets the strict built-in errors flag for the rego instance.",
    "rego"_a,
    "enabled"_a);
  m.def(
    "regoGetStrictBuiltInErrors",
    &regoGetStrictBuiltInErrors,
    "Gets the strict built-in errors flag for the rego instance.",
    "rego"_a);
  m.def(
    "regoGetError",
    &regoGetError,
    "Gets the error from the rego instance.",
    "rego"_a);

  // Output functions
  m.def(
    "regoOutputOk",
    &regoOutputOk,
    "Returns true if the output is ok.",
    "output"_a);
  m.def(
    "regoOutputNode", &regoOutputNode, "Returns the output node.", "output"_a);
  m.def(
    "regoOutputBinding",
    &regoOutputBinding,
    "Returns the output binding.",
    "output"_a,
    "name"_a);
  m.def(
    "regoOutputString",
    &regoOutputString,
    "Returns the output string.",
    "output"_a);
  m.def("regoFreeOutput", &regoFreeOutput, "Frees the output.", "output"_a);

  // Node functions
  m.def("regoNodeType", &regoNodeType, "Returns the node type.", "node"_a);
  m.def(
    "regoNodeTypeName",
    &regoNodeTypeName,
    "Returns a human-readable node type name.",
    "node"_a);
  m.def(
    "regoNodeValue",
    &get_value,
    "Gets the node value as a string.",
    "node"_a);
  m.def("regoNodeSize", &regoNodeSize, "Returns the node size.", "node"_a);
  m.def(
    "regoNodeGet",
    &regoNodeGet,
    "Returns the node at the specified index.",
    "node"_a,
    "index"_a);
  m.def(
    "regoNodeJSON",
    &get_json,
    "Gets the node's JSON representation.",
    "node"_a);
}