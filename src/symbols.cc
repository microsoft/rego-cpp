#include "passes.h"

namespace rego
{

  PassDef symbols()
  {
    return {
      In(ModuleSeq) *
          (T(Module) << ((T(Package) << T(Var)[Id]) * T(Policy)[Policy])) >>
        [](Match& _) { return Module << _(Id) << _(Policy); },

      T(Rule)
          << ((
                T(RuleHead)
                << (T(Var)[Id] *
                    (T(RuleHeadComp) << (T(AssignOperator) * T(Expr)[Expr])))) *
              T(RuleBodySeq)[RuleBodySeq]) >>
        [](Match& _) { return RuleComp << _(Id) << _(Expr) << _(RuleBodySeq); },

      In(ObjectItem) *
          (T(Scalar) << (T(String) << T(JSONString)[JSONString])) >>
        [](Match& _) {
          std::string key = std::string(_(JSONString)->location().view());
          key = key.substr(1, key.size() - 2);
          return Key ^ key;
        },

      In(Expr) * (T(Term) << (T(Ref) / T(Var))[Value]) >>
        [](Match& _) { return RefTerm << _(Value); },

      In(Expr) *
          (T(Term) << (T(Scalar) << (T(JSONInt) / T(JSONFloat))[Value])) >>
        [](Match& _) { return NumTerm << _(Value); },

      In(RefArgBrack) * T(Var)[Var] >>
        [](Match& _) { return RefTerm << _(Var); },

      // errors

      In(ObjectItem) * T(Scalar)[Scalar] >>
        [](Match& _) { return err(_(Scalar), "Invalid object key"); },
    };
  }

}