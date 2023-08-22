#pragma once

#include <string>

namespace rego
{
  const std::string EvalTypeError = "eval_type_error";
  const std::string EvalBuiltInError = "eval_builtin_error";
  const std::string RegoTypeError = "rego_type_error";
  const std::string WellFormedError = "wellformed_error";

  Node err(
    NodeRange& r,
    const std::string& msg,
    const std::string& code = WellFormedError);

  Node err(
    Node node,
    const std::string& msg,
    const std::string& code = WellFormedError);
}