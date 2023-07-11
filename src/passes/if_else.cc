#include "passes.h"

namespace rego
{
  PassDef if_else()
  {
    return {
      In(Group) * (T(IfTruthy) * ExprToken[Head] * ExprToken++[Tail] * End) >>
        [](Match& _) { return UnifyBody << (Group << _(Head) << _[Tail]); },

      In(Group) *
          (T(IfTruthy) * ExprToken[Head] * ExprToken++[Tail] *
           (T(UnifyBody) / T(Else))[UnifyBody]) >>
        [](Match& _) {
          return Seq << (UnifyBody << (Group << _(Head) << _[Tail]))
                     << _(UnifyBody);
        },

      In(Group) * (T(IfTruthy) * T(UnifyBody)[UnifyBody]) >>
        [](Match& _) { return _(UnifyBody); },

      In(Group) *
          (T(Else) * (T(Assign) / T(Unify)) * ExprToken[Head] *
           ExprToken++[Tail] * T(UnifyBody)[UnifyBody]) >>
        [](Match& _) {
          return Else << (Group << _(Head) << _[Tail]) << _(UnifyBody);
        },

      In(Group) * (T(Else) * T(UnifyBody)[UnifyBody]) >>
        [](Match& _) { return Else << Undefined << _(UnifyBody); },

      // errors
      In(Group) * T(IfTruthy)[IfTruthy] >>
        [](Match& _) { return err(_(IfTruthy), "Invalid if"); },

      In(Group) * (T(Else) << End)[Else] >>
        [](Match& _) { return err(_(Else), "Invalid else"); },
    };
  }
}