#include "passes.h"
#include "utils.h"

namespace
{
  using namespace rego;

  using RefHeads = std::shared_ptr<std::set<std::string>>;

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

      Node module = node->parent()->parent()->shared_from_this();
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

      refheads->insert(flatten_ref(full_ref));
    }
    else
    {
      for (auto& child : *node)
      {
        collect_refheads(child, refheads);
      }
    }
  }

  bool starts_with(const std::string& ref, RefHeads refheads)
  {
    for (auto& refhead : *refheads)
    {
      if (ref.starts_with(refhead))
      {
        return true;
      }
    }

    return false;
  }

  void prepend_refs(Node& node, Node& prefix_ref, RefHeads refheads)
  {
    if (node->type() == Term || node->type() == RuleRef)
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

        std::string new_ref_str = flatten_ref(new_ref);
        if (starts_with(new_ref_str, refheads))
        {
          node->replace(ref, new_ref);
        }
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
  PassDef lift_refheads()
  {
    RefHeads refheads = std::make_shared<std::set<std::string>>();

    PassDef lift_refheads = {
      In(RefHead) *
          (T(Ref) << ((T(RefHead) << T(Var)[Var]) * (T(RefArgSeq) << End))) >>
        [](Match& _) { return _(Var); },

      In(Policy) * T(Rule)[Rule]([](auto& n) {
        Node rule = *n.first;
        Node ruleref = (rule / RuleHead / RuleRef)->front();
        if (ruleref->type() == Ref)
        {
          return (ruleref / RefArgSeq)->size() > 0;
        }

        return false;
      }) >>
        [refheads](Match& _) {
          Node module = _(Rule)->parent()->parent()->shared_from_this();
          Node imports = (module / ImportSeq)->clone();
          Node package_ref = (module / Package)->front();
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

          prepend_refs(_(Rule) / Body, prefix_ref, refheads);
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
            var = Var ^ "[" + std::string(loc.view()) + "]";
          }

          rulehead->replace(ruleref, RuleRef << var);

          return Lift << ModuleSeq
                      << (Module << (Package << new_package_ref) << imports
                                 << (Policy << _(Rule)));
        },
    };

    lift_refheads.pre(Rego, [refheads](Node node) {
      if (refheads->size() == 0)
      {
        collect_refheads(node, refheads);
      }

      return 0;
    });

    lift_refheads.post(Rule, [refheads](Node node) {
      Node module = node->parent()->parent()->shared_from_this();
      Node rulehead = node / RuleHead;
      Node package_ref = (module / Package)->front();
      Node prefix_ref = concat_refs(Var ^ "data", package_ref);
      if (prefix_ref->type() == Error)
      {
        return 0;
      }

      prepend_refs(node / Body, prefix_ref, refheads);
      prepend_refs(rulehead / RuleHeadType, prefix_ref, refheads);
      return 0;
    });

    return lift_refheads;
  }
}