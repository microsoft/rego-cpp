#include "lang.h"
#include "passes.h"

#include <set>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  bool is_ref_to_type(const Node& var, const std::set<Token>& types)
  {
    Nodes defs = var->lookup();
    if (defs.size() == 1)
    {
      return types.contains(defs[0]->type());
    }
    else
    {
      return false;
    }
  }
}

namespace rego
{
  // Converts all local rule references to absolute references. Also
  // converts all imports to their absolute references.
  PassDef absolute_refs()
  {
    return {
      In(RefTerm) * T(Var)[Var]([](auto& n) {
        return is_ref_to_type(*n.first, {Import});
      }) >>
        [](Match& _) {
          // <import>
          Nodes defs = _(Var)->lookup();
          Node import = defs[0];
          Node ref = import / Ref;
          return ref->clone();
        },

      In(RefTerm) *
          (T(Ref)
           << ((T(RefHead) << T(Var)[Var](
                  [](auto& n) { return is_ref_to_type(*n.first, {Import}); })) *
               (T(RefArgSeq)[RefArgSeq]))) >>
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

      In(RefTerm) * T(Var)[Var]([](auto& n) {
        return is_ref_to_type(*n.first, RuleTypes);
      }) >>
        [](Match& _) {
          // <rule>
          Nodes defs = _(Var)->lookup();
          Node rule = defs[0];
          Node module = rule->parent()->parent()->shared_from_this();
          Node ref = (module / Package)->front()->clone();
          Node refhead = (ref / RefHead)->front()->clone();
          Node refargseq = ref / RefArgSeq;
          refargseq->push_front(RefArgDot << refhead);
          refargseq->push_back(RefArgDot << _(Var));
          return Ref << (RefHead << (Var ^ "data")) << refargseq;
        },

      In(RefTerm) *
          (T(Ref) << (T(RefHead) << T(Var)[Var]([](auto& n) {
                        return is_ref_to_type(*n.first, RuleTypes);
                      }))
                  << T(RefArgSeq)[RefArgSeq]) >>
        [](Match& _) {
          // <rule>.dot <rule>[brack]
          Nodes defs = _(Var)->lookup();
          Node rule = defs[0];
          Node module = rule->parent()->parent()->shared_from_this();
          Node ref = (module / Package)->front()->clone();
          Node refhead = (ref / RefHead)->front()->clone();
          Node refargseq = ref / RefArgSeq;
          refargseq->push_front(RefArgDot << refhead);
          refargseq->push_back(RefArgDot << _(Var));
          refargseq->insert(
            refargseq->end(), _(RefArgSeq)->begin(), _(RefArgSeq)->end());
          return Ref << (RefHead << (Var ^ "data")) << refargseq;
        },
    };
  }
}