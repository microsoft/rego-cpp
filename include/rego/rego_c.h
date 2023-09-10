// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _REGO_C_H_
#define _REGO_C_H_

#include "version.h"

typedef void regoInterpreter;
typedef void regoNode;
typedef void regoOutput;
typedef unsigned int regoEnum;
typedef unsigned char regoBoolean;
typedef unsigned int regoSize;

// error codes
#define REGO_OK 0
#define REGO_ERROR 1
#define REGO_ERROR_BUFFER_TOO_SMALL 2

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
#define REGO_NODE_ERROR_SEQ 1804

#define REGO_NODE_INTERNAL 1999

#ifdef __cplusplus
extern "C"
{
#endif
  // Interpreter functions
  void regoSetLoggingEnabled(regoBoolean enabled);
  regoInterpreter* regoNew();
  void regoFree(regoInterpreter* rego);
  regoEnum regoAddModuleFile(regoInterpreter* rego, const char* path);
  regoEnum regoAddModule(
    regoInterpreter* rego, const char* name, const char* contents);
  regoEnum regoAddDataJSONFile(regoInterpreter* rego, const char* path);
  regoEnum regoAddDataJSON(regoInterpreter* rego, const char* contents);
  regoEnum regoSetInputJSONFile(regoInterpreter* rego, const char* path);
  regoEnum regoSetInputJSON(regoInterpreter* rego, const char* contents);
  void regoSetDebugEnabled(regoInterpreter* rego, regoBoolean enabled);
  regoBoolean regoGetDebugEnabled(regoInterpreter* rego);
  regoEnum regoSetDebugPath(regoInterpreter* rego, const char* path);
  void regoSetWellFormedChecksEnabled(
    regoInterpreter* rego, regoBoolean enabled);
  regoBoolean regoGetWellFormedChecksEnabled(regoInterpreter* rego);
  regoOutput* regoQuery(regoInterpreter* rego, const char* query_expr);
  void regoSetStrictBuiltInErrors(regoInterpreter* rego, regoBoolean enabled);
  regoBoolean regoGetStrictBuiltInErrors(regoInterpreter* rego);
  const char* regoGetError(regoInterpreter* rego);

  // Output functions
  regoBoolean regoOutputOk(regoOutput* output);
  regoNode* regoOutputNode(regoOutput* output);
  regoNode* regoOutputBinding(regoOutput* output, const char* name);
  const char* regoOutputString(regoOutput* output);
  void regoFreeOutput(regoOutput* output);

  // Node functions
  regoEnum regoNodeType(regoNode* node);
  const char* regoNodeTypeName(regoNode* node);
  regoSize regoNodeValueSize(regoNode* node);
  regoEnum regoNodeValue(regoNode* node, char* buffer, regoSize size);
  regoSize regoNodeSize(regoNode* node);
  regoNode* regoNodeGet(regoNode* node, regoSize index);
  regoSize regoNodeJSONSize(regoNode* node);
  regoEnum regoNodeJSON(regoNode* node, char* buffer, regoSize size);

#ifdef __cplusplus
}
#endif

#endif
