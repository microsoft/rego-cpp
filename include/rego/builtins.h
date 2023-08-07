#pragma once

#include "value.h"

#include <functional>
#include <string>

namespace rego
{
  struct BuiltInDef;
  using BuiltIn = std::shared_ptr<BuiltInDef>;
  using BuiltInBehavior = Node (*)(const Nodes&);

  const std::size_t AnyArity = std::numeric_limits<std::size_t>::max();
  struct BuiltInDef
  {
    Location name;
    std::size_t arity;
    Node (*behavior)(const Nodes&);
    static BuiltIn create(
      const Location& name, std::size_t arity, BuiltInBehavior behavior);
  };

  class BuiltIns
  {
  public:
    bool is_builtin(const Location& name) const;
    Node call(const Location& name, const Nodes& args) const;
    BuiltIns& register_builtin(const BuiltIn& built_in);
    BuiltIns& register_standard_builtins();
    std::map<Location, BuiltIn>::const_iterator begin() const;
    std::map<Location, BuiltIn>::const_iterator end() const;

  private:
    std::map<Location, BuiltIn> s_builtins;
  };
}