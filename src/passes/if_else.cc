#include "passes.h"

namespace
{
  using namespace rego;

  const inline auto ExprTailToken = ExprToken / T(IsIn);
}

namespace rego
{
  // Handles the if keyword.
  PassDef ifs()
  {
    return {
      In(Group) * (T(If) * ExprToken[Head] * ExprTailToken++[Tail]) >>
        [](Match& _) { return UnifyBody << (Group << _(Head) << _[Tail]); },

      In(Group) * (T(If) * T(UnifyBody)[UnifyBody]) >>
        [](Match& _) { return _(UnifyBody); },

      // errors

      In(Group) * T(If)[If] >>
        [](Match& _) { return err(_(If), "Invalid if statement"); },
    };
  }

  // Creates Else nodes
  PassDef elses()
  {
    return {
      In(Group) *
          (T(Else) * (T(Assign) / T(Unify)) * ExprToken[Head] *
           ExprTailToken++[Tail] * T(UnifyBody)[UnifyBody]) >>
        [](Match& _) {
          return Else << (Group << _(Head) << _[Tail]) << _(UnifyBody);
        },

      In(Group) * (T(Else) * T(UnifyBody)[UnifyBody]) >>
        [](Match& _) { return Else << Undefined << _(UnifyBody); },

      In(Group) *
          (T(Else) * (T(Assign) / T(Unify)) * T(Group)[Group] *
           T(UnifyBody)[UnifyBody]) >>
        [](Match& _) { return Else << _(Group) << _(UnifyBody); },

      // errors
      In(Group) * (T(Else)[Else] << End) >>
        [](Match& _) { return err(_(Else), "Invalid else statement"); },
    };
  }

}