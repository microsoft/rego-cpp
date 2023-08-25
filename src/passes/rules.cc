#include "passes.h"
#include "errors.h"
#include "utils.h"


namespace
{
  using namespace rego;

  Node to_elseseq(const NodeRange& elses, const Node& init_value)
  {
    Node value = init_value;
    Node elseseq = NodeDef::create(ElseSeq);
    for (auto it = elses.first; it < elses.second; ++it)
    {
      Node n = *it;
      if (n->type() == UnifyBody)
      {
        n = Else << init_value->clone() << n;
      }
      else if (n->type() == Else)
      {
        Node else_value = n / Val;
        if (else_value->type() == Undefined)
        {
          n->replace(else_value, value->clone());
        }
        else
        {
          value = else_value;
        }
      }
      elseseq->push_back(n);
    }
    return elseseq;
  }
}

namespace rego
{
  PassDef rules()
  {
    return {
      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node value = Group << JSONTrue;
          return Rule << (RuleHead << _(Id)
                                   << (RuleHeadComp
                                       << (AssignOperator << Assign) << value))
                      << _(UnifyBody) << to_elseseq(_[Else], _(UnifyBody));
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Assign) / T(Unify)) * ExprToken[Head] *
               ExprToken++[Tail] * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node value = (Group << _(Head) << _[Tail]);
          return Rule << (RuleHead << _(Id)
                                   << (RuleHeadComp
                                       << (AssignOperator << Assign) << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Assign) / T(Unify)) * ExprToken[Head] *
               ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Group << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Assign) / T(Unify)) * T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          // a misclassified single-element set
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Group << (Set << *_[UnifyBody]))))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Paren)[Paren] * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node args = NodeDef::create(RuleArgs);
          Node paren = _(Paren);
          if (paren->front()->type() == List)
          {
            for (const auto& arg : *paren->front())
            {
              args->push_back(arg);
            }
          }
          else
          {
            args->push_back(paren->front());
          }

          Node value = Group << JSONTrue;
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Paren)[Paren] * (T(Assign) / T(Unify)) *
               ExprToken[Head] * ExprToken++[Tail] * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node args = NodeDef::create(RuleArgs);
          Node paren = _(Paren);
          if (paren->front()->type() == List)
          {
            for (const auto& arg : *paren->front())
            {
              args->push_back(arg);
            }
          }
          else
          {
            args->push_back(paren->front());
          }

          Node value = Group << _(Head) << _[Tail];
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Paren)[Paren] * (T(Assign) / T(Unify)) *
               ExprToken[Head] * ExprToken++[Tail] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node args = NodeDef::create(RuleArgs);
          Node paren = _(Paren);
          if (paren->front()->type() == List)
          {
            for (const auto& arg : *paren->front())
            {
              args->push_back(arg);
            }
          }
          else
          {
            args->push_back(paren->front());
          }

          Node value = Group << _(Head) << _[Tail];
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << value))
                      << Empty << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Contains) * ExprToken[Head] * ExprToken++[Tail] *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadSet << (Group << _(Head) << _[Tail])))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * T(Contains) * ExprToken[Head] *
               ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule << (RuleHead
                          << _(Id)
                          << (RuleHeadSet << (Group << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Item] * End)) *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << (RuleHead << _(Id) << (RuleHeadSet << _(Item)))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Item] * End)) * End)) >>
        [](Match& _) {
          return Rule << (RuleHead << _(Id) << (RuleHeadSet << _(Item)))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Key] * End)) *
               (T(Assign) / T(Unify)) * ExprToken[Head] * ExprToken++[Tail] *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << (RuleHead << _(Id)
                                   << (RuleHeadObj
                                       << _(Key) << (AssignOperator << Assign)
                                       << (Group << _(Head) << _[Tail])))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Key] * End)) *
               (T(Assign) / T(Unify)) * ExprToken[Head] * ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule << (RuleHead << _(Id)
                                   << (RuleHeadObj
                                       << _(Key) << (AssignOperator << Assign)
                                       << (Group << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Default) * T(Var)[Id] * (T(Assign) / T(Unify)) *
               ExprToken[Head] * ExprToken++[Tail])) >>
        [](Match& _) {
          return DefaultRule << _(Id) << (Group << _(Head) << _[Tail]);
        },

      // errors

      In(Policy) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid rule"); },

      In(Group) * T(Else)[Else] >>
        [](Match& _) { return err(_(Else), "Invalid else"); },

      In(Group) * T(Default)[Default] >>
        [](Match& _) { return err(_(Default), "Invalid default rule"); },
    };
  }
}