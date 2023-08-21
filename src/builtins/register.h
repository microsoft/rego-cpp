#pragma once

#include "builtins.h"

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> aggregates();
    std::vector<BuiltIn> arrays();
    std::vector<BuiltIn> encoding();
    std::vector<BuiltIn> numbers();
    std::vector<BuiltIn> semver();
    std::vector<BuiltIn> sets();
    std::vector<BuiltIn> strings();
  }
}