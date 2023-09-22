#include "internal.hh"

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
          ACTION();
          return Module << (Package << (Group << _[Package])) << ImportSeq
                        << (Policy << _[Policy]);
        },

      In(List, Compr) *
          (T(Group) << (KeyToken++[Key] * T(Colon) * Any++[Val])) >>
        [](Match& _) {
          ACTION();
          return ObjectItem << (Group << _[Key]) << (Group << _[Val]);
        },

      In(Brace) * (T(Group) << (KeyToken++[Key] * T(Colon) * Any++[Val])) >>
        [](Match& _) {
          ACTION();
          return List << (ObjectItem << (Group << _[Key]) << (Group << _[Val]));
        },

      In(Policy) *
          ((T(Group) << (T(Import) * ImportToken++[Import] * End)) *
           (T(Group) << (T(As) * T(Var)[Var] * End))) >>
        [](Match& _) {
          ACTION();
          return Lift << Module
                      << (Import << (Group << _[Import] << As << _(Var)));
        },

      In(Policy) * (T(Group) << (T(Import) * ImportToken++[Import] * End)) >>
        [](Match& _) {
          ACTION();
          return Lift << Module << (Import << (Group << _[Import]));
        },

      In(Module) * (T(ImportSeq)[ImportSeq] * T(Import)[Import]) >>
        [](Match& _) {
          ACTION();
          return ImportSeq << *_[ImportSeq] << _(Import);
        },

      T(Placeholder) >>
        [](Match& _) {
          ACTION();
          Location temp = _.fresh({"_"});
          return (Var ^ temp);
        },

      // errors
      In(ModuleSeq) * T(File)[File] >>
        [](Match& _) {
          ACTION();
          return err(_(File), "Invalid module file");
        },

      In(Group) * T(Package)[Package] >>
        [](Match& _) {
          ACTION();
          return err(_(Package), "Invalid package declaration.");
        },

      In(Group) * T(Colon)[Colon] >>
        [](Match& _) {
          ACTION();
          return err(_(Colon), "Invalid character");
        },

      In(ObjectItem) * (T(Group)[Group] << End) >>
        [](Match& _) {
          ACTION();
          return err(_(Group), "Invalid key/value pair");
        },

      In(Group) * (T(Import)[Import] << End) >>
        [](Match& _) {
          ACTION();
          return err(_(Import), "Invalid import");
        },

      In(Import, Package) * (T(Group)[Group] << End) >>
        [](Match& _) {
          ACTION();
          return err(_(Group), "Invalid import");
        },
    };
  }
}