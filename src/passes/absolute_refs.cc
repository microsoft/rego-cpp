#include "passes.h"
#include "utils.h"
#include "errors.h"

#include <set>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  bool is_ref_to_type(const Node& var, const std::set<Token>& types)
  {
    Nodes defs = var->lookup();
    if (defs.size() > 0)
    {
      return types.contains(defs[0]->type());
    }
    else
    {
      return false;
    }
  }

  Node build_ref(Node leaf)
  {
    if (leaf->type() == Data)
    {
      return Ref << (RefHead << (Var ^ "data")) << RefArgSeq;
    }

    if (leaf->type() == Module)
    {
      Node ref = (leaf / Package)->front()->clone();
      Node refhead = (ref / RefHead)->front()->clone();
      Node refargseq = ref / RefArgSeq;
      refargseq->push_front(RefArgDot << refhead);
      return Ref << (RefHead << (Var ^ "data")) << refargseq;
    }

    Node ref = build_ref(leaf->parent()->shared_from_this());

    if (leaf->type() == Policy || leaf->type() == DataModule)
    {
      return ref;
    }

    if (leaf->type() == Submodule)
    {
      (ref / RefArgSeq) << (RefArgDot << (Var ^ (leaf / Key)));
    }
    else if(RuleTypes.contains(leaf->type()))
    {
      (ref / RefArgSeq) << (RefArgDot << (leaf / Var)->clone());
    }else{
      return err(leaf, "Unable to build ref");
    }

    return ref;
  }
}

namespace rego
{
  // Converts all local rule references to absolute references. Also
  // converts all imports to their absolute references.
  PassDef absolute_refs()
  {
    return {
      (In(RefTerm) / In(RuleRef)) * T(Var)[Var]([](auto& n) {
        return is_ref_to_type(*n.first, {Import});
      }) >>
        [](Match& _) {
          // <import>
          Nodes defs = _(Var)->lookup();
          Node import = defs[0];
          Node ref = import / Ref;
          return ref->clone();
        },

      (In(RefTerm) / In(RuleRef)) *
          (T(Ref)
           << ((T(RefHead) << T(Var)[Var](
                  [](auto& n) { return is_ref_to_type(*n.first, {Import}); })) *
               T(RefArgSeq)[RefArgSeq])) >>
        [](Match& _) {
          // <import>.dot <import>[brack]
          Nodes defs = _(Var)->lookup();
          Node import = defs[0];
          Node ref = import / Ref;
          Node refhead = (ref / RefHead)->clone();
          Node refargseq = (ref / RefArgSeq)->clone();
          refargseq->insert(
            refargseq->end(), _(RefArgSeq)->begin(), _(RefArgSeq)->end());
          return Ref << refhead << refargseq;
        },

      (In(RefTerm) / In(RuleRef)) * T(Var)[Var]([](auto& n) {
        return is_ref_to_type(*n.first, RuleTypes);
      }) >>
        [](Match& _) {
          // <rule>
          Nodes defs = _(Var)->lookup();
          Node rule = defs[0];
          return build_ref(rule);
        },

      (In(RefTerm) / In(RuleRef)) *
          (T(Ref)
           << ((T(RefHead) << T(Var)[Var]([](auto& n) {
                  return is_ref_to_type(*n.first, RuleTypes);
                })) *
               T(RefArgSeq)[RefArgSeq])) >>
        [](Match& _) {
          // <rule>.dot <rule>[brack]
          Nodes defs = _(Var)->lookup();
          Node rule = defs[0];
          Node ref = build_ref(rule);
          (ref / RefArgSeq) << *_[RefArgSeq];
          return ref;
        },

      In(Policy) * T(Import) >> [](Match&) { return Node{}; },
    };
  }
}