#include "lang.h"
#include "passes.h"
#include "trieste/token.h"

namespace rego
{
  PassDef lists()
  {
    return {
      (In(Input) / In(Data)) * (T(Brace) << T(List)[List]) >>
        [](Match& _) { return ObjectItemSeq << *_[List]; },

      (In(Input) / In(Data)) * (T(Brace) << End) >>
        ([](Match&) -> Node { return ObjectItemSeq; }),

      In(Group) *
          (T(Brace)
           << (T(List)
               << (T(ObjectItem)[Head] * T(ObjectItem)++[Tail] * End))) >>
        [](Match& _) { return Object << _(Head) << _[Tail]; },

      In(Group) * (T(Square) << (T(List)[List] * End)) >>
        [](Match& _) { return Array << *_[List]; },

      In(Group) * (T(Square) << T(Group)[Group]) >>
        [](Match& _) { return Array << _(Group); },

      In(Group) * (T(Brace) << (T(Group)[Head] * T(Group)++[Tail] * End)) >>
        [](Match& _) { return UnifyBody << _(Head) << _[Tail]; },

      In(Group) * (T(Brace) << (T(List)[List] * End)) >>
        [](Match& _) { return Set << *_[List]; },

      In(Group) * (T(Some) << (T(List)[List] * End)) >>
        [](Match& _) {
          Node list = _(List);
          if (list->size() == 0)
          {
            return err(list, "some must contain at least one variable");
          }

          Node last_group = list->back();
          if (last_group->size() > 1)
          {
            Node insome_group = NodeDef::create(Group);
            insome_group->insert(
              insome_group->end(), last_group->begin() + 1, last_group->end());
            list->back() = Group << last_group->front();
            Node varseq = NodeDef::create(VarSeq);
            varseq->insert(varseq->end(), list->begin(), list->end());
            return SomeDecl << varseq << insome_group;
          }
          else
          {
            return SomeDecl << (VarSeq << *_[List]) << (Group << Default);
          }
        },

      In(Group) * (T(Some) << T(Group)[Group]) >>
        [](Match& _) {
          Node group = _(Group);
          if (group->size() > 1)
          {
            Node insome_group = NodeDef::create(Group);
            insome_group->insert(
              insome_group->end(), group->begin() + 1, group->end());
            return SomeDecl << (VarSeq << (Group << group->front()))
                            << insome_group;
          }
          else
          {
            return SomeDecl << (VarSeq << _(Group)) << (Group << Default);
          }
        },

      In(Group) * T(EmptySet) >> ([](Match&) -> Node { return Set; }),

      // errors

      (In(Input) / In(Data)) * T(Brace)[Brace] >>
        [](Match& _) { return err(_(Brace), "Invalid input/data body"); },

      In(Group) * T(Brace)[Brace] >>
        [](Match& _) { return err(_(Brace), "Invalid object"); },

      In(Group) * T(Some)[Some] >>
        [](Match& _) { return err(_(Some), "Invalid some declaration"); },

      In(SomeDecl) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) {
          return err(_(ObjectItem), "Invalid object item in some-decl");
        },

      In(VarSeq) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) {
          return err(_(ObjectItem), "Invalid object item in var-seq");
        },

      In(Some) * (T(List) * Any[Rhs]) >>
        [](Match& _) {
          return err(_(Rhs), "Invalid second node in some declaration");
        },

      (In(ObjectItemSeq) / In(Object)) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid object key/value"); },

      (In(Array) / In(Set) / In(List)) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) { return err(_(ObjectItem), "Invalid item"); },

      In(Group) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Syntax error"); },
    };
  }
}