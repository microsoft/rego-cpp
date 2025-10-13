// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/// @file

#ifndef _REGO_C_H_
#define _REGO_C_H_

#include "version.h"

#include <stdint.h>

/// @brief Opaque interpreter type
typedef void regoInterpreter;

/// @brief Opaque node type
typedef void regoNode;

/// @brief Opaque output type
typedef void regoOutput;

/// @brief Opaque bundle type
typedef void regoBundle;

/// @brief Opaque input type
typedef void regoInput;

/// @brief Boolean type
typedef uint_least8_t regoBoolean;

/// @brief Enum type
typedef uint_least32_t regoEnum;

/// @brief Size type
typedef uint_least32_t regoSize;

/// @brief Integer type
typedef int_least64_t regoInt;

/// @brief Node Type type
typedef uint_least32_t regoType;

/// @cond
// error codes
#define REGO_OK 0
#define REGO_ERROR 1
#define REGO_ERROR_BUFFER_TOO_SMALL 2
#define REGO_ERROR_INVALID_LOG_LEVEL 3
// #define REGO_ERROR_MANUAL_TZDATA_NOT_SUPPORTED 4 deprecated
#define REGO_ERROR_INPUT_NULL 5
#define REGO_ERROR_INPUT_MISSING_ARGUMENTS 6
#define REGO_ERROR_INPUT_OBJECT_ITEM 7

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
#define REGO_NODE_TERMS 1015
#define REGO_NODE_BINDINGS 1016
#define REGO_NODE_RESULTS 1017
#define REGO_NODE_RESULT 1018

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
#define REGO_LOG_LEVEL_UNSUPPORTED 7

#define REGO_BUILD_INFO \
  (REGOCPP_VERSION " (" REGOCPP_BUILD_NAME ", " REGOCPP_BUILD_DATE \
                   ") " REGOCPP_BUILD_TOOLCHAIN " on " REGOCPP_PLATFORM)

/// @endcond

#if defined(REGO_SHARED) && defined(WIN32)
/// @brief Macro for exporting functions from a DLL on Windows
#define REGO_API(x) __declspec(dllexport) x __cdecl
#else
/// @brief Macro for exporting functions from a shared library on other
/// platforms
#define REGO_API(x) x
#endif

