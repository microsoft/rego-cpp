#include "lang.h"
#include "passes.h"
#include "trieste/token.h"

namespace
{
  using namespace rego;

  bool contains_or(const Node& group)
  {
    for (Node child : *group)
    {
      if (child->type() == Or)
      {
        return true;
      }
    }

    return false;
  }

  bool contains(const Node& node, const std::set<Token>& tokens)
  {
    if (node->type() == Error)
    {
      return false;
    }

    if (tokens.contains(node->type()))
    {
      return true;
    }

    for (Node child : *node)
    {
      if (contains(child, tokens))
      {
        return true;
      }
    }

    return false;
  }
}

namespace rego
{
  // Processes lists into more semantically meaningful structures.
  PassDef lists()
  {
    return {
      (In(Input) / In(Data)) * (T(Brace) << T(List)[List]) >>
        [](Match& _) { return ObjectItemSeq << *_[List]; },

      (In(Input) / In(Data)) * (T(Brace) << End) >>
        ([](Match&) -> Node { return ObjectItemSeq; }),

      In(Group) * (T(Var)[Var] * T(IfTruthy) * T(Brace)[Brace]) >>
        [](Match& _) {
          return Seq << _(Var) << IfTruthy << (UnifyBody << *_[Brace]);
        },

      In(Group) *
          ((T(Square) / T(Brace))[Compr] << (T(Group)[Group](
             [](auto& n) { return contains_or(*n.first); }))) >>
        [](Match& _) {
          // array/set comprehension
          Node group = NodeDef::create(Group);
          Node item = NodeDef::create(Undefined);
          for (auto& node : *_(Group))
          {
            if (node->type() == Or)
            {
              item = group;
              group = NodeDef::create(Group);
            }
            else
            {
              group->push_back(node);
            }
          }

          Node unifybody = NodeDef::create(UnifyBody);
          if (group->size() > 0)
          {
            unifybody->push_back(group);
          }
          for (auto it = _(Compr)->begin() + 1; it != _(Compr)->end(); ++it)
          {
            unifybody->push_back(*it);
          }

          if (_(Compr)->type() == Brace)
          {
            return SetCompr << item << unifybody;
          }

          return ArrayCompr << item << unifybody;
        },

      In(Group) *
          (T(Brace)[Compr]
           << ((T(List) << ((T(ObjectItem)
                             << (T(Group)[Key] * T(Group)[Val]([](auto& n) {
                                   return contains_or(*n.first);
                                 })))) *
                  End) *
               T(Group)++[UnifyBody] * End)) >>
        [](Match& _) {
          // object comprehension
          Node group = NodeDef::create(Group);
          Node val;
          for (auto& node : *_(Val))
          {
            if (node->type() == Or)
            {
              val = group;
              group = NodeDef::create(Group);
            }
            else
            {
              group->push_back(node);
            }
          }

          Node unifybody = NodeDef::create(UnifyBody);
          if (group->size() > 0)
          {
            unifybody->push_back(group);
          }

          unifybody << _[UnifyBody];

          return ObjectCompr << _(Key) << val << unifybody;
        },

      In(Group) *
          (T(Brace)
           << (T(List)
               << (T(ObjectItem)[Head] * T(ObjectItem)++[Tail] * End))) >>
        [](Match& _) { return Object << _(Head) << _[Tail]; },

      In(Group) * (T(Square) << (T(List)[List] * End)) >>
        [](Match& _) { return Array << *_[List]; },

      (In(Group) / In(ImportRef) / In(WithRef)) *
          (T(Square) << T(Group)[Group]) >>
        [](Match& _) { return Array << _(Group); },

      T(List)
          << ((T(Group) << (T(Every) * T(Var)[Key])) *
              (T(Group) << (T(Var)[Val] * Any[Head] * Any++[Tail]))) >>
        [](Match& _) {
          return Group
            << (ExprEvery << (VarSeq << (Group << _(Key)) << (Group << _(Val)))
                          << (EverySeq << (Group << _(Head) << _[Tail])));
        },

      In(Group) * (T(Every) * T(Var)[Var] * Any++[Tail]) >>
        [](Match& _) {
          return ExprEvery << (VarSeq << (Group << _(Var)))
                           << (EverySeq << (Group << _[Tail]));
        },

      In(Group) * T(UnifyBody)[UnifyBody]([](auto& n) {
        return is_in(*n.first, {ExprEvery});
      }) >>
        [](Match& _) { return Lift << ExprEvery << _(UnifyBody); },

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
            return SomeDecl << (VarSeq << *_[List]) << (Group << Undefined);
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
            return SomeDecl << (VarSeq << _(Group)) << (Group << Undefined);
          }
        },

      In(Group) * T(EmptySet) >> ([](Match&) -> Node { return Set; }),

      // errors

      (In(Input) / In(Data)) * T(Brace)[Brace] >>
        [](Match& _) { return err(_(Brace), "Invalid input/data body"); },

      In(Group) * T(Brace)[Brace] >>
        [](Match& _) { return err(_(Brace), "Invalid object"); },

      In(Group) * T(Square)[Square] >>
        [](Match& _) { return err(_(Square), "Invalid array"); },

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

      In(Group) * T(Every)[Every] >>
        [](Match& _) { return err(_(Every), "Invalid every"); },

      (In(ImportRef) / In(WithRef)) * T(Square)[Square] >>
        [](Match& _) { return err(_(Square), "Invalid reference"); },

      In(UnifyBody) * T(List)[List] >>
        [](Match& _) { return err(_(List), "Invalid block"); },

      In(EverySeq) * (T(Group)[Group] << End) >>
        [](Match& _) { return err(_(Group), "Invalid every sequence"); },

      In(ExprEvery) * (T(VarSeq) * T(EverySeq)[EverySeq]([](auto& n) {
                         return !contains(*n.first, {UnifyBody, Brace});
                       })) >>
        [](Match& _) { return err(_(EverySeq), "Missing body of every"); },

      (In(ArrayCompr) / In(SetCompr) / In(ObjectCompr)) *
          (T(Group)[Group] << End) >>
        [](Match& _) { return err(_(Group), "Invalid comprehension"); },
    };
  }
}