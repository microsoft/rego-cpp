// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>
#include <trieste/ast.h>

namespace rego
{
  const std::string EvalTypeError = "eval_type_error";
  const std::string EvalBuiltInError = "eval_builtin_error";
  const std::string RegoTypeError = "rego_type_error";
  const std::string EvalConflictError = "eval_conflict_error";
  const std::string WellFormedError = "wellformed_error";
  const std::string RuntimeError = "runtime_error";

  Node err(
    NodeRange& r,
    const std::string& msg,
    const std::string& code = WellFormedError);

  Node err(
    Node node,
    const std::string& msg,
    const std::string& code = WellFormedError);
}