#include "passes.h"
#include "trieste/rewrite.h"

namespace
{
  using namespace rego;

  const auto inline MembershipToken = ScalarToken / T(JSONString) /
    T(RawString) / T(Var) / T(Object) / T(Array) / T(Set) / T(Dot) / T(Paren) /
    ArithToken / BoolToken;

  const auto inline GroundToken = ScalarToken / T(JSONString) / T(RawString) / T(ExprCall);
}

namespace rego
{
  PassDef membership()
  {
    return {
      In(Group) * (T(IsIn) * (T(UnifyBody) << (T(Group)[Group] * End))) >>
        [](Match& _) { return Seq << IsIn << (Set << _(Group)); },

      In(List) *
          ((T(Group) << (MembershipToken++[Idx] * End)) *
           (T(Group) << (MembershipToken++[Item] * T(IsIn) *
                         MembershipToken++[ItemSeq] * T(IsIn) *
                         MembershipToken++[ItemSeq1]) *
              End)) >>
        [](Match& _) {
          return Group << (Paren << (Group << _[Idx])
                                 << (Group << _[Item] << IsIn << _[ItemSeq]))
                       << IsIn << _[ItemSeq1];
        },

      In(Paren) * (T(List) << (T(Group)[Group] * End)) >>
        [](Match& _) { return _(Group); },

      In(Group) *
          (MembershipToken++[Item] * T(IsIn) * MembershipToken++[ItemSeq] *
           T(IsIn) * MembershipToken++[ItemSeq1]) >>
        [](Match& _) {
          return Seq << (Paren << (Group << _[Item] << IsIn << _[ItemSeq]))
                     << IsIn << _[ItemSeq1];
        },

      (In(Paren) / In(List) / In(UnifyBody)) *
          ((T(Group) << (MembershipToken++[Idx] * End)) *
           (T(Group)
            << (MembershipToken++[Item] * T(IsIn)[IsIn] *
                MembershipToken++[ItemSeq] * End))) >>
        [](Match& _) {
          return Group
            << (Membership << (Group << _[Idx]) << (Group << _[Item])
                           << (Group << _[ItemSeq]));
        },

      (In(Paren) / In(List) / In(UnifyBody)) *
          ((T(Group)
            << (MembershipToken++[Lhs] * (T(Assign) / T(Unify))[Assign] *
                MembershipToken[Head] * MembershipToken++[Tail] * End)) *
           (T(Group)
            << (MembershipToken[Head1] * MembershipToken++[Tail1] *
                T(IsIn)[IsIn] * MembershipToken[Head2] *
                MembershipToken++[Tail2] * End)))[Membership] >>
        [](Match& _) {
          return Group << _[Lhs] << _(Assign)
                       << (Membership << (Group << _(Head) << _[Tail])
                                      << (Group << _(Head1) << _[Tail1])
                                      << (Group << _(Head2) << _[Tail2]));
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
    };
  }
}