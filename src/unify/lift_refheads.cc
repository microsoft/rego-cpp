#include "unify.hh"

namespace
{
  using namespace rego;

  class RefTree
  {
  public:
    RefTree() : root{"", {}} {}

    void insert(Node& ref)
    {
      assert(ref == Ref);
      Node refhead = ref / RefHead;
      Node refargseq = ref / RefArgSeq;

      Node var = refhead->front();
      assert(var == Var);
      RefNode* current = root.at(var->location().view());
      for (auto& refarg : *refargseq)
      {
        current = current->at(
          refarg == RefArgDot ? refarg->front()->location().view() :
                                refargbrack_string(refarg));
      }
    }

    size_t max_prefix(Node& ref) const
    {
      assert(ref == Ref);
      Node refhead = ref / RefHead;
      Node refargseq = ref / RefArgSeq;

      Node var = refhead->front();
      assert(var == Var);
      if (!root.contains(var->location().view()))
      {
        return 0;
      }

      const RefNode* current = root.at(var->location().view());
      size_t prefix = 0;
      for (auto& refarg : *refargseq)
      {
        if (refarg == RefArgDot)
        {
          if (current->contains(refarg->front()->location().view()))
          {
            current = current->at(refarg->front()->location().view());
            prefix++;
          }
          else
          {
            break;
          }
        }
        else if (refarg == RefArgBrack)
        {
          std::string key = refargbrack_string(refarg);
          if (current->contains(key))
          {
            current = current->at(key);
            prefix++;
          }
          else
          {
            break;
          }
        }
      }

      return prefix;
    }

    void clear()
    {
      root.children.clear();
    }

    size_t size() const
    {
      size_t size = 0;
      std::vector<const RefNode*> stack{&root};
      while (!stack.empty())
      {
        const RefNode* current = stack.back();
        stack.pop_back();
        size += current->children.size();
        for (auto& child : current->children)
        {
          stack.push_back(&child.second);
        }
      }

      return size;
    }

    void dump(std::ostream& os) const
    {
      std::vector<std::pair<const RefNode*, std::string>> stack{
        {&this->root, ""}};
      while (!stack.empty())
      {
        auto [current, indent] = stack.back();
        stack.pop_back();
        if (current == nullptr)
        {
          os << indent << ')' << std::endl;
          continue;
        }

        os << indent << '(' << current->arg;
        if (current->children.empty())
        {
          os << ')' << std::endl;
          continue;
        }

        os << std::endl;
        stack.push_back({nullptr, indent});
        for (auto& child : current->children)
        {
          stack.push_back({&child.second, indent + "  "});
        }
      }
    }

  private:
    static std::string refargbrack_string(const Node& arg)
    {
      std::ostringstream buf;

      Node index = arg->front();
      if (index->type() == Scalar)
      {
        index = index->front();
      }

      Location key = index->location();
      if (index->type() == JSONString)
      {
        key.pos += 1;
        key.len -= 2;
      }

      if (all_alnum(key.view()))
      {
        buf << key.view();
      }
      else
      {
        buf << "[" << index->location().view() << "]";
      }

      return buf.str();
    }

    struct RefNode
    {
      std::string arg;
      std::map<std::string, RefNode> children;

      RefNode* at(const std::string_view& key)
      {
        if (!contains(key))
        {
          std::string key_str(key);
          children[key_str] = RefNode{key_str, {}};
        }

        auto it =
          std::find_if(children.begin(), children.end(), [key](auto& p) {
            return p.first == key;
          });
        return &it->second;
      }

      const RefNode* at(const std::string_view& key) const
      {
        auto it =
          std::find_if(children.begin(), children.end(), [key](auto& p) {
            return p.first == key;
          });
        return &it->second;
      }

      bool contains(const std::string_view& key) const
      {
        return std::find_if(children.begin(), children.end(), [key](auto& p) {
                 return p.first == key;
               }) != children.end();
      }
    };

    RefNode root;
  };

  using RefHeads = std::shared_ptr<RefTree>;

  static std::ostream& operator<<(std::ostream& os, RefHeads refheads)
  {
    refheads->dump(os);
    return os;
  }

  void collect_refheads(const Node& node, RefHeads refheads)
  {
    if (node->type() == Rule)
    {
      Node rulehead = node / RuleHead;
      Node ruleref = rulehead / RuleRef;
      if (ruleref->front()->type() == Var)
      {
        return;
      }

      Node module = node->parent()->parent();
      Node prefix_ref = concat_refs(Var ^ "data", (module / Package)->front());
      if (prefix_ref->type() == Error)
      {
        return;
      }

      Node ref = ruleref->front();
      Node full_ref = concat_refs(prefix_ref, ref);
      if (full_ref->type() == Error)
      {
        return;
      }

      refheads->insert(full_ref);
    }
    else
    {
      for (auto& child : *node)
      {
        collect_refheads(child, refheads);
      }
    }
  }

