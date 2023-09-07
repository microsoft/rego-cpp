#include "errors.h"
#include "helpers.h"
#include "passes.h"

namespace
{
  using namespace rego;

  const inline auto KeyToken = T(Var) / T(Square) / T(Dot) / ScalarToken /
    T(RawString) / T(JSONString) / T(Brace);
  const inline auto RefToken =
    T(Var) / T(Dot) / T(Square) / T(RawString) / T(JSONString);
  const inline auto ImportToken = T(Var) / T(Dot) / T(Square) / T(As);
}

namespace rego
{
  // Separates out Modules from the rest of the AST.
  PassDef modules()
  {
    return {
      In(ModuleSeq) *
          (T(File)
           << ((T(Group) << (T(Package) * RefToken++[Package])) *
               T(Group)++[Policy])) >>
        [](Match& _) {
          return Module << (Package << (Group << _[Package])) << ImportSeq
                        << (Policy << _[Policy]);
        },

      In(List, Compr) *
          (T(Group) << (KeyToken++[Key] * T(Colon) * Any++[Val])) >>
        [](Match& _) {
          return ObjectItem << (Group << _[Key]) << (Group << _[Val]);
        },

      In(Brace) * (T(Group) << (KeyToken++[Key] * T(Colon) * Any++[Val])) >>
        [](Match& _) {
          return List << (ObjectItem << (Group << _[Key]) << (Group << _[Val]));
        },

      In(Policy) *
          ((T(Group) << (T(Import) * ImportToken++[Import] * End)) *
           (T(Group) << (T(As) * T(Var)[Var] * End))) >>
        [](Match& _) {
          return Lift << Module
                      << (Import << (Group << _[Import] << As << _(Var)));
        },

      In(Policy) * (T(Group) << (T(Import) * ImportToken++[Import] * End)) >>
        [](Match& _) {
          return Lift << Module << (Import << (Group << _[Import]));
        },

      In(Module) * (T(ImportSeq)[ImportSeq] * T(Import)[Import]) >>
        [](Match& _) { return ImportSeq << *_[ImportSeq] << _(Import); },

      In(ModuleSeq) * (T(Module)[Lhs] * T(Module)[Rhs])([](auto& n) {
        Node lhs = *n.first;
        Node rhs = *(n.first + 1);
        Node lhs_pkg = (lhs / Package / Group);
        Node rhs_pkg = (rhs / Package / Group);
        if (lhs_pkg->size() != rhs_pkg->size())
        {
          return false;
        }

        for (std::size_t i = 0; i < lhs_pkg->size(); ++i)
        {
          if (lhs_pkg->at(i)->location() != rhs_pkg->at(i)->location())
          {
            return false;
          }
        }

        return true;
      }) >>
        [](Match& _) {
          Node pkg = _(Lhs) / Package;
          Node imports = _(Lhs) / ImportSeq;
          Node policy = _(Lhs) / Policy;
          Node rhs_imports = _(Rhs) / ImportSeq;
          Node rhs_policy = _(Rhs) / Policy;
          imports->insert(
            imports->end(), rhs_imports->begin(), rhs_imports->end());
          policy->insert(policy->end(), rhs_policy->begin(), rhs_policy->end());
          return Module << pkg << imports << policy;
        },

      T(Placeholder) >>
        [](Match& _) {
          Location temp = _.fresh({"_"});
          return (Var ^ temp);
        },

      // errors
      In(ModuleSeq) * T(File)[File] >>
        [](Match& _) { return err(_(File), "Invalid module file"); },

      In(Group) * T(Package)[Package] >>
        [](Match& _) {
          return err(_(Package), "Invalid package declaration.");
        },

      In(Group) * T(Colon)[Colon] >>
        [](Match& _) { return err(_(Colon), "Invalid character"); },

      In(ObjectItem) * (T(Group)[Group] << End) >>
        [](Match& _) { return err(_(Group), "Invalid key/value pair"); },

      In(Group) * (T(Import)[Import] << End) >>
        [](Match& _) { return err(_(Import), "Invalid import"); },

      In(Import, Package) * (T(Group)[Group] << End) >>
        [](Match& _) { return err(_(Group), "Invalid import"); },
    };
  }
}