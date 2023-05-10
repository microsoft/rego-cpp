#include "passes.h"

namespace rego
{
  PassDef query()
  {
    return {
      In(Top) * (T(Rego) << (T(Query) << T(Term)[Term])) >>
        [](Match& _) { return _(Term); },
    };
  }
}