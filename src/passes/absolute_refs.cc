#include "passes.h"
#include "utils.h"

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

    return false;
  }

  bool is_absolute(const Node& node)
  {
    Location name = node->location();
    return name.view().starts_with("data.");
  }

  struct FlatRef
  {
    Node node;
  };

  std::ostream& operator<<(std::ostream& os, const FlatRef& ref)
  {
    os << (ref.node / RefHead)->front()->location().view();
    for (auto& refarg : *(ref.node / RefArgSeq))
    {
      if (refarg->type() == RefArgDot)
      {
        os << "." << refarg->front()->location().view();
      }
      else if (refarg->type() == RefArgBrack)
      {
        std::string key = strip_quotes(to_json(refarg->front()));
        if(all_alnum(key)){
          os << "." << key;
        }else{
          os << "[\"" << key << "\"]";
        }
      }
    }
    return os;
  }

  std::string concat_refs(const Node& prefix, const Node& var)
  {
    std::ostringstream oss;
    oss << FlatRef{prefix} << "." << var->location().view();
    return oss.str();
  }

  std::string flatten_ref(const Node& prefix)
  {
    std::ostringstream oss;
    oss << FlatRef{prefix};
    return oss.str();
  }

  const auto inline RuleToken =
    T(RuleComp) / T(RuleObj) / T(RuleSet) / T(RuleFunc) / T(DefaultRule);
}

namespace rego
{
  // Converts all local rule references to absolute references. Also
  // converts all imports to their absolute references. Finally, assigns
  // all rules fully-qualified names.
  PassDef absolute_refs()
  {
    return {
      In(Policy) * (RuleToken[Val] << T(Var)([](auto& n) {
                      return !is_absolute(*n.first);
                    })) >>
        [](Match& _) {
          Node mod = _(Val)->parent()->parent()->shared_from_this();
          Node prefix = (mod / Package)->front();
          Node rule = _(Val);
          Node var = rule / Var;
          ;
          std::string name = "data." + concat_refs(prefix, var);
          rule->replace(var, Var ^ name);
          return rule;
        },

      In(DataItem) *
          T(Key)[Key]([](auto& n) { return !is_absolute(*n.first); }) >>
        [](Match& _) {
          std::string name = "data." + std::string(_(Key)->location().view());
          return Key ^ name;
        },

      In(RefTerm) * T(Var)[Var]([](auto& n) {
        return is_ref_to_type(*n.first, {Import});
      }) >>
        [](Match& _) {
          // <import>
          Nodes defs = _(Var)->lookup();
          Node import = defs[0];
          Node ref = import / Ref;
          return Var ^ flatten_ref(ref);
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
          Node datahead = RefHead << (Var ^ "data");
          std::string flat = flatten_ref(Ref << datahead << refargseq);
          Node flathead = RefHead << (Var ^ flat);
          return Ref << flathead << _(RefArgSeq);
        },

      In(ExprCall) *
          (T(VarSeq)
           << (T(Var)[Var](
                 [](auto& n) { return is_ref_to_type(*n.first, {Import}); }) *
               End)) >>
        [](Match& _) {
          // <import>
          Nodes defs = _(Var)->lookup();
          Node import = defs[0];
          Node ref = import / Ref;
          return VarSeq << (Var ^ flatten_ref(ref));
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
          return Var ^
            flatten_ref(Ref << (RefHead << (Var ^ "data")) << refargseq);
        },

      In(ExprCall) *
          (T(VarSeq)
           << (T(Var)[Var](
                 [](auto& n) { return is_ref_to_type(*n.first, RuleTypes); }) *
               End)) >>
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
          return VarSeq
            << (Var ^
                flatten_ref(Ref << (RefHead << (Var ^ "data")) << refargseq));
        },

      In(RefTerm) *
          (T(Ref)
           << ((T(RefHead) << T(Var)[Var]([](auto& n) {
                  return is_ref_to_type(*n.first, RuleTypes);
                })) *
               (T(RefArgSeq)[RefArgSeq]))) >>
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
          Node datahead = RefHead << (Var ^ "data");
          std::string flat = flatten_ref(Ref << datahead << refargseq);
          Node flathead = RefHead << (Var ^ flat);
          return Ref << flathead << _(RefArgSeq);
        },
      
      In(Policy) * T(Import) >> [](Match&){
        return Node{};
      },
    };
  }
}