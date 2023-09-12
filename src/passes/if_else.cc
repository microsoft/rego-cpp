#include "internal.hh"

namespace
{
  using namespace rego;

  const inline auto ExprTailToken = ExprToken / T(IsIn) / T(Assign) / T(Unify);
}

namespace rego
{
  // Handles the if keyword.
  PassDef ifs()
  {
    return {
      In(Group) *
          (T(If) * (ExprToken / T(SomeDecl))[Head] * ExprTailToken++[Tail]) >>
        [](Match& _) {
          return Seq << If << (UnifyBody << (Group << _(Head) << _[Tail]));
        },

      // errors
    };
  }

  // Creates Else nodes
  PassDef elses()
  {
    return {
      In(Group) *
          (T(Else) * (T(Assign) / T(Unify)) * ExprToken[Head] *
           ExprTailToken++[Tail] * ~T(If) * T(UnifyBody)[UnifyBody]) >>
        [](Match& _) {
          return Else << (Group << _(Head) << _[Tail]) << _(UnifyBody);
        },

      In(Group) * (T(Else) * ~T(If) * T(UnifyBody)[UnifyBody]) >>
        [](Match& _) {
          return Else << (Group << (True ^ "true")) << _(UnifyBody);
        },

      In(Group) *
          (T(Else) * (T(Assign) / T(Unify)) * T(Group)[Group] * ~T(If) *
           T(UnifyBody)[UnifyBody]) >>
        [](Match& _) { return Else << _(Group) << _(UnifyBody); },

      In(Policy) * ((T(Group)[Lhs] << T(Var)) * (T(Group)[Rhs] << T(Else))) >>
        [](Match& _) { return Group << *_[Lhs] << *_[Rhs]; },

      In(Group) *
          (T(Else) * (T(Assign) / T(Unify)) * ExprToken[Head] *
           ExprTailToken++[Tail]) >>
        [](Match& _) { return Else << (Group << _(Head) << _[Tail]) << Empty; },

      // errors
      In(Group) * (T(Else)[Else] << End) >>
        [](Match& _) { return err(_(Else), "Invalid else statement"); },
    };
  }

}