#include "passes.h"

namespace rego
{
  PassDef query()
  {
    return {
      In(Top) * (T(Rego) << T(Query)[Query]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          for (auto& child : *_(Query))
          {
            seq->push_back(child);
          }
          return seq;
        },
    };
  }
}