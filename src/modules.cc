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

      // errors
      In(ModuleSeq) * T(File)[File] >>
        [](Match& _) { return err(_(File), "Invalid module file"); },

      In(Group) * T(Package)[Package] >>
        [](Match& _) {
          return err(_(Package), "Invalid package declaration.");
        },
    };
  }
}