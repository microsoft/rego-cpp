#include "passes.h"

namespace rego
{
  PassDef lists()
  {
    return {
      T(Brace) << (T(KeyValue)[KeyValue] * T(KeyValue)++[Tail]) >>
        [](Match& _) {
          return Brace << (KeyValueList << _(KeyValue) << _[Tail]);
        },

      T(Square) << (T(Group)[Group] * T(Group)++[Tail]) >>
        [](Match& _) { return Square << (SquareList << _(Group) << _[Tail]); },

      T(Brace) << (BraceListItem[Head] * BraceListItem++[Tail]) >>
        [](Match& _) { return Brace << (BraceList << _(Head) << _[Tail]); },

      T(Brace) << End >> [](Match&) { return Brace << KeyValueList; },

      T(Square) << End >> [](Match&) { return Square << SquareList; },
    };
  }
}