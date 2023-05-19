#include "passes.h"

namespace rego
{
  PassDef modules()
  {
    return {
      In(ModuleSeq) *
          (T(File)
           << ((T(Group) << (T(Package) * T(Var)[Id])) * T(Group)++[Tail])) >>
        [](Match& _) {
          return Module << (Package << _(Id)) << (Policy << _[Tail]);
        },

      In(List) *
          (T(Group) << (Any[Key] * T(Colon) * Any[Head] * Any++[Tail])) >>
        [](Match& _) {
          return ObjectItem << (Group << _(Key))
                            << (Group << _(Head) << _[Tail]);
        },

      In(Brace) *
          (T(Group) << (Any[Key] * T(Colon) * Any[Head] * Any++[Tail])) >>
        [](Match& _) {
          return List
            << (ObjectItem << (Group << _(Key))
                           << (Group << _(Head) << _[Tail]));
        },

      // errors
      In(ModuleSeq) * T(File)[File] >>
        [](Match& _) { return err(_(File), "Invalid module file"); },

      In(Group) * T(Package)[Package] >>
        [](Match& _) {
          return err(_(Package), "Invalid package declaration.");
        },

      In(Group) * T(Colon)[Colon] >>
        [](Match& _) { return err(_(Colon), "Invalid character"); }};
  }
}