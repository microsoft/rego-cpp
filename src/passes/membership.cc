#include "errors.h"
#include "passes.h"
#include "utils.h"

namespace
{
  using namespace rego;

  const auto inline GroundToken =
    ScalarToken / T(JSONString) / T(RawString) / T(ExprCall);
}

namespace rego
{
  PassDef membership()
  {
    return {
      In(Group) * (T(IsIn) * (T(UnifyBody) << (T(Group)[Group] * End))) >>
        [](Match& _) { return Seq << IsIn << (Set << _(Group)); },

      In(Group) *
          (MembershipToken++[Item] * T(IsIn) * MembershipToken++[ItemSeq] *
           T(IsIn) * MembershipToken++[ItemSeq1]) >>
        [](Match& _) {
          return Seq << (Paren << (Group << _[Item] << IsIn << _[ItemSeq]))
                     << IsIn << _[ItemSeq1];
        },

      In(Group) *
          (MembershipToken++[Idx] * T(Comma) * MembershipToken++[Item] *
           T(IsIn) * MembershipToken++[ItemSeq] * End) >>
        [](Match& _) {
          return Membership << (Group << _[Idx]) << (Group << _[Item])
                            << (Group << _[ItemSeq]);
        },

      In(Group) *
          (MembershipToken++[Lhs] * (T(Assign) / T(Unify))[Assign] *
           MembershipToken++[Idx] * T(Comma) * MembershipToken++[Item] *
           T(IsIn) * MembershipToken++[ItemSeq] * End) >>
        [](Match& _) {
          return Seq << _[Lhs] << _(Assign)
                     << (Membership << (Group << _[Idx]) << (Group << _[Item])
                                    << (Group << _[ItemSeq]));
        },

      In(Group) *
          (MembershipToken[Head] * MembershipToken++[Tail] * T(IsIn)[IsIn] *
           MembershipToken[Head1] * MembershipToken++[Tail1] *
           End)[Membership] >>
        [](Match& _) {
          return Membership << Undefined << (Group << _(Head) << _[Tail])
                            << (Group << _(Head1) << _[Tail1]);
        },

      In(Group) *
          (MembershipToken++[Lhs] * (T(Assign) / T(Unify))[Assign] *
           MembershipToken[Head] * MembershipToken++[Tail] * T(IsIn)[IsIn] *
           MembershipToken[Head1] * MembershipToken++[Tail1] *
           End)[Membership] >>
        [](Match& _) {
          return Seq << _[Lhs] << _(Assign)
                     << (Membership << Undefined
                                    << (Group << _(Head) << _[Tail])
                                    << (Group << _(Head1) << _[Tail1]));
        },

      In(Group) *
          (T(SomeDecl)
           << ((T(VarSeq) << ((T(Group) << (GroundToken[Item] * End)) * End)) *
               (T(Group) << (T(IsIn) * MembershipToken++[ItemSeq] * End)))) >>
        [](Match& _) {
          return Membership << Undefined << (Group << _(Item))
                            << (Group << _[ItemSeq]);
        },

      // errors
      In(Group) * T(Comma)[Comma] >>
        [](Match& _) { return err(_(Comma), "invalid membership statement"); },

      T(Group)[Group] << End >>
        [](Match& _) { return err(_(Group), "Syntax error: empty group."); },
    };
  }
}