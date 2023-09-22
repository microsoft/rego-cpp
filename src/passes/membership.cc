#include "internal.hh"

namespace
{
  using namespace rego;

  const auto inline GroundToken =
    ScalarToken / T(JSONString) / T(RawString) / T(ExprCall);
}

namespace rego
{
  // Assembles tokens into Membership nodes.
  PassDef membership()
  {
    return {
      In(Group) * (T(IsIn) * (T(UnifyBody) << (T(Group)[Group] * End))) >>
        [](Match& _) {
          ACTION();
          return Seq << IsIn << (Set << _(Group));
        },

      In(Group) *
          (MembershipToken++[Item] * T(IsIn) * MembershipToken++[ItemSeq] *
           T(IsIn) * MembershipToken++[ItemSeq1]) >>
        [](Match& _) {
          ACTION();
          return Seq << (Paren << (Group << _[Item] << IsIn << _[ItemSeq]))
                     << IsIn << _[ItemSeq1];
        },

      In(Group) *
          (MembershipToken++[Idx] * T(Comma) * MembershipToken++[Item] *
           T(IsIn) * MembershipToken++[ItemSeq] * End) >>
        [](Match& _) {
          ACTION();
          return Membership << (Group << _[Idx]) << (Group << _[Item])
                            << (Group << _[ItemSeq]);
        },

      In(Group) *
          (MembershipToken++[Lhs] * (T(Assign) / T(Unify))[Assign] *
           MembershipToken++[Idx] * T(Comma) * MembershipToken++[Item] *
           T(IsIn) * MembershipToken++[ItemSeq] * End) >>
        [](Match& _) {
          ACTION();
          return Seq << _[Lhs] << _(Assign)
                     << (Membership << (Group << _[Idx]) << (Group << _[Item])
                                    << (Group << _[ItemSeq]));
        },

      In(Group) *
          (MembershipToken[Head] * MembershipToken++[Tail] * T(IsIn)[IsIn] *
           MembershipToken[Head1] * MembershipToken++[Tail1] *
           End)[Membership] >>
        [](Match& _) {
          ACTION();
          return Membership << Undefined << (Group << _(Head) << _[Tail])
                            << (Group << _(Head1) << _[Tail1]);
        },

      In(Group) *
          (MembershipToken++[Lhs] * (T(Assign) / T(Unify))[Assign] *
           MembershipToken[Head] * MembershipToken++[Tail] * T(IsIn)[IsIn] *
           MembershipToken[Head1] * MembershipToken++[Tail1] *
           End)[Membership] >>
        [](Match& _) {
          ACTION();
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
          ACTION();
          return Membership << Undefined << (Group << _(Item))
                            << (Group << _[ItemSeq]);
        },

      // errors
      In(Group) * T(Comma)[Comma] >>
        [](Match& _) {
          ACTION();
          return err(_(Comma), "invalid membership statement");
        },

      T(Group)[Group] << End >>
        [](Match& _) {
          ACTION();
          return err(_(Group), "Syntax error: empty group.");
        },
    };
  }
}