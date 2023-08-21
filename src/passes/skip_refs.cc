#include "lang.h"
#include "passes.h"

#include <set>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  using SkipSet = std::shared_ptr<std::set<std::string>>;

  struct SkipPrefix
  {
    std::size_t length;
    std::string path;
  };

  SkipPrefix skip_prefix_varseq(
    const SkipSet& skips, Node varseq, const BuiltIns& builtins)
  {
    Node head = varseq->front();
    std::string path = std::string(head->location().view());
    std::size_t skip = 0;
    std::string longest_path = "";

    for (std::size_t i = 1; i < varseq->size(); i++)
    {
      Node var = varseq->at(i);
      std::string var_str = std::string(var->location().view());
      path = path + "." + var_str;
      if (skips->contains(path) || builtins.is_builtin(path))
      {
        skip = i + 1;
        longest_path = path;
      }
    }

    return {skip, longest_path};
  }

  SkipPrefix skip_prefix_ref(const SkipSet& skips, Node ref)
  {
    Node head = (ref / RefHead)->front();
    std::string path = std::string(head->location().view());
    Node refargseq = (ref / RefArgSeq);
    std::size_t skip = 0;
    std::string longest_path = "";
    for (std::size_t i = 0; i < refargseq->size(); i++)
    {
      Node refarg = refargseq->at(i);
      std::string refarg_str;
      if (refarg->type() == RefArgDot)
      {
        refarg_str = std::string(refarg->front()->location().view());
      }
      else if (refarg->type() == RefArgBrack)
      {
        refarg_str = to_json(refarg->front());
      }
      else
      {
        return {0, ""};
      }

      path = path + "." + strip_quotes(refarg_str);
      Node test = Var ^ path;
      if (skips->contains(path))
      {
        skip = i + 1;
        longest_path = path;
      }
    }

    return {skip, longest_path};
  }

  void find_used_builtins(
    const Node& node,
    const BuiltIns& builtins,
    std::set<std::string>& used_builtins)
  {
    if (node->type() == Var)
    {
      std::string path = std::string(node->location().view());
      if (builtins.is_builtin(path))
      {
        used_builtins.insert(path);
      }
    }
    else
    {
      for (auto& child : *node)
      {
        find_used_builtins(child, builtins, used_builtins);
      }
    }
  }
}

namespace rego
{
  // Elides references to skips for efficiency.
  PassDef skip_refs(const BuiltIns& builtins)
  {
    SkipSet skips = std::make_shared<std::set<std::string>>();

    PassDef skip_refs = {
      In(RefTerm) * T(Ref)[Ref]([skips](auto& n) {
        return skip_prefix_ref(skips, *n.first).length > 0;
      }) >>
        [skips](Match& _) {
          SkipPrefix skip = skip_prefix_ref(skips, _(Ref));
          Node refargseq = (_(Ref) / RefArgSeq);
          refargseq->erase(
            refargseq->begin(), refargseq->begin() + skip.length);
          if (refargseq->size() == 0)
          {
            return Var ^ skip.path;
          }

          return Ref << (RefHead << (Var ^ skip.path)) << refargseq;
        },

      In(ExprCall) * T(VarSeq)[VarSeq]([skips, builtins](auto& n) {
        return skip_prefix_varseq(skips, *n.first, builtins).length > 0;
      }) >>
        [skips, builtins](Match& _) {
          SkipPrefix skip = skip_prefix_varseq(skips, _(VarSeq), builtins);
          Node varseq = (_(VarSeq));
          varseq->erase(varseq->begin(), varseq->begin() + skip.length);

          varseq->push_front(Var ^ skip.path);

          return varseq;
        }};

    skip_refs.pre(Rego, [skips](Node node) {
      Node skipseq = node / SkipSeq;
      for (Node skip : *skipseq)
      {
        std::string path = std::string((skip / Key)->location().view());
        skips->insert(path);
      }

      return 0;
    });

    skip_refs.post(Rego, [skips, builtins](Node node) {
      std::set<std::string> used_builtins;
      find_used_builtins(node, builtins, used_builtins);
      Node skipseq = node / SkipSeq;
      for (auto& builtin : used_builtins)
      {
        skipseq << (Skip << (Key ^ builtin) << (BuiltInHook ^ builtin));
      }

      return 0;
    });

    return skip_refs;
  }
}