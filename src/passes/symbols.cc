#include "passes.h"

#include <sstream>

namespace
{
  using namespace rego;

  void find_assigned_vars(const Node& node, Nodes& vars)
  {
    if (node->type() == Var)
    {
      Nodes defs = node->lookup();
      if (defs.size() == 0)
      {
        vars.push_back(node->clone());
      }
    }
    else
    {
      for (auto& child : *node)
      {
        find_assigned_vars(child, vars);
      }
    }
  }
}

namespace rego
{
  const inline auto LocalToken = T(RefTerm) / T(Term);
  PassDef symbols()
  {
    return {
      In(Query) * (T(Literal)[Head] * T(Literal)++[Tail]) >>
        [](Match& _) { return UnifyBody << _(Head) << _[Tail]; },

      In(ModuleSeq) *
          (T(Module) << ((T(Package) << T(Var)[Id]) * T(Policy)[Policy])) >>
        [](Match& _) { return Module << _(Id) << _(Policy); },

      In(Policy) *
          (T(Rule)
           << ((T(RuleHead)
                << (T(Var)[Id] *
                    (T(RuleHeadComp) << (T(AssignOperator) * T(Expr)[Expr])))) *
               T(RuleBodySeq)[RuleBodySeq])) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          for (auto& rulebody : *_(RuleBodySeq))
          {
            seq->push_back(
              RuleComp << _(Id)->clone() << rulebody << _(Expr)->clone());
          }

          return seq;
        },

      In(Policy) *
          (T(Rule)
           << ((T(RuleHead)
                << (T(Var)[Id] *
                    (T(RuleHeadFunc)
                     << (T(RuleArgs)[RuleArgs] * T(AssignOperator) *
                         T(Expr)[Expr])))) *
               T(RuleBodySeq)[RuleBodySeq])) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          for (auto& rulebody : *_(RuleBodySeq))
          {
            seq->push_back(
              RuleFunc << _(Id)->clone() << _(RuleArgs)->clone() << rulebody
                       << _(Expr)->clone());
          }

          return seq;
        },

      In(RuleArgs) * (T(Term) << T(Var)[Var]) >>
        [](Match& _) { return ArgVar << _(Var) << Undefined; },

      In(RuleArgs) *
          (T(Term) << (T(Scalar) / T(Array) / T(Object) / T(Set))[Val]) >>
        [](Match& _) { return ArgVal << _(Val); },

      In(ObjectItem) * ((T(ObjectItemHead) << T(Scalar)[Scalar])) >>
        [](Match& _) {
          std::string key = to_json(_(Scalar));
          if (key.starts_with('"'))
          {
            key = key.substr(1, key.size() - 2);
          }

          return Key ^ key;
        },

      In(Object) *
          (T(ObjectItem)
           << ((T(ObjectItemHead) << T(Var)[Var]) * T(Expr)[Expr])) >>
        [](Match& _) {
          return RefObjectItem << (RefTerm << _(Var)) << _(Expr);
        },

      In(Object) *
          (T(ObjectItem)
           << ((T(ObjectItemHead) << T(Ref)[Ref]) * T(Expr)[Expr])) >>
        [](Match& _) {
          return RefObjectItem << (RefTerm << _(Ref)) << _(Expr);
        },

      In(Expr) * (T(Term) << (T(Ref) / T(Var))[Val]) >>
        [](Match& _) { return RefTerm << _(Val); },

      In(Expr) * (T(Term) << (T(Scalar) << (T(JSONInt) / T(JSONFloat))[Val])) >>
        [](Match& _) { return NumTerm << _(Val); },

      In(RefArgBrack) * T(Var)[Var] >>
        [](Match& _) { return RefTerm << _(Var); },

      In(UnifyBody) * (T(Literal) << T(SomeDecl)[SomeDecl]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          for (auto& var : *_(SomeDecl))
          {
            seq->push_back(Local << var << Undefined);
          }
          return seq;
        },

      In(UnifyBody) *
          (T(Literal)
           << (T(Expr) << (LocalToken++[Head] * T(Assign) * Any++[Tail]))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Nodes vars;
          for (auto& node : *_(Head))
          {
            find_assigned_vars(node, vars);
          }

          for (auto& var : vars)
          {
            seq->push_back(Local << var << Undefined);
          }

          seq->push_back(Literal << (Expr << _[Head] << Unify << _[Tail]));
          return seq;
        },

      // errors

      In(UnifyBody) * (T(RuleHead)[RuleHead] << (T(Var) * T(RuleHeadFunc))) >>
        [](Match& _) {
          return err(_(RuleHead), "No rule functions allowed in rule bodies");
        },

      In(ObjectItem) * T(ObjectItemHead)[ObjectItemHead] >>
        [](Match& _) { return err(_(ObjectItemHead), "Invalid object key"); },

      In(Term) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Invalid ref term"); },

      In(Term) * T(Var)[Var] >>
        [](Match& _) { return err(_(Var), "Invalid var term"); },

      In(RuleArgs) * T(Term)[Term] >>
        [](Match& _) { return err(_(Term), "Invalid rule function argument"); },

      In(Literal) * T(SomeDecl)[SomeDecl] >>
        [](Match& _) { return err(_(SomeDecl), "Invalid some declaration"); },

      In(Expr) * T(Assign)[Assign] >>
        [](Match& _) { return err(_(Assign), "Invalid assignment"); },

      T(UnifyBody)[UnifyBody] << End >>
        [](Match& _) { return err(_(UnifyBody), "Invalid unification body"); },

      In(RuleFunc) * T(Empty)[Empty] >>
        [](Match& _) { return err(_(Empty), "RuleFunc cannot have an empty body"); },
    };
  }

}