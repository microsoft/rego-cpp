#include "passes.h"

namespace rego
{
  PassDef modules()
  {
    return {
      T(File)
          << (T(Group)
              << ((T(Package) << (T(Group) << T(Ident)[Id])) *
                  T(RuleSeq)[RuleSeq])) >>
        [](Match& _) { return Module << _(Id) << _(RuleSeq); },

      // errors
      In(ModuleSeq) * T(File)[File] >>
        [](Match& _) { return err(_(File), "Invalid module file"); },

      In(Group) * T(Package)[Package] >>
        [](Match& _) {
          return err(_(Package), "Invalid package declaration.");
        },

      In(Group) * T(RuleSeq)[RuleSeq] >>
        [](Match& _) {
          return err(_(RuleSeq), "Invalid rule sequence declaration.");
        },
    };
  }
}