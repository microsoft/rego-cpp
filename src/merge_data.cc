#include "passes.h"

namespace rego
{
  PassDef merge_data()
  {
    return {
      In(DataSeq) * (T(Data)[Lhs] * T(Data)[Rhs]) >>
        [](Match& _) {
          auto lhs = _(Lhs);
          auto rhs = _(Rhs);
          return Data << merge_lists(_(Lhs)->front(), _(Rhs)->front());
        },

      In(Policy) * (T(DataSeq) << (Start * T(Data)[Data] * End)) >>
        [](Match& _) { return Data << (Ident ^ "data") << *_[Data]; },

      In(Policy) * (T(DataSeq) << End) >>
        [](Match&) { return Data << (Ident ^ "data") << TopKeyValueList; },
    };
  }

}