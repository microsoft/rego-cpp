// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/** @file */

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

// log levels
#define REGO_LOG_LEVEL_NONE 0
#define REGO_LOG_LEVEL_ERROR 1
#define REGO_LOG_LEVEL_OUTPUT 2
#define REGO_LOG_LEVEL_WARN 3
#define REGO_LOG_LEVEL_INFO 4
#define REGO_LOG_LEVEL_DEBUG 5
#define REGO_LOG_LEVEL_TRACE 6

#ifdef __cplusplus
extern "C"
{
#endif
  ////////////////////////////////////////
  // ----- Interpreter functions ------ //
  ////////////////////////////////////////

  /**
   * Sets the level of logging.
   *
   * This setting controls the amount of logging that will be output to stdout.
   * The default level is REGO_LOG_LEVEL_NONE.
   *
   * @param level One of the following values: REGO_LOG_LEVEL_NONE,
   *              REGO_LOG_LEVEL_ERROR, REGO_LOG_LEVEL_OUTPUT, 
   *              REGO_LOG_LEVEL_WARN, REGO_LOG_LEVEL_INFO,
   *              REGO_LOG_LEVEL_DEBUG, REGO_LOG_LEVEL_TRACE.
   */
  void regoSetLogLevel(regoEnum level);

  /**
   * Allocates and initializes a new Rego interpreter.
   *
   * The caller is responsible for freeing the interpreter with regoFree.
   *
   * @return A pointer to the new interpreter.
   */
  regoInterpreter* regoNew();

  /**
   * Frees a Rego interpreter.
   *
   * This pointer must have been allocated with regoNew.
   *
   * @param rego The interpreter to free.
   */
  void regoFree(regoInterpreter* rego);

  /**
   * Adds a module (e.g. virtual document) from the file at the
   * specified path.
   *
   * If an error code is returned, more error information can be
   * obtained by calling regoGetError.
   *
   * @param rego The interpreter.
   * @param path The path to the policy file.
   * @return REGO_OK if successful, REGO_ERROR otherwise.
   */
  regoEnum regoAddModuleFile(regoInterpreter* rego, const char* path);

  /**
   * Adds a module (e.g. virtual document) from the specified string.
   *
   * If an error code is returned, more error information can be
   * obtained by calling regoGetError.
   *
   * @param rego The interpreter.
   * @param name The name of the module.
   * @param contents The contents of the module.
   * @return REGO_OK if successful, REGO_ERROR otherwise.
   */
  regoEnum regoAddModule(
    regoInterpreter* rego, const char* name, const char* contents);

  /**
   * Adds a base document from the file at the specified path.
   *
   * The file should contain a single JSON object. The object will be
   * parsed and merged with the interpreter's base document.
   *
   * If an error code is returned, more error information can be
   * obtained by calling regoGetError.
   *
   * @param rego The interpreter.
   * @param path The path to the JSON file.
   * @return REGO_OK if successful, REGO_ERROR otherwise.
   */
  regoEnum regoAddDataJSONFile(regoInterpreter* rego, const char* path);

  /**
   * Adds a base document from the specified string.
   *
   * The string should contain a single JSON object. The object will be
   * parsed and merged with the interpreter's base document.
   *
   * If an error code is returned, more error information can be
   * obtained by calling regoGetError.
   *
   * @param rego The interpreter.
   * @param contents The contents of the JSON object.
   * @return REGO_OK if successful, REGO_ERROR otherwise.
   */
  regoEnum regoAddDataJSON(regoInterpreter* rego, const char* contents);

  /**
   * Sets the current input document from the file at the specified path.
   *
   * The file should contain a single JSON value. The value will be
   * parsed and set as the interpreter's input document.
   *
   * If an error code is returned, more error information can be
   * obtained by calling regoGetError.
   *
   * @param rego The interpreter.
   * @param path The path to the JSON file.
   * @return REGO_OK if successful, REGO_ERROR otherwise.
   */
  regoEnum regoSetInputJSONFile(regoInterpreter* rego, const char* path);

  /**
   * Sets the current input document from the specified string.
   *
   * The string should contain a single JSON value. The value will be
   * parsed and set as the interpreter's input document.
   *
   * If an error code is returned, more error information can be
   * obtained by calling regoGetError.
   *
   * @param rego The interpreter.
   * @param contents The contents of the JSON value.
   * @return REGO_OK if successful, REGO_ERROR otherwise.
   */
  regoEnum regoSetInputJSON(regoInterpreter* rego, const char* contents);

  /**
   * Sets the debug mode of the interpreter.
   *
   * When debug mode is enabled, the interpreter will output intermediary
   * ASTs after each compiler pass to the debug directory and output pass
   * information to stdout. This is mostly useful for creating reports for
   * compiler issues, but can also be of use in understanding why a policy is
   * invalid or is behaving unexpectedly.
   *
   * @param rego The interpreter.
   * @param enabled Whether debug mode should be enabled.
   */
  void regoSetDebugEnabled(regoInterpreter* rego, regoBoolean enabled);

  /**
   * Gets the debug mode of the interpreter.
   *
   * @param rego The interpreter.
   * @return Whether debug mode is enabled.
   */
  regoBoolean regoGetDebugEnabled(regoInterpreter* rego);

  /**
   * Sets the path to the debug directory.
   *
   * If set, then (when in debug mode) the interpreter will output intermediary
   * ASTs after each compiler pass to the debug directory. If the directory does
   * not exist, it will be created.
   *
   * If an error code is returned, more error information can be
   * obtained by calling regoGetError.
   *
   * @param rego The interpreter.
   * @param path The path to the debug directory.
   * @return REGO_OK if successful, REGO_ERROR otherwise.
   */
  regoEnum regoSetDebugPath(regoInterpreter* rego, const char* path);

  /**
   * Sets whether to perform well-formed checks after each compiler pass.
   *
   * The interpreter has a set of well-formness definitions which indicate the
   * expected form of the AST before and after each compiler pass. This setting
   * determines whether the interpreter will perform these intermediary checks.
   *
   * @param rego The interpreter.
   * @param enabled Whether well-formed checks should be enabled.
   */
  void regoSetWellFormedChecksEnabled(
    regoInterpreter* rego, regoBoolean enabled);

  /**
   * Gets whether well-formed checks are enabled.
   *
   * @param rego The interpreter.
   * @return Whether well-formed checks are enabled.
   */
  regoBoolean regoGetWellFormedChecksEnabled(regoInterpreter* rego);

  /**
   * Performs a query against the current base and virtual documents.
   *
   * The query expression should be a Rego query. The output of the query
   * will be returned as a regoOutput object. The caller is responsible for
   * freeing the output object with regoFreeOutput.
   *
   * @param rego The interpreter.
   * @param query_expr The query expression.
   * @return The output of the query.
   */
  regoOutput* regoQuery(regoInterpreter* rego, const char* query_expr);

  /**
   * Sets whether the built-ins should throw errors.
   *
   * When strict built-in errors are enabled, built-in functions will throw
   * errors when they encounter invalid input. When disabled, built-in
   * functions will return undefined when they encounter invalid input.
   *
   * @param rego The interpreter.
   * @param enabled Whether strict built-in errors should be enabled.
   */
  void regoSetStrictBuiltInErrors(regoInterpreter* rego, regoBoolean enabled);

  /**
   * Gets whether strict built-in errors are enabled.
   *
   * @param rego The interpreter.
   * @return Whether strict built-in errors are enabled.
   */
  regoBoolean regoGetStrictBuiltInErrors(regoInterpreter* rego);

  /**
   * Returns the most recently thrown error.
   *
   * If an error code is returned from an interface function, more error
   * information can be obtained by calling this function.
   *
   * @param rego The interpreter.
   * @return The error message.
   */
  const char* regoGetError(regoInterpreter* rego);

  ////////////////////////////////////////
  // -------- Output functions -------- //
  ////////////////////////////////////////

  /**
   * Returns whether the output is ok.
   *
   * If the output resulted in a valid query result, then this function will
   * return true. Otherwise, it will return false, indicating that the
   * output contains an error sequence.
   *
   * @param output The output.
   * @return Whether the output is ok.
   */
  regoBoolean regoOutputOk(regoOutput* output);

  /**
   * Returns the node containing the output of the query.
   *
   * This will either be a node which contains sequence of terms and/or
   * bindings, or an error sequence.
   *
   * @param output The output.
   * @return The output node.
   */
  regoNode* regoOutputNode(regoOutput* output);

  /**
   * Returns the bound value for a given variable name.
   *
   * If the variable is not bound, then this function will return NULL.
   *
   * @param output The output.
   * @param name The variable name.
   * @return The bound value (or NULL if the variable was not bound)
   */
  regoNode* regoOutputBinding(regoOutput* output, const char* name);

  /**
   * Returns the output represented as a human-readable string.
   *
   * @param output The output.
   * @return The output string.
   */
  const char* regoOutputString(regoOutput* output);

  /**
   * Frees a Rego output.
   *
   * This pointer must have been allocated with regoQuery.
   *
   * @param output The output to free.
   */
  void regoFreeOutput(regoOutput* output);

  ////////////////////////////////////////
  // --------- Node functions --------- //
  ////////////////////////////////////////

  // clang-format off

  /**
   * Returns an enumeration value indicating the nodes type.
   *
   * This type will be one of the following values:
   *
   * 
   * | Name | Description |
   * | ---- | ----------- |
   * | REGO_NODE_BINDING | A binding. Will have two children, a REGO_NODE_VAR and a REGO_NODE_TERM |
   * | REGO_NODE_VAR | A variable name. | 
   * | REGO_NODE_TERM | A term. Will have one child of: REGO_NODE_SCALAR, REGO_NODE_ARRAY, REGO_NODE_SET, REGO_NODE_OBJECT |
   * | REGO_NODE_SCALAR | A scalar value. Will have one child of: REGO_NODE_INT, REGO_NODE_FLOAT, REGO_NODE_STRING, REGO_NODE_TRUE, REGO_NODE_FALSE, REGO_NODE_NULL, REGO_NODE_UNDEFINED |
   * | REGO_NODE_ARRAY | An array. Will have one or more children of: REGO_NODE_TERM |
   * | REGO_NODE_SET | A set. Will have one or more children of: REGO_NODE_TERM |
   * | REGO_NODE_OBJECT | An object. Will have one or more children of: REGO_NODE_OBJECT_ITEM |
   * | REGO_NODE_OBJECT_ITEM | An object item. Will have two children, a REGO_NODE_TERM (the key) and a REGO_NODE_TERM (the value) |
   * | REGO_NODE_INT | An integer value. |
   * | REGO_NODE_FLOAT | A floating point value. |
   * | REGO_NODE_STRING | A string value. |
   * | REGO_NODE_TRUE | A true value. |
   * | REGO_NODE_FALSE | A false value. |
   * | REGO_NODE_NULL | A null value. |
   * | REGO_NODE_UNDEFINED | An undefined value. |
   * | REGO_NODE_ERROR | An error. Will have three children: REGO_NODE_ERROR_MESSAGE, REGO_NODE_ERROR_AST, and REGO_NODE_ERROR_CODE | 
   * | REGO_NODE_ERROR_MESSAGE | An error message. |
   * | REGO_NODE_ERROR_AST | An error AST. |
   * | REGO_NODE_ERROR_CODE | An error code. |
   * | REGO_NODE_ERROR_SEQ | An error sequence. Will have one or more children of: REGO_NODE_ERROR |
   * | REGO_NODE_INTERNAL | An internal node. Use regoNodeTypeName to get the full value. |
   *
   * @return The node type.
   */
  regoEnum regoNodeType(regoNode* node);

  // clang-format on

  /**
   * Returns the name of the node type as a human-readable string.
   *
   * This function supports arbitrary nodes (i.e. it will always produce a
   * value) including internal nodes which appear in error messages.
   *
   * @param node The node.
   * @return The node type name.
   */
  const char* regoNodeTypeName(regoNode* node);

  /**
   * Returns the number of bytes needed to store a 0-terminated string
   * representing the text value of the node.
   *
   * The value returned by this function can be used to allocate a buffer to
   * pass to regoNodeValue.
   *
   * @param node The node.
   * @return The number of bytes needed to store the text value.
   */
  regoSize regoNodeValueSize(regoNode* node);

  /**
   * Populate a buffer with the node value.
   *
   * The buffer must be large enough to hold the value. The size of the buffer
   * can be determined by calling regoNodeValueSize.
   *
   * @param node The node.
   * @param buffer The buffer to populate.
   * @param size The size of the buffer.
   * @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
   */
  regoEnum regoNodeValue(regoNode* node, char* buffer, regoSize size);

  /**
   * Returns the number of children of the node.
   *
   * @param node The node.
   * @return The number of children.
   */
  regoSize regoNodeSize(regoNode* node);

  /**
   * Returns the child node at the specified index.
   *
   * @param node The node.
   * @param index The index of the child.
   * @return The child node.
   */
  regoNode* regoNodeGet(regoNode* node, regoSize index);

  /**
   * Returns the number of bytes needed to store a 0-terminated string
   * representing the JSON representation of the node.
   *
   * The value returned by this function can be used to allocate a buffer to
   * pass to regoNodeJSON.
   *
   * @param node The node.
   * @return The number of bytes needed to store the JSON representation.
   */
  regoSize regoNodeJSONSize(regoNode* node);

  /**
   * Populate a buffer with the JSON representation of the node.
   *
   * The buffer must be large enough to hold the value. The size of the buffer
   * can be determined by calling regoNodeJSONSize.
   *
   * @param node The node.
   * @param buffer The buffer to populate.
   * @param size The size of the buffer.
   * @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
   */
  regoEnum regoNodeJSON(regoNode* node, char* buffer, regoSize size);

#ifdef __cplusplus
}
#endif

#endif
