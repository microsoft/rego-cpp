#include "builtins.h"
#include "lang.h"
#include "passes.h"

#include <map>

namespace
{
  using namespace rego;
  using Scope = std::map<std::string, Node>;

  void add_locals(
    Node unifybody, std::vector<Scope>& scopes, const BuiltIns& builtins);

  void find_free_vars(
    const Node& node, std::vector<Scope>& scopes, const BuiltIns& builtins)
  {
    std::set<Token> exclude_parents = {
      RefArgDot, VarSeq, ArrayCompr, SetCompr, ObjectCompr};
    if (exclude_parents.contains(node->type()))
    {
      return;
    }

    if (node->type() == Local)
    {
      Node var = node / Var;
      std::string name = std::string(var->location().view());
      for (auto& scope : scopes)
      {
        if (scope.contains(name))
        {
          scope[name] = Undefined;
        }
      }
      return;
    }

    if (node->type() == Var)
    {
      if (builtins.is_builtin(node->location()))
      {
        return;
      }

      std::string name = std::string(node->location().view());
      if (name == "data")
      {
        // reserved
        return;
      }

      for (auto& scope : scopes)
      {
        if (scope.contains(name))
        {
          return;
        }
      }

      Nodes defs = node->lookup();
      if (defs.size() > 0)
      {
        return;
      }

      scopes.back().insert({name, Local << node->clone() << Undefined});
    }
    else if (node->type() == UnifyBody)
    {
      add_locals(node, scopes, builtins);
    }
    else
    {
      for (auto& child : *node)
      {
        find_free_vars(child, scopes, builtins);
      }
    }
  }

  void add_locals(
    Node unifybody, std::vector<Scope>& scopes, const BuiltIns& builtins)
  {
    if (unifybody->type() == NestedBody)
    {
      unifybody = unifybody / Val;
    }

    if (unifybody->type() != UnifyBody)
    {
      return;
    }

    scopes.push_back({});
    for (const auto& child : *unifybody)
    {
      find_free_vars(child, scopes, builtins);
    }

    for (const auto& [name, local] : scopes.back())
    {
      if (local->type() == Local)
      {
        unifybody->push_front(local);
      }
    }

    scopes.pop_back();
  }

  int preprocess_body(Node node)
  {
    std::vector<Scope> scopes;
    add_locals(node / Body, scopes, BuiltIns().register_standard_builtins());
    return 0;
  }

  int preprocess_value(Node node)
  {
    std::vector<Scope> scopes;
    add_locals(node / Val, scopes, BuiltIns().register_standard_builtins());
    return 0;
  }
}

namespace rego
{
  // Discovers undeclared local variables from rule bodies and the query and
  // inserts Local nodes for them at the appropriate scope.
  PassDef body_locals()
  {
    PassDef locals = {dir::topdown | dir::once};

    locals.pre(RuleComp, preprocess_body);
    locals.pre(RuleFunc, preprocess_body);
    locals.pre(RuleObj, preprocess_body);
    locals.pre(RuleSet, preprocess_body);
    locals.pre(ArrayCompr, preprocess_body);
    locals.pre(SetCompr, preprocess_body);
    locals.pre(ObjectCompr, preprocess_body);

    return locals;
  }

  // Discovers undeclared local variables from rule values and inserts Local
  // nodes for them at the appropriate scope.
  PassDef value_locals()
  {
    PassDef locals = {dir::topdown | dir::once};

    locals.pre(RuleComp, preprocess_value);
    locals.pre(RuleFunc, preprocess_value);
    locals.pre(RuleObj, preprocess_value);
    locals.pre(RuleSet, preprocess_value);

    return locals;
  }
}