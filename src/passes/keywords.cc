#include "passes.h"

namespace
{
  using namespace trieste;

  bool maybe_keyword(const Node& node)
  {
    std::string name = std::string(node->location().view());
    return rego::Keywords.contains(name);
  }
}

namespace rego
{
  PassDef keywords()
  {
    return {
      In(Group) * T(Var)[Var]([](auto& n) {
        Node var = *n.first;
        if (maybe_keyword(var))
        {
          Nodes defs = var->lookup();
          return defs.size() == 1 && defs[0]->type() == Keyword;
        }
        return false;
      }) >>
        [](Match& _) {
          auto name = _(Var)->location().view();
          if (name == "if")
          {
            return IfTruthy ^ _(Var)->location();
          }

          if (name == "in")
          {
            return InSome ^ _(Var)->location();
          }

          if (name == "contains")
          {
            return Contains ^ _(Var)->location();
          }

          return err(_(Var), "unsupported keyword");
        },
    };
  }
}