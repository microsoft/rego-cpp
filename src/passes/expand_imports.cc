#include "internal.hh"

namespace rego
{
  // Expands imports to their fully-qualified values.
  PassDef expand_imports()
  {
    return {
      In(RefTerm, RuleRef) * T(Var)[Var] >>
        [](Match& _) -> Node {
          // <import>
          Nodes defs = _(Var)->lookup();
          if (defs.empty())
            return NoChange;
          Node import = defs[0];
          if (import->type() != Import)
            return NoChange;
          Node ref = import / Ref;
          return ref->clone();
        },

      In(RefTerm, RuleRef) *
          (T(Ref) << ((T(RefHead) << T(Var)[Var]) * T(RefArgSeq)[RefArgSeq])) >>
        [](Match& _) -> Node {
          // <import>.dot <import>[brack]
          Nodes defs = _(Var)->lookup();
          if (defs.empty())
            return NoChange;
          Node import = defs[0];
          if (import->type() != Import)
            return NoChange;

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