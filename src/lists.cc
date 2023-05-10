#include "passes.h"

namespace rego
{
  PassDef lists()
  {
    return {
      T(Brace) << (T(ObjectItem)[ObjectItem] * T(ObjectItem)++[Tail]) >>
        [](Match& _) { return ObjectItemList << _(ObjectItem) << _[Tail]; },

      T(Square) << (T(Group)[Group] * T(Group)++[Tail]) >>
        [](Match& _) { return SquareList << _(Group) << _[Tail]; },

      T(Brace) << (T(Group)[Head] * T(Group)++[Tail]) >>
        [](Match& _) { return BraceList << _(Head) << _[Tail]; },

      T(Brace) << End >> ([](Match&) -> Node { return ObjectItemList; }),

      T(Square) << End >> ([](Match&) -> Node { return SquareList; }),

      // errors

      (In(Input) / In(Data)) * T(BraceList)[BraceList] >>
        [](Match& _) { return err(_(BraceList), "invalid object"); },
    };
  }
}