#include "passes.h"

namespace rego
{
  const inline auto KeyToken =
    T(Var) / T(Square) / T(Dot) / ScalarToken / T(RawString) / T(JSONString);

  PassDef modules()
  {
    return {
      In(ModuleSeq) *
          (T(File)
           << ((T(Group) << (T(Package) * T(Var)[Id])) * T(Group)++[Tail])) >>
        [](Match& _) {
          return Module << (Package << _(Id)) << (Policy << _[Tail]);
        },

      In(List) * (T(Group) << (KeyToken++[Key] * T(Colon) * Any++[Val])) >>
        [](Match& _) {
          return ObjectItem << (Group << _[Key]) << (Group << _[Val]);
        },

      In(Brace) * (T(Group) << (KeyToken++[Key] * T(Colon) * Any++[Val])) >>
        [](Match& _) {
          return List << (ObjectItem << (Group << _[Key]) << (Group << _[Val]));
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
    };
  }
}