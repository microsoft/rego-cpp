#include "passes.h"

namespace rego
{

  PassDef merge_modules()
  {
    return {
      In(ModuleSeq) * (T(Module) << (T(Ident)[Id] * T(RuleSeq)[RuleSeq])) >>
        [](Match& _) {
          std::string key(_(Id)->location().view());
          return TopKeyValue << (Key ^ key) << (Module << *_[RuleSeq]);
        },

      T(ModuleSeq) << (T(TopKeyValue)++[TopKeyValue] * End) >>
        [](Match& _) { return Data << (TopKeyValueList << _[TopKeyValue]); },

      In(Policy) * (T(Data) << (T(Ident) * T(TopKeyValueList)[Lhs])) *
          (T(Data) << T(TopKeyValueList)[Rhs]) >>
        [](Match& _) {
          return Data << (Ident ^ "data") << merge_lists(_(Lhs), _(Rhs));
        },
    };
  }

}