#ifdef __cplusplus
extern "C"
{
#endif
  ////////////////////////////////////////
  // ----- Interpreter functions ------ //
  ////////////////////////////////////////

  /// @brief Returns the number of bytes needed to store a 0-terminated string
  /// representing the build info.
  /// @details
  /// The value returned by this function can be used to allocate a buffer to
  /// pass to ::regoBuildInfo.
  /// @return The number of bytes needed to store the build info.
  REGO_API(regoSize) regoBuildInfoSize(void);

  /// @brief Populate a buffer with a string of the form "VERSION (BUILD_NAME,
  /// BUILD_DATE) BUILD_TOOLCHAIN on PLATFORM"
  /// @details
  /// The buffer must be large enough to hold the value. The size of the buffer
  /// can be determined by calling ::regoBuildInfoSize.
  /// @param buffer The buffer to populate.
  /// @param size The size of the buffer.
  /// @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
  REGO_API(regoEnum) regoBuildInfo(char* buffer, regoSize size);

  /// @brief Returns the number of bytes needed to store a 0-terminated string
  /// representing version.
  /// @details
  /// The value returned by this function can be used to allocate a buffer to
  /// pass to ::regoVersion.
  /// @return The number of bytes needed to store the version string.
  REGO_API(regoSize) regoVersionSize(void);

  /// @brief Populate a buffer with a string representing the semantic version
  /// of the library.
  /// @details
  /// The buffer must be large enough to hold the value. The size of the buffer
  /// can be determined by calling ::regoVersionSize.
  /// @param buffer The buffer to populate.
  /// @param size The size of the buffer.
  /// @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
  REGO_API(regoEnum) regoVersion(char* buffer, regoSize size);

  /// @brief Gets a log level constant value from a string (case-invariant).
  /// @details String must be one of None, Error, Output, Warn, Info, Debug or
  /// Trace. An unrecognized string will result in a value of
  /// REGO_LOG_LEVEL_UNSUPPORTED.
  /// @param level The log level string.
  /// @return One of the following values: REGO_LOG_LEVEL_NONE,
  ///              REGO_LOG_LEVEL_ERROR, REGO_LOG_LEVEL_OUTPUT,
  ///              REGO_LOG_LEVEL_WARN, REGO_LOG_LEVEL_INFO,
  ///              REGO_LOG_LEVEL_DEBUG, REGO_LOG_LEVEL_TRACE,
  ///              or REGO_LOG_LEVEL_UNSUPPORTED
  REGO_API(regoEnum) regoLogLevelFromString(const char* level);

  /// @brief Sets the level of logging on the interpreter.
  /// @details
  /// This setting controls the amount of logging that will be output to stdout.
  /// The default level is REGO_LOG_LEVEL_OUTPUT, but can be changed using
  /// ::regoSetDefaultLogLevel.
  /// @param rego The interpreter whose log level is to be set.
  /// @param level One of the following values: REGO_LOG_LEVEL_NONE,
  ///              REGO_LOG_LEVEL_ERROR, REGO_LOG_LEVEL_OUTPUT,
  ///              REGO_LOG_LEVEL_WARN, REGO_LOG_LEVEL_INFO,
  ///              REGO_LOG_LEVEL_DEBUG, REGO_LOG_LEVEL_TRACE.
  /// @return REGO_OK if successful, REGO_ERROR_INVALID_LOG_LEVEL otherwise.
  REGO_API(regoEnum) regoSetLogLevel(regoInterpreter* rego, regoEnum level);

  /// @brief Sets the default level of logging.
  /// @details
  /// This setting controls the amount of logging that will be output to stdout.
  /// The default level is REGO_LOG_LEVEL_OUTPUT. New interpreters will be set
  /// to this level of logging unless overridden by ::regoSetLogLevel.
  /// @param level One of the following values: REGO_LOG_LEVEL_NONE,
  ///              REGO_LOG_LEVEL_ERROR, REGO_LOG_LEVEL_OUTPUT,
  ///              REGO_LOG_LEVEL_WARN, REGO_LOG_LEVEL_INFO,
  ///              REGO_LOG_LEVEL_DEBUG, REGO_LOG_LEVEL_TRACE.
  /// @return REGO_OK if successful, REGO_ERROR_INVALID_LOG_LEVEL otherwise.
  REGO_API(regoEnum) regoSetDefaultLogLevel(regoEnum level);

  /// @brief Gets the level of logging on the interpreter.
  /// @return One of the following values: REGO_LOG_LEVEL_NONE,
  ///              REGO_LOG_LEVEL_ERROR, REGO_LOG_LEVEL_OUTPUT,
  ///              REGO_LOG_LEVEL_WARN, REGO_LOG_LEVEL_INFO,
  ///              REGO_LOG_LEVEL_DEBUG, REGO_LOG_LEVEL_TRACE.
  REGO_API(regoEnum) regoGetLogLevel(regoInterpreter* rego);

  /// @brief Allocates and initializes a new Rego interpreter.
  /// @note The caller is responsible for freeing the interpreter with
  /// ::regoFree.
  /// @return A pointer to the new interpreter.
  REGO_API(regoInterpreter*) regoNew(void);

  /// @brief Frees a Rego interpreter.
  /// @note This pointer must have been allocated with ::regoNew.
  /// @param rego The interpreter to free.
  REGO_API(void) regoFree(regoInterpreter* rego);

  /// @brief Adds a module (e.g. virtual document) from the file at the
  /// specified path.
  /// @details
  /// If an error code is returned, more error information can be
  /// obtained by calling ::regoError.
  /// @param rego The interpreter.
  /// @param path The path to the policy file.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum) regoAddModuleFile(regoInterpreter* rego, const char* path);

  /// @brief Adds a module (e.g. virtual document) from the specified string.
  /// @details
  /// If an error code is returned, more error information can be
  /// obtained by calling ::regoError.
  /// @param rego The interpreter.
  /// @param name The name of the module.
  /// @param contents The contents of the module.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum)
  regoAddModule(regoInterpreter* rego, const char* name, const char* contents);

  /// @brief Adds a base document from the file at the specified path.
  /// @details
  /// The file should contain a single JSON object. The object will be
  /// parsed and merged with the interpreter's base document.
  /// @par If an error code is returned, more error information can be
  /// obtained by calling ::regoError.
  /// @param rego The interpreter.
  /// @param path The path to the JSON file.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum)
  regoAddDataJSONFile(regoInterpreter* rego, const char* path);

  /// @brief Adds a base document from the specified string.
  /// @details
  /// The string should contain a single JSON object. The object will be
  /// parsed and merged with the interpreter's base document.
  /// @par If an error code is returned, more error information can be
  /// obtained by calling ::regoError.
  /// @param rego The interpreter.
  /// @param contents The contents of the JSON object.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum)
  regoAddDataJSON(regoInterpreter* rego, const char* contents);

  /// @brief Sets the current input document from the file at the specified
  /// path.
  /// @details
  /// The file should contain a single JSON value. The value will be
  /// parsed and set as the interpreter's input document.
  /// @par If an error code is returned, more error information can be
  /// obtained by calling ::regoError.
  /// @param rego The interpreter.
  /// @param path The path to the JSON file.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum)
  regoSetInputJSONFile(regoInterpreter* rego, const char* path);

  /// @brief Sets the current input document from the specified string.
  /// @details
  /// The string should contain a single Rego data term. The value will be
  /// parsed and set as the interpreter's input document.
  /// @par If an error code is returned, more error information can be
  /// obtained by calling ::regoError.
  /// @param rego The interpreter.
  /// @param contents The contents of the Rego data term.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum)
  regoSetInputTerm(regoInterpreter* rego, const char* contents);

  /// @brief Sets the current input document from the specified Input object.
  /// @details
  /// If an error code is returned, more error information can be
  /// obtained by calling ::regoError.
  /// @param rego The interpreter.
  /// @param input The input object.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum)
  regoSetInput(regoInterpreter* rego, regoInput* input);

  /// @brief Sets the debug mode of the interpreter.
  /// @details
  /// When debug mode is enabled, the interpreter will output intermediary
  /// ASTs after each compiler pass to the debug directory and output pass
  /// information to stdout. This is mostly useful for creating reports for
  /// compiler issues, but can also be of use in understanding why a policy is
  /// invalid or is behaving unexpectedly.
  /// @param rego The interpreter.
  /// @param enabled Whether debug mode should be enabled.
  REGO_API(void)
  regoSetDebugEnabled(regoInterpreter* rego, regoBoolean enabled);

  /// @brief Gets the debug mode of the interpreter.
  /// @param rego The interpreter.
  // /@return Whether debug mode is enabled.
  REGO_API(regoBoolean) regoGetDebugEnabled(regoInterpreter* rego);

  /// @brief Sets the path to the debug directory.
  /// @details
  /// If set, then (when in debug mode) the interpreter will output intermediary
  /// ASTs after each compiler pass to the debug directory. If the directory
  /// does not exist, it will be created.
  /// @par If an error code is returned, more error information can be
  /// obtained by calling ::regoError.
  /// @param rego The interpreter.
  /// @param path The path to the debug directory.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum) regoSetDebugPath(regoInterpreter* rego, const char* path);

  /// @brief Sets whether to perform well-formedness checks after each compiler
  /// pass.
  /// @details
  /// The interpreter has a set of well-formedness definitions which indicate
  /// the expected form of the AST before and after each compiler pass. This
  /// setting determines whether the interpreter will perform these intermediary
  /// checks.
  /// @param rego The interpreter.
  /// @param enabled Whether well-formedness checks should be enabled.
  REGO_API(void)
  regoSetWellFormedChecksEnabled(regoInterpreter* rego, regoBoolean enabled);

  /// @brief Gets whether well-formedness checks are enabled.
  /// @param rego The interpreter.
  /// @return Whether well-formedness checks are enabled.
  REGO_API(regoBoolean) regoGetWellFormedChecksEnabled(regoInterpreter* rego);

  /// @brief Performs a query against the current base and virtual documents.
  /// @details
  /// The query expression should be a Rego query. The output of the query
  /// will be returned as a regoOutput object.
  /// @note The caller is responsible for freeing the output object with
  /// ::regoFreeOutput.
  /// @param rego The interpreter.
  /// @param query_expr The query expression.
  /// @return The output of the query.
  REGO_API(regoOutput*)
  regoQuery(regoInterpreter* rego, const char* query_expr);

  /// @brief Sets whether the built-ins should throw errors.
  /// @details
  /// When strict built-in errors are enabled, built-in functions will throw
  /// errors when they encounter invalid input. When disabled, built-in
  /// functions will return undefined when they encounter invalid input.
  /// @param rego The interpreter.
  /// @param enabled Whether strict built-in errors should be enabled.
  REGO_API(void)
  regoSetStrictBuiltInErrors(regoInterpreter* rego, regoBoolean enabled);

  /// @brief Gets whether strict built-in errors are enabled.
  /// @param rego The interpreter.
  /// @return Whether strict built-in errors are enabled.
  REGO_API(regoBoolean) regoGetStrictBuiltInErrors(regoInterpreter* rego);

  /// @brief Returns whether the specified name corresponds to an available
  /// built-in in the interpreter.
  /// @param rego The interpreter.
  /// @param name The name of the built-in.
  /// @return Whether the built-in exists.
  REGO_API(regoBoolean)
  regoIsAvailableBuiltIn(regoInterpreter* rego, const char* name);

  /// @brief Returns the number of bytes needed to store a 0-terminated string
  /// representing the most recent error message.
  /// @details
  /// The value returned by this function can be used to allocate a buffer to
  /// pass to ::regoError.
  /// @return The number of bytes needed to store the error message.
  REGO_API(regoSize) regoErrorSize(regoInterpreter* rego);

  /// @brief Populate a buffer with the most recent error message.
  /// @details
  /// If an error code is returned from an interface function, more error
  /// information can usually be obtained by calling this function.
  /// @par The buffer must be large enough to hold the value. The size of the
  /// buffer can be determined by calling ::regoErrorSize.
  /// @param rego The interpreter.
  /// @param buffer The buffer to populate.
  /// @param size The size of the buffer.
  /// @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
  REGO_API(regoEnum)
  regoError(regoInterpreter* rego, char* buffer, regoSize size);

  /// @brief Sets the query for the interpreter.
  /// @details
  /// By using this method, a query can be set such that it is included as a
  /// default entrypoint in a bundle.
  /// @param rego The interpreter
  /// @param query_expr A Rego query expression
  /// @return REGO_OK if successful, REGO_ERROR if there was a problem parsing
  /// the query
  REGO_API(regoEnum)
  regoSetQuery(regoInterpreter* rego, const char* query_expr);

  /// @brief Adds an entrypoint to the interpreter.
  /// @details
  /// By using this method, an entrypoint can be added to the list of those with
  /// compiled execution plans in a bundle.
  /// @param rego The interpreter
  /// @param entrypoint A valid Rego entrypoint of the form `path/to/rule`
  /// @return REGO_OK if successful, REGO_ERROR if there were any issues adding
  /// the entrypoint.
  REGO_API(regoEnum)
  regoAddEntrypoint(regoInterpreter* rego, const char* entrypoint);

  /// @brief Builds a compiled Rego bundle.
  /// @details
  /// The bundle will include any base and virtual documents added to the
  /// interpreter, as well as a query set with ::regoSetQuery (if present) and
  /// any entrypoints added with ::regoAddEntrypoint (if any). There must be at
  /// least one entrypoint or a query specified for the build to be successful.
  /// The resulting handle can be used with ::regoBundleQuery and/or saved to
  /// the disk for later re-use.
  /// @return a compiled Rego bundle
  REGO_API(regoBundle*) regoBuild(regoInterpreter* rego);

  /// @brief Loads a compiled Rego bundle from the specified directory.
  /// @details
  /// The directory must contain a valid Rego bundle, which consists of a
  /// `plan.json` file, a `data.json` file, and zero or more `.rego` files
  /// which were used to create the bundle.
  /// @param rego The interpreter
  /// @param dir The path to the directory containing the bundle.
  /// @return The loaded bundle, or NULL if there was an error.
  REGO_API(regoBundle*) regoBundleLoad(regoInterpreter* rego, const char* dir);

  /// @brief Loads a compiled Rego bundle from the specified binary file.
  /// @details
  /// The file must contain a valid Rego bundle in Rego Bundle Binary format.
  /// For more information on the format, see [the
  /// specification](../../binary.md).
  /// @param rego The interpreter
  /// @param path The path to the binary file containing the bundle.
  /// @return The loaded bundle, or NULL if there was an error.
  REGO_API(regoBundle*)
  regoBundleLoadBinary(regoInterpreter* rego, const char* path);

  /// @brief Saves a compiled Rego bundle to the specified directory.
  /// @details
  /// The directory will be created if it does not exist.
  /// The bundle will be saved in the standard Rego bundle format, which
  /// consists of a `plan.json` file, a `data.json` file, and
  /// zero or more `.rego` files.
  /// @param rego The interpreter
  /// @param dir The path to the directory where the bundle will be saved.
  /// @param bundle The bundle to save.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum)
  regoBundleSave(regoInterpreter* rego, const char* dir, regoBundle* bundle);

  /// @brief Saves a compiled Rego bundle to the specified binary file.
  /// @details
  /// The bundle will be saved in Rego Bundle Binary format.
  /// For more information on the format, see [the
  /// specification](../../binary.md).
  /// @param rego The interpreter
  /// @param path The path to the binary file where the bundle will be saved.
  /// @param bundle The bundle to save.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum)
  regoBundleSaveBinary(
    regoInterpreter* rego, const char* path, regoBundle* bundle);

  /// @brief Performs a query against the specified bundle.
  /// @details
  /// The output of the query will be returned as a regoOutput object.
  /// The query executed will be the one set with ::regoSetQuery when the bundle
  /// was built. If no query was set, the output will be an error sequence.
  /// @note The caller is responsible for freeing the output object with
  /// ::regoFreeOutput.
  /// @param rego The interpreter.
  /// @param bundle The bundle to query.
  /// @return The output of the query.
  REGO_API(regoOutput*)
  regoBundleQuery(regoInterpreter* rego, regoBundle* bundle);

  /// @brief Performs a query against the specified bundle at the specified
  /// entrypoint.
  /// @details
  /// The output of the query will be returned as a regoOutput object.
  /// The entrypoint must be one of those added with ::regoAddEntrypoint when
  /// the bundle was built. If the entrypoint does not exist, the output will be
  /// an error sequence.
  /// @note The caller is responsible for freeing the output object with
  /// ::regoFreeOutput.
  /// @param rego The interpreter.
  /// @param bundle The bundle to query.
  /// @param endpoint The entrypoint to query.
  /// @return The output of the query.
  REGO_API(regoOutput*)
  regoBundleQueryEntrypoint(
    regoInterpreter* rego, regoBundle* bundle, const char* endpoint);

  ////////////////////////////////////////
  // -------- Output functions -------- //
  ////////////////////////////////////////

  /// @brief Returns whether the output is ok.
  /// @details
  /// If the output resulted in a valid query result, then this function will
  /// return true. Otherwise, it will return false, indicating that the
  /// output contains an error sequence.
  /// @param output The output.
  /// @return Whether the output is ok.
  REGO_API(regoBoolean) regoOutputOk(regoOutput* output);

  /// @brief Returns the number of results in the output.
  /// @details
  /// Each query can potentially generate multiple results.
  /// @return The number of results in the output
  REGO_API(regoSize) regoOutputSize(regoOutput* output);

  /// @brief Returns a node containing a list of terms resulting from the query
  /// at the specified index.
  /// @param output The output.
  /// @param index The result index.
  /// @return The output node.
  REGO_API(regoNode*)
  regoOutputExpressionsAtIndex(regoOutput* output, regoSize index);

  /// @brief Returns a node containing a list of terms resulting from the query
  /// at the default index.
  /// @param output The output.
  /// @return The output node.
  REGO_API(regoNode*) regoOutputExpressions(regoOutput* output);

  /// @brief Returns the node containing the output of the query.
  /// @details
  /// This will either be a node which contains one or more results,
  /// or an error sequence.
  /// @param output The output.
  /// @return The output node.
  REGO_API(regoNode*) regoOutputNode(regoOutput* output);

  /// @brief Returns the bound value for a given variable name.
  /// @details
  /// If the variable is not bound, then this function will return NULL.
  /// @param output The output.
  /// @param index The result index.
  /// @param name The variable name.
  /// @return The bound value (or NULL if the variable was not bound)
  REGO_API(regoNode*)
  regoOutputBindingAtIndex(
    regoOutput* output, regoSize index, const char* name);

  /// @brief Returns the bound value for a given variable name at the first
  /// index.
  /// @details
  /// If the variable is not bound, then this function will return NULL.
  /// @param output The output.
  /// @param name The variable name.
  /// @return The bound value (or NULL if the variable was not bound)
  REGO_API(regoNode*) regoOutputBinding(regoOutput* output, const char* name);

  /// @brief Returns the number of bytes needed to store a 0-terminated string
  /// representing the output as a human-readable string.
  /// @details
  /// The value returned by this function can be used to allocate a buffer to
  /// pass to ::regoOutputJSON.
  /// @param output The output.
  /// @return The number of bytes needed to store the output string.
  REGO_API(regoSize) regoOutputJSONSize(regoOutput* output);

  /// @brief Populate a buffer with the output represented as a human-readable
  /// string.
  /// @details
  /// The buffer must be large enough to hold the value. The size of the buffer
  /// can be determined by calling ::regoOutputJSONSize.
  /// @param output The output.
  /// @param buffer The buffer to populate.
  /// @param size The size of the buffer.
  /// @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
  REGO_API(regoEnum)
  regoOutputJSON(regoOutput* output, char* buffer, regoSize size);

  /// @brief Frees a Rego output.
  /// @note This pointer must have been allocated with regoQuery.
  /// @param output The output to free.
  REGO_API(void) regoFreeOutput(regoOutput* output);

  ////////////////////////////////////////
  // --------- Node functions --------- //
  ////////////////////////////////////////

  // clang-format off

  /// @brief Returns an enumeration value indicating the node's type.
  /// @details
  /// This type will be one of the following values:
  ///
  ///
  /// | Name | Description |
  /// | ---- | ----------- |
  /// | REGO_NODE_BINDING | A binding. Will have two children, a REGO_NODE_VAR and a REGO_NODE_TERM |
  /// | REGO_NODE_VAR | A variable name. | 
  /// | REGO_NODE_TERM | A term. Will have one child of: REGO_NODE_SCALAR, REGO_NODE_ARRAY, REGO_NODE_SET, REGO_NODE_OBJECT |
  /// | REGO_NODE_SCALAR | A scalar value. Will have one child of: REGO_NODE_INT, REGO_NODE_FLOAT, REGO_NODE_STRING, REGO_NODE_TRUE, REGO_NODE_FALSE, REGO_NODE_NULL, REGO_NODE_UNDEFINED |
  /// | REGO_NODE_ARRAY | An array. Will have zero or more children of: REGO_NODE_TERM |
  /// | REGO_NODE_SET | A set. Will have zero or more children of: REGO_NODE_TERM |
  /// | REGO_NODE_OBJECT | An object. Will have zero or more children of: REGO_NODE_OBJECT_ITEM |
  /// | REGO_NODE_OBJECT_ITEM | An object item. Will have two children, a REGO_NODE_TERM (the key) and a REGO_NODE_TERM (the value) |
  /// | REGO_NODE_INT | An integer value. |
  /// | REGO_NODE_FLOAT | A floating point value. |
  /// | REGO_NODE_STRING | A string value. |
  /// | REGO_NODE_TRUE | A true value. |
  /// | REGO_NODE_FALSE | A false value. |
  /// | REGO_NODE_NULL | A null value. |
  /// | REGO_NODE_UNDEFINED | An undefined value. |
  /// | REGO_NODE_TERMS | Will have zero or more children of: REGO_NODE_TERM
  /// | REGO_NODE_BINDINGS | Will have zero or more children of: REGO_NODE_BINDING
  /// | REGO_NODE_RESULTS | Will have one or more children of: REGO_NODE_RESULT
  /// | REGO_NODE_RESULT | Will have two children, a REGO_NODE_BINDINGS and a REGO_NODE_TERMS.
  /// | REGO_NODE_ERROR | An error. Will have three children: REGO_NODE_ERROR_MESSAGE, REGO_NODE_ERROR_AST, and REGO_NODE_ERROR_CODE | 
  /// | REGO_NODE_ERROR_MESSAGE | An error message. |
  /// | REGO_NODE_ERROR_AST | An error AST. |
  /// | REGO_NODE_ERROR_CODE | An error code. |
  /// | REGO_NODE_ERROR_SEQ | An error sequence. Will have one or more children of: REGO_NODE_ERROR |
  /// | REGO_NODE_INTERNAL | An internal node. Use regoNodeTypeName to get the full value. |
  ///
  /// @param node the node to inspect
  /// @return The node type.
  REGO_API(regoType) regoNodeType(regoNode* node);

  // clang-format on

  /// @brief Returns the number of bytes needed to store a 0-terminated string
  /// representing the name of the node type.
  /// @details
  /// The value returned by this function can be used to allocate a buffer to
  /// pass to ::regoNodeTypeName.
  /// @param node The node.
  /// @return The number of bytes needed to store the node type name.
  REGO_API(regoEnum) regoNodeTypeNameSize(regoNode* node);

  /// @brief Returns the name of the node type as a human-readable string.
  /// @details
  /// This function supports arbitrary nodes (i.e. it will always produce a
  /// value) including internal nodes which appear in error messages.
  /// The buffer must be large enough to hold the value. The size of the buffer
  /// can be determined by calling ::regoNodeTypeNameSize.
  /// @param node The node.
  /// @param buffer The buffer to populate.
  /// @param size The size of the buffer.
  /// @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
  REGO_API(regoEnum)
  regoNodeTypeName(regoNode* node, char* buffer, regoSize size);

  /// @brief Returns the number of bytes needed to store a 0-terminated string
  /// representing the text value of the node.
  /// @details
  /// The value returned by this function can be used to allocate a buffer to
  /// pass to ::regoNodeValue.
  /// @param node The node.
  /// @return The number of bytes needed to store the text value.
  REGO_API(regoSize) regoNodeValueSize(regoNode* node);

  /// @brief Populate a buffer with the node value.
  /// @details
  /// The buffer must be large enough to hold the value. The size of the buffer
  /// can be determined by calling regoNodeValueSize.
  /// @param node The node.
  /// @param buffer The buffer to populate.
  /// @param size The size of the buffer.
  /// @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
  REGO_API(regoEnum) regoNodeValue(regoNode* node, char* buffer, regoSize size);

  /// @brief Returns the number of children of the node.
  /// @param node The node.
  /// @return The number of children.
  REGO_API(regoSize) regoNodeSize(regoNode* node);

  /// @brief Returns the child node at the specified index.
  /// @param node The node.
  /// @param index The index of the child.
  /// @return The child node.
  REGO_API(regoNode*) regoNodeGet(regoNode* node, regoSize index);

  /// @brief Returns the number of bytes needed to store a 0-terminated string
  /// representing the JSON representation of the node.
  /// @details
  /// The value returned by this function can be used to allocate a buffer to
  /// pass to ::regoNodeJSON.
  /// @param node The node.
  /// @return The number of bytes needed to store the JSON representation.
  REGO_API(regoSize) regoNodeJSONSize(regoNode* node);

  /// @brief Populate a buffer with the JSON representation of the node.
  /// @details
  /// The buffer must be large enough to hold the value. The size of the buffer
  /// can be determined by calling ::regoNodeJSONSize.
  /// @param node The node.
  /// @param buffer The buffer to populate.
  /// @param size The size of the buffer.
  /// @return REGO_OK if successful, REGO_ERROR_BUFFER_TOO_SMALL otherwise.
  REGO_API(regoEnum) regoNodeJSON(regoNode* node, char* buffer, regoSize size);

  /////////////////////////////////////////
  // --------- Input functions --------- //
  /////////////////////////////////////////

  // clang-format off

  /// @brief Allocates and initializes a new Rego Input object.
  /// @details
  /// Rego Input objects can be used to build input documents programmatically.
  /// An input is built up by making a series of calls to the API to add
  /// values to the stack. Some operations use the values on the stack to
  /// create composite values (e.g. objects, arrays, sets) which are then
  /// pushed back onto the stack. Once the input is built, it can be validated
  /// and then set on the interpreter with regoSetInput. While you can test the
  /// result of every operation, you can continue to use a broken Input object
  /// and the error will be returned when you call regoInputValidate.
  ///
  /// ```c
  /// // Example of building the following input document:
  /// // {
  /// //   "user": {
  /// //     "name": "alice",
  /// //     "age": 30,
  /// //     "active": true
  /// //   },
  /// //   "roles": ["admin", "user"]
  /// // }
  /// regoInput* input = regoNewInput();
  /// regoInputString(input, "user");     // Push "user"
  /// regoInputString(input, "name");     // Push "name"
  /// regoInputString(input, "alice");    // Push "alice"
  /// regoInputObjectItem(input);         // Create {"name": "alice"}
  /// regoInputString(input, "age");      // Push "age"
  /// regoInputInt(input, 30);            // Push 30
  /// regoInputObjectItem(input);         // Create {"age": 30}
  /// regoInputString(input, "active");   // Push "active"
  /// regoInputBoolean(input, true);      // Push true
  /// regoInputObjectItem(input);         // Create {"active": true}
  /// regoInputObject(input, 3);          // Create {"name": "alice", "age": 30, "active": true}
  /// regoInputObjectItem(input);         // Create {"user": {...}}
  /// regoInputString(input, "roles");    // Push "roles"
  /// regoInputString(input, "admin");    // Push "admin"
  /// regoInputString(input, "user");     // Push "user"
  /// regoInputArray(input, 2);           // Create ["admin", "user"]
  /// regoInputObjectItem(input);         // Create {"roles": [...]}
  /// regoInputObject(input, 2);          // Create the final object
  /// regoInputValidate(input);           // Validate the input
  /// regoSetInput(rego, input);          // Set the input on the interpreter
  /// ```
  /// @note The caller is responsible for freeing the input object with
  /// regoFreeInput.
  /// @return A pointer to the new input object.
  REGO_API(regoInput*) regoNewInput();

  // clang-format on

  /// @brief Adds an integer value to the input object.
  /// @param input The input object.
  /// @param value The integer value.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum) regoInputInt(regoInput* input, regoInt value);

  /// @brief Adds a floating point value to the input object.
  /// @param input The input object.
  /// @param value The floating point value.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum) regoInputFloat(regoInput* input, double value);

  /// @brief Adds a string value to the input object.
  /// @param input The input object.
  /// @param value The string value.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum) regoInputString(regoInput* input, const char* value);

  /// @brief Adds a boolean value to the input object.
  /// @param input The input object.
  /// @param value The boolean value.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum) regoInputBoolean(regoInput* input, regoBoolean value);

  /// @brief Adds a null value to the input object.
  /// @param input The input object.
  /// @return REGO_OK if successful, REGO_ERROR otherwise.
  REGO_API(regoEnum) regoInputNull(regoInput* input);

  /// @brief Uses the top two values on the stack to create an object item
  /// and pushes it back onto the stack.
  /// @details
  /// The top value on the stack will be used as the value, and the second value
  /// will be used as the key.
  /// @param input The input object.
  /// @return REGO_OK if successful, REGO_ERROR or REGO_INPUT_MISSING_ARGUMENTS
  /// otherwise.
  REGO_API(regoEnum) regoInputObjectItem(regoInput* input);

  /// @brief Uses the top 'count' values on the stack to create an object
  /// and pushes it back onto the stack.
  /// @note The top 'count' values must be object items.
  /// @param input The input object.
  /// @param count The number of object items to use.
  /// @return REGO_OK if successful, REGO_ERROR, REGO_INPUT_MISSING_ARGUMENTS,
  /// or REGO_INPUT_OBJECT_ITEM otherwise.
  REGO_API(regoEnum) regoInputObject(regoInput* input, regoSize count);

  /// @brief Uses the top 'count' values on the stack to create an array and
  /// pushes it back onto the stack.
  /// @param input The input object.
  /// @param count The number of values to use.
  /// @return REGO_OK if successful, REGO_ERROR or REGO_INPUT_MISSING_ARGUMENTS
  /// otherwise.
  REGO_API(regoEnum) regoInputArray(regoInput* input, regoSize count);

  /// @brief Uses the top 'count' values on the stack to create a set and pushes
  /// it back onto the stack.
  /// @note As part of this operation, duplicate values will be removed and the
  /// values will be sorted.
  /// @param input The input object.
  /// @param count The number of values to use.
  /// @return REGO_OK if successful, REGO_ERROR or REGO_INPUT_MISSING_ARGUMENTS
  /// otherwise.
  REGO_API(regoEnum) regoInputSet(regoInput* input, regoSize count);

  /// @brief Validates the input object.
  /// @details
  /// This function should be called after building the input object
  /// and before using it with ::regoSetInput. This function will ensure
  /// that the input object is valid (e.g. that there is exactly one
  /// value on the stack and there were no errors during construction).
  /// @param input The input object.
  /// @return REGO_OK if the input is valid, REGO_ERROR_INPUT_* otherwise.
  REGO_API(regoEnum) regoInputValidate(regoInput* input);

  /// @brief Returns the top of the Input stack.
  /// @param input The input object.
  /// @return The top of the input stack as a regoNode. If the input is invalid
  /// or empty, this function will return NULL.
  REGO_API(regoNode*) regoInputNode(regoInput* input);

  /// @brief Returns the number of items on the Input stack.
  /// @param input The input object.
  /// @return The number of items on the input stack.
  REGO_API(regoSize) regoInputSize(regoInput* input);

  /// @brief Frees a Rego Input object.
  /// @note This pointer must have been allocated with ::regoNewInput.
  /// @param input The input object to free.
  REGO_API(void) regoFreeInput(regoInput* input);

  //////////////////////////////////////////
  // --------- Bundle functions --------- //
  //////////////////////////////////////////

  /// @brief Returns whether the bundle is ok.
  /// @details
  /// If the bundle was built or loaded successfully, this function will
  /// return true. Otherwise, it will return false, indicating that there
  /// was an error during bundle creation. This error can be retrieved using
  /// regoError on the interpreter used to create or load the bundle.
  /// @param bundle The bundle.
  /// @return Whether the bundle is ok.
  REGO_API(regoBoolean) regoBundleOk(regoBundle* bundle);

  /// @brief Returns a node containing the base document of the bundle.
  /// @warning If the node was loaded from a binary file, this node will be
  /// missing. Future versions of the API may provide a way to
  /// reconstruct the bundle AST from the binary file.
  /// @param bundle The bundle.
  /// @return A node containing the base document of the bundle.
  REGO_API(regoNode*) regoBundleNode(regoBundle* bundle);

  /// @brief Frees a Rego bundle.
  /// @note This pointer must have been allocated with ::regoBuild,
  /// ::regoBundleLoad, or ::regoBundleLoadBinary.
  /// @param bundle The bundle to free.
  REGO_API(void) regoFreeBundle(regoBundle* bundle);

#ifdef __cplusplus
}
#endif

#endif
