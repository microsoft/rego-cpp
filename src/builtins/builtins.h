#pragma once

#include "internal.hh"

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> aggregates();
    std::vector<BuiltIn> arrays();
    std::vector<BuiltIn> bits();
    std::vector<BuiltIn> casts();
    std::vector<BuiltIn> encoding();
    std::vector<BuiltIn> graph();
    std::vector<BuiltIn> numbers();
    std::vector<BuiltIn> objects();
    std::vector<BuiltIn> regex();
    std::vector<BuiltIn> semver();
    std::vector<BuiltIn> sets();
    std::vector<BuiltIn> strings();
    std::vector<BuiltIn> time();
    std::vector<BuiltIn> types();
    std::vector<BuiltIn> units();
    std::vector<BuiltIn> uuid();
  }
}