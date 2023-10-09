#include "internal.hh"

namespace
{
  using namespace rego;

  bool maybe_keyword(const Node& node)
  {
    if (is_in(node, {Package}))
    {
      return false;
    }
    std::string name = std::string(node->location().view());
    return contains(Keywords, name);
  }
}

namespace rego
{
  // Replaces keywords with their specific tokens, conditional on whether those
  // keywords have been imported.
  PassDef keywords()
  {
    return {
      "keywords",
      wf_pass_keywords,
      dir::topdown,
      {
        In(Group) * T(Var)[Var]([](auto& n) {
          Node var = *n.first;
          if (maybe_keyword(var))
          {
            Nodes defs = var->lookup();
            return defs.size() >= 1 && defs[0]->type() == Keyword;
          }
          return false;
        }) >>
          [](Match& _) {
            ACTION();
            auto name = _(Var)->location().view();
            if (name == "if")
            {
              return If ^ _(Var)->location();
            }

            if (name == "in")
            {
              return IsIn ^ _(Var)->location();
            }

            if (name == "contains")
            {
              return Contains ^ _(Var)->location();
            }

            if (name == "every")
            {
              return Every ^ _(Var)->location();
            }

            return err(_(Var), "unsupported keyword");
          },
      }};
  }
}