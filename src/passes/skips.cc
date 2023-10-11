#include "internal.hh"

namespace
{
  using namespace rego;

  using SkipMap = std::shared_ptr<std::map<std::string, Node>>;

  void find_withs(SkipMap skips, Node node)
  {
    if (node->type() == With)
    {
      Node ref = (node / RuleRef)->front();
      std::ostringstream buf;
      if (ref->type() == Var)
      {
        buf << ref->location().view();
      }
      else
      {
        buf << (ref / RefHead)->front()->location().view();
        Node refargseq = ref / RefArgSeq;
        for (auto& arg : *refargseq)
        {
          if (arg->type() == RefArgDot)
          {
            buf << "." << (arg->front())->location().view();
          }
          else
          {
            buf << "[" << (arg->front())->location().view() << "]";
          }
        }
      }

      std::string path = buf.str();

      if (!contains(skips, path) && path != "data" && path != "input")
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
    if (starts_with(rhs.view(), lhs))
    {
      return std::string(rhs.view());
    }

    if (rhs.view()[0] == '[')
    {
      Location key = rhs;
      key.pos += 2;
      key.len -= 4;
      if (all_alnum(key.view()))
      {
        return lhs + "." + std::string(key.view());
      }
      else
      {
        return lhs + std::string(rhs.view());
      }
    }

    return lhs + "." + std::string(rhs.view());
  }

  void to_absolute_names(const Node& module, const std::string& branch)
  {
    for (auto& item : *module)
    {
      if (item->type() == Submodule)
      {
        Node key = item / Key;
        std::string stem = concat(branch, key->location());
        item->replace(key, Key ^ stem);
        to_absolute_names(item / Val, stem);
      }
      else
      {
        Node var = item / Var;
        std::string leaf = concat(branch, var->location());
        item->replace(var, Var ^ leaf);
      }
    }
  }
}

namespace rego
{
  PassDef datarule()
  {
    return {
      "datarule",
      wf_pass_datarule,
      dir::bottomup | dir::once,
      {
        In(DataModule) *
            (T(DataRule) << (T(Var)[Var] * T(DataTerm)[DataTerm])) >>
          [](Match& _) {
            ACTION();
            return RuleComp << _(Var) << Empty << _(DataTerm) << (Int ^ "0");
          },
      }};
  }

  // Primes the skiplist with With locations, to cover the cases in which
  // a With statement adds a path into the base document which does exist
  // already. This ensures those paths are still in the symbol table.
  PassDef skips()
  {
    SkipMap skip_links = std::make_shared<std::map<std::string, Node>>();

    PassDef skips = {"skips", wf_pass_skips, dir::topdown};

    skips.pre(Rego, [skip_links](Node node) {
      to_absolute_names(node / Data / Val, "data");
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
