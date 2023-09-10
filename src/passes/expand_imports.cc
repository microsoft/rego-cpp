#include "internal.h"

namespace rego
{
  // Expands imports to their fully-qualified values.
  PassDef expand_imports()
  {
    return {
      In(RefTerm, RuleRef) * T(Var)[Var]([](auto& n) {
        return is_ref_to_type(*n.first, {Import});
      }) >>
        [](Match& _) {
          // <import>
          Nodes defs = _(Var)->lookup();
          Node import = defs[0];
          Node ref = import / Ref;
          return ref->clone();
        },

      In(RefTerm, RuleRef) *
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
    };
  }  
}