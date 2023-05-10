#include "passes.h"

namespace rego
{

  PassDef merge_modules()
  {
    return {
      In(ModuleSeq) * (T(Module) << (T(Var)[Id] * T(Policy)[Policy])) >>
        [](Match& _) { return DataModule << _(Id) << (Module << *_[Policy]); },

      T(ModuleSeq) << (T(DataModule)++[DataModule] * End) >>
        [](Match& _) { return DataModuleSeq << _[DataModule]; },

      In(Rego) * (T(Data) << (T(Var)[Id] * T(ObjectItemList)[ObjectItemList])) *
          T(DataModuleSeq)[DataModuleSeq] >>
        [](Match& _) {
          return Data << _(Id) << _(ObjectItemList) << _(DataModuleSeq);
        },
    };
  }

}