  void prepend_refs(const Node& node, Node& prefix_ref, const RefHeads refheads)
  {
    if (node == Term || node == ExprCall)
    {
      Node ref = node->front();
      if (ref->type() != Ref)
      {
        return;
      }

      Node refhead = ref / RefHead;
      if (refhead->front()->lookup().empty())
      {
        Node new_ref = concat_refs(prefix_ref, ref);
        if (new_ref->type() == Error)
        {
          return;
        }

        size_t prefix = refheads->max_prefix(prefix_ref);
        size_t full = refheads->max_prefix(new_ref);
        if (full > prefix)
        {
          node->replace(ref, new_ref);
        }
      }

      if (node == ExprCall)
      {
        prepend_refs(node / ExprSeq, prefix_ref, refheads);
      }
    }
    else
    {
      for (auto& child : *node)
      {
        prepend_refs(child, prefix_ref, refheads);
      }
    }
  }
}

namespace rego
{
  // This pass finds all complex, i.e. reference-based, rule heads and lifts
  // them into their own modules which encode their prefix. For example, a rule
  // called a.b.c.foo will be lifted into a new module with package a.b.c and
  // rule foo.
  PassDef lift_refheads()
  {
    RefHeads refheads = std::make_shared<RefTree>();

    PassDef lift_refheads{
      "lift_refheads",
      wf_pass_lift_refheads,
      dir::topdown,
      {
        In(RuleRef) *
            (T(Ref) << ((T(RefHead) << T(Var)[Var]) * (T(RefArgSeq) << End))) >>
          [](Match& _) {
            ACTION();
            return _(Var);
          },

        In(Policy) * T(Rule)[Rule]([](auto& n) {
          Node rule = n.front();
          Node ruleref = (rule / RuleHead / RuleRef)->front();
          if (ruleref->type() == Ref)
          {
            return (ruleref / RefArgSeq)->size() > 0;
          }

          return false;
        }) >>
          [refheads](Match& _) {
            ACTION();
            Node module = _(Rule)->parent()->parent();
            Node imports = (module / ImportSeq)->clone();
            Node package_ref = (module / Package)->front();
            Node version = (module / Version)->clone();
            Node rulehead = _(Rule) / RuleHead;
            Node ruleref = rulehead / RuleRef;
            Node ref = ruleref->front();
            Node refhead = ref / RefHead;
            Node refargseq = ref / RefArgSeq;
            Node vararg = refargseq->back();
            refargseq->pop_back();

            Node prefix_ref = concat_refs(Var ^ "data", package_ref);
            if (prefix_ref->type() == Error)
            {
              return prefix_ref;
            }

            prepend_refs(_(Rule) / RuleBodySeq, prefix_ref, refheads);
            prepend_refs(rulehead / RuleHeadType, prefix_ref, refheads);

            Node new_package_ref = concat_refs(package_ref, ref);
            if (new_package_ref->type() == Error)
            {
              return new_package_ref;
            }

            Node var;
            if (vararg->type() == RefArgDot)
            {
              var = vararg->front();
            }
            else
            {
              Location loc = vararg->front()->location();
              var = Var ^ ("[" + std::string(loc.view()) + "]");
            }

            rulehead->replace(ruleref, RuleRef << var);

            return Lift << ModuleSeq
                        << (Module << (Package << new_package_ref) << version
                                   << imports << (Policy << _(Rule)));
          },
      }};

    lift_refheads.pre(Rego, [refheads](Node node) {
      if (refheads->size() == 0)
      {
        collect_refheads(node, refheads);
      }

      logging::Trace() << "RefHeads: " << refheads;

      return 0;
    });

    lift_refheads.post(Rule, [refheads](Node node) {
      Node module = node->parent()->parent();
      Node rulehead = node / RuleHead;
      Node package_ref = (module / Package)->front();
      Node prefix_ref = concat_refs(Var ^ "data", package_ref);
      if (prefix_ref->type() == Error)
      {
        return 0;
      }

      prepend_refs(node / RuleBodySeq, prefix_ref, refheads);
      prepend_refs(rulehead / RuleHeadType, prefix_ref, refheads);
      return 0;
    });

    lift_refheads.post([refheads](Node) {
      refheads->clear();
      return 0;
    });

    return lift_refheads;
  }
}