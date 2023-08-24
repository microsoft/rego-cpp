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

  std::string concat(const std::string& lhs, const Location& rhs)
  {
    if(rhs.view().starts_with(lhs))
    {
      return std::string(rhs.view());
    }

    if(rhs.view()[0] == '['){
      return lhs + std::string(rhs.view());
    }
    
    return lhs + "." + std::string(rhs.view());
  }

  void to_absolute_names(const Node& module, const std::string& branch)
  {
    for(auto& item : *module)
    {
      if(item->type() == Submodule)
      {
        Node key = item / Key;
        std::string stem = concat(branch, key->location());
        item->replace(key, Key ^ stem);
        to_absolute_names(item / Val, stem);
      }else{
        Node var = item / Var;
        std::string leaf = concat(branch, var->location());
        item->replace(var, Var ^ leaf);
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
      to_absolute_names(node / Data / DataModule, "data");
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
