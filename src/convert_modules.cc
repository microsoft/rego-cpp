#include "passes.h"

namespace rego
{
  PassDef convert_modules()
  {
    return {
      In(Module) * (T(Rule) << (T(Ident)[Key] * Result[Value])) >>
        [](Match& _) {
          std::string key(_(Key)->location().view());
          return KeyValue << (Key ^ key) << _(Value);
        },

      In(TopKeyValue) *
          (T(Module) << (T(KeyValue)[Head] * T(KeyValue)++[Tail] * End)) >>
        [](Match& _) { return Object << _(Head) << _[Tail]; },

      In(TopKeyValue) * (T(Module) << End) >>
        [](Match&) { return Node(Object); },
    };
  }

}