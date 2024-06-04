#include "rego.hh"
#include "unify.hh"

namespace rego
{
  // Expands imports to their fully-qualified values.
  PassDef expand_imports(BuiltIns builtins)
  {
    return {
      "expand_imports",
      wf_pass_expand_imports,
      dir::bottomup | dir::once,
      {In(RefTerm, RuleRef) * T(Var)[Var] >> [](Match& _) -> Node {
         // <import>
         ACTION();
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
           (T(Ref)
            << ((T(RefHead) << T(Var)[Var]) * T(RefArgSeq)[RefArgSeq])) >>
         [](Match& _) -> Node {
         // <import>.dot <import>[brack]
         ACTION();
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

       In(RuleRef, RefTerm) * T(Ref)[Ref]([builtins](auto n) {
         std::string flatten = flatten_ref(n.front());
         return builtins->is_builtin({flatten});
       }) >>
         [](Match& _) {
           auto ref = _(Ref);
           std::string flatten = flatten_ref(ref);
           ref / RefHead = RefHead << (Var ^ flatten);
           (ref / RefArgSeq) = NodeDef::create(RefArgSeq);
           return ref;
         }}};
  }
}