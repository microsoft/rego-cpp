#include "passes.h"
#include "errors.h"
#include "utils.h"


namespace
{
  using namespace rego;

  using SkipMap = std::shared_ptr<std::map<std::string, Node>>;

  void find_withs(SkipMap skips, Node node)
  {
    if (node->type() == With)
    {
      Node varseq = node / VarSeq;
      std::ostringstream buf;
      std::string sep = "";
      for (auto& var : *varseq)
      {
        buf << sep << var->location().view();
        sep = ".";
      }
      std::string path = buf.str();

      if (!skips->contains(path) && path != "data" && path != "input")
      {
        skips->insert({path, Undefined});
      }
    }
    else
    {
      for (auto& child : *node)
      {
        find_withs(skips, child);
      }
    }
  }
}

namespace rego
{
  // Discovers skip paths in the AST to constant DataTerm nodes.
  PassDef skips()
  {
    SkipMap skip_links = std::make_shared<std::map<std::string, Node>>();

    PassDef skips = {
      dir::topdown | dir::once,
    };

    skips.pre(Rego, [skip_links](Node node) {
      find_withs(skip_links, node / Data);
      Node skipseq = NodeDef::create(SkipSeq);
      for (auto [path, ref] : *skip_links)
      {
        skipseq->push_back(Skip << (Key ^ path) << ref);
      }
      node->push_back(skipseq);

      return 0;
    });

    return skips;
  }
}