#include "unify.hh"

namespace
{
  using namespace rego;
  using namespace wf::ops;

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

    Node ref = build_ref(leaf->parent()->intrusive_ptr_from_this());

    if (leaf->type() == Policy || leaf->type() == DataModule)
    {
      return ref;
    }

    if (leaf->type() == Submodule)
    {
      (ref / RefArgSeq) << (RefArgDot << (Var ^ (leaf / Key)));
    }
    else if (contains(RuleTypes, leaf->type()))
    {
      (ref / RefArgSeq) << (RefArgDot << (leaf / Var)->clone());
    }
    else
    {
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
      "absolute_refs",
      wf_pass_absolute_refs,
      dir::topdown,
      {
        In(RefTerm, RuleRef) * T(Var)[Var]([](auto& n) {
          return is_ref_to_type(n.front(), RuleTypes);
        }) >>
          [](Match& _) {
            ACTION();
            // <rule>
            Nodes defs = _(Var)->lookup();
            Node rule = defs[0];
            return build_ref(rule);
          },

        In(RefTerm, RuleRef, ExprCall) *
            (T(Ref)
             << ((T(RefHead) << T(Var)[Var]([](auto& n) {
                    return is_ref_to_type(n.front(), RuleTypes);
                  })) *
                 T(RefArgSeq)[RefArgSeq])) >>
          [](Match& _) {
            ACTION();
            // <rule>.dot <rule>[brack]
            Nodes defs = _(Var)->lookup();
            Node rule = defs[0];
            Node ref = build_ref(rule);
            Node refargseq = (ref / RefArgSeq);
            refargseq << *_[RefArgSeq];
            return ref;
          },

        In(Policy) * T(Import) >>
          [](Match&) {
            ACTION();
            return Node{};
          },
      }};
  }
}