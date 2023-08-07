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

  SkipPrefix skip_prefix(const SkipSet& skips, Node ref)
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
}

namespace rego
{
  // Elides references to skips for efficiency.
  PassDef skip_refs()
  {
    SkipSet skips = std::make_shared<std::set<std::string>>();

    PassDef skip_refs = {
      In(RefTerm) * T(Ref)[Ref]([skips](auto& n) {
        return skip_prefix(skips, *n.first).length > 0;
      }) >>
        [skips](Match& _) {
          SkipPrefix skip = skip_prefix(skips, _(Ref));
          Node refargseq = (_(Ref) / RefArgSeq);
          refargseq->erase(
            refargseq->begin(), refargseq->begin() + skip.length);
          if (refargseq->size() == 0)
          {
            return Var ^ skip.path;
          }

          return Ref << (RefHead << (Var ^ skip.path)) << refargseq;
        },
    };

    skip_refs.pre(Rego, [skips](Node node) {
      Node skipseq = node / SkipSeq;
      for (Node skip : *skipseq)
      {
        std::string path = std::string((skip / Key)->location().view());
        skips->insert(path);
      }

      return 0;
    });

    return skip_refs;
  }
}