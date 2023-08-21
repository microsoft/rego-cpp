#include "builtins.h"
#include "passes.h"

namespace
{
  using namespace rego;

  using SkipMap = std::shared_ptr<std::map<std::string, Node>>;

  void find_skips(SkipMap skips, std::string path, Node varseq, Node node)
  {
    if (node->type() == DataTerm)
    {
      skips->insert({path, varseq});
      return;
    }

    if (node->type() == Module)
    {
      for (auto member : *node)
      {
        std::string key_str;
        Node val;
        if (member->type() == Import)
        {
          continue;
        }
        else if (member->type() == Submodule)
        {
          auto key = member / Key;
          key_str = strip_quotes(std::string(key->location().view()));
        }
        else
        {
          auto var = member / Var;
          key_str = strip_quotes(std::string(var->location().view()));
        }

        std::string child_path = path + "." + key_str;
        Node child_vars = varseq->clone();
        child_vars->push_back(Var ^ key_str);

        if (member->type() == Submodule)
        {
          find_skips(skips, child_path, child_vars, member / Val);
        }
        else
        {
          skips->insert({child_path, RuleRef << child_vars});
        }
      }
    }
    else
    {
      Node itemseq;
      if (node->type() == Data)
      {
        itemseq = node / DataItemSeq;
      }
      else if (node->type() == Input)
      {
        itemseq = node / Val;
        if(itemseq->type() == DataArray){
          return;
        }
      }
      else if (node->type() == DataObject)
      {
        itemseq = node;
      }
      else
      {
        return;
      }

      for (auto item : *itemseq)
      {
        auto key = item / Key;
        auto val = item / Val;
        std::string key_str = strip_quotes(std::string(key->location().view()));
        std::string child_path = path + "." + key_str;
        Node child_vars = varseq->clone();
        child_vars->push_back(Var ^ key_str);

        find_skips(skips, child_path, child_vars, val);
      }
    }
  }

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
      find_skips(skip_links, "data", VarSeq << (Var ^ "data"), node / Data);
      find_skips(skip_links, "input", VarSeq << (Var ^ "input"), node / Input);
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