#include "passes.h"

namespace rego
{
  PassDef query()
  {
    return {
      In(Top) * (T(Policy) << (T(Query) << Result[Rhs])) >>
        [](Match& _) { return _(Rhs); },
    };
  }
}