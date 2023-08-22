#pragma once

#include <functional>
#include <string>
#include <trieste/source.h>
#include <trieste/ast.h>

namespace rego
{
  using namespace trieste;
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
    BuiltIns() noexcept;
    bool is_builtin(const Location& name) const;
    Node call(const Location& name, const Nodes& args) const;
    BuiltIns& register_builtin(const BuiltIn& built_in);
    const BuiltIn& at(const Location& name) const;
    bool strict_errors() const;
    BuiltIns& strict_errors(bool strict_errors);

    template <typename T>
    BuiltIns& register_builtins(const T& built_ins)
    {
      for (auto& built_in : built_ins)
      {
        register_builtin(built_in);
      }

      return *this;
    }

    BuiltIns& register_standard_builtins();
    std::map<Location, BuiltIn>::const_iterator begin() const;
    std::map<Location, BuiltIn>::const_iterator end() const;

  private:
    std::map<Location, BuiltIn> m_builtins;
    bool m_strict_errors;
  };
}