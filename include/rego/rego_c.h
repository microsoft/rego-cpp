// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _REGOCPP_WRAPPER_H_
#define _REGOCPP_WRAPPER_H_

typedef void regoInterpreter;
typedef void regoNode;
typedef unsigned int regoEnum;
typedef unsigned char regoBoolean;
typedef unsigned int regoSize;

// error codes
#define REGO_OK 0
#define REGO_ERROR 1

// term node types
#define REGO_NODE_SCALAR 100
#define REGO_NODE_ARRAY 101
#define REGO_NODE_SET 102
#define REGO_NODE_OBJECT 103
#define REGO_NODE_OBJECT_ITEM 104
#define REGO_NODE_INT 105
#define REGO_NODE_FLOAT 106
#define REGO_NODE_STRING 107
#define REGO_NODE_TRUE 108
#define REGO_NODE_FALSE 109
#define REGO_NODE_NULL 110
#define REGO_NODE_UNDEFINED 111
#define REGO_NODE_ERROR 112
#define REGO_NODE_ERROR_MESSAGE 113
#define REGO_NODE_ERROR_AST 114
#define REGO_NODE_ERROR_CODE 115
#define REGO_NODE_INTERNAL 116

#ifdef __cplusplus
extern "C"
{
#endif
  // Interpreter functions
  void regoSetLoggingEnabled(regoBoolean enabled);
  regoInterpreter* regoNew();
  void regoDelete(regoInterpreter* rego);
  regoEnum regoAddModuleFile(regoInterpreter* rego, const char* path);
  regoEnum regoAddModule(regoInterpreter* rego, const char* name, const char* contents);
  regoEnum regoAddDataJSONFile(regoInterpreter* rego, const char* path);
  regoEnum regoAddDataJSON(regoInterpreter* rego, const char* contents);
  regoEnum regoAddInputJSONFile(regoInterpreter* rego, const char* path);
  regoEnum regoAddInputJSON(regoInterpreter* rego, const char* contents);
  void regoSetDebugEnabled(regoInterpreter* rego, regoBoolean enabled);
  regoBoolean regoGetDebugEnabled(regoInterpreter* rego);
  void regoSetDebugPath(regoInterpreter* rego, const char* path);
  const char* regoGetDebugPath(regoInterpreter* rego);
  void regoSetWellFormedChecksEnabled(regoInterpreter* rego, regoBoolean enabled);
  regoBoolean regoGetWellFormedChecksEnabled(regoInterpreter* rego);
  void regoSetExecutable(regoInterpreter* rego, const char* path);
  const char* regoGetExecutable(regoInterpreter* rego);
  regoNode* regoRawQuery(regoInterpreter* rego, const char* query_expr);
  const char* regoQuery(regoInterpreter* rego, const char* query_expr);
  const char* regoGetError(regoInterpreter* rego);

  // Node functions
  regoEnum regoNodeType(regoNode* node);
  const char* regoNodeLocation(regoNode* node);
  regoSize regoNodeSize(regoNode* node);
  regoNode* regoNodeGet(regoNode* node, regoSize index);
  void regoNodeToJSON(regoNode* node, char* buffer, regoSize size);

#ifdef __cplusplus
}
#endif

#endif