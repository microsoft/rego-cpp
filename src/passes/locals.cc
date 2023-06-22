#include "passes.h"

#include <map>

namespace
{
  using namespace rego;

  void find_free_vars(const Node& node, std::map<std::string, Node>& locals)
  {
    if (node->type() == Var && node->parent()->type() != RefArgDot)
    {
      std::string name = std::string(node->location().view());
      if (name == "data")
      {
        // reserved
        return;
      }

      if (locals.count(name) > 0)
      {
        return;
      }

      Nodes defs = node->lookup();
      if (defs.size() > 0)
      {
        return;
      }

      locals.insert({name, Local << node->clone() << Undefined});
    }
    else
    {
      for (auto& child : *node)
      {
        find_free_vars(child, locals);
      }
    }
  }
}

namespace rego
{
  PassDef locals()
  {
    PassDef locals = {dir::topdown | dir::once};

    locals.pre(UnifyBody, [](Node node) {
      std::map<std::string, Node> locals;
      for (const auto& child : *node)
      {
        if (child->type() == Literal)
        {
          find_free_vars(child, locals);
        }
      }

      for (const auto& [name, local] : locals)
      {
        node->push_front(local);
      }

      return 0;
    });

    return locals;
  }
}