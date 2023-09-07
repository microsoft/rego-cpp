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
#define REGO_NODE_BINDING 1000
#define REGO_NODE_VAR 1001
#define REGO_NODE_TERM 1002
#define REGO_NODE_SCALAR 1003
#define REGO_NODE_ARRAY 1004
#define REGO_NODE_SET 1005
#define REGO_NODE_OBJECT 1006
#define REGO_NODE_OBJECT_ITEM 1007
#define REGO_NODE_INT 1008
#define REGO_NODE_FLOAT 1009
#define REGO_NODE_STRING 1010
#define REGO_NODE_TRUE 1011
#define REGO_NODE_FALSE 1012
#define REGO_NODE_NULL 1013
#define REGO_NODE_UNDEFINED 1014

#define REGO_NODE_ERROR 1800
#define REGO_NODE_ERROR_MESSAGE 1801
#define REGO_NODE_ERROR_AST 1802
#define REGO_NODE_ERROR_CODE 1803

#define REGO_NODE_INTERNAL 1999

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
  const char* regoNodeValue(regoNode* node);
  regoSize regoNodeSize(regoNode* node);
  regoNode* regoNodeGet(regoNode* node, regoSize index);
  void regoNodeToJSON(regoNode* node, char* buffer, regoSize size);

#ifdef __cplusplus
}
#endif

#endif
