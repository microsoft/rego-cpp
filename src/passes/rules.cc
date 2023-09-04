#include "errors.h"
#include "helpers.h"
#include "passes.h"

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
  // Assembles tokens into a early form of what will become rule
  // definitions.
  PassDef rules()
  {
    return {
      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Item] * End)) *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << JSONFalse
                      << (RuleHead << (RuleRef << _(Id))
                                   << (RuleHeadSet << _(Item)))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Item] * End)) * T(If) *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          // this has the form of a set rule but the reference implementation
          // interprets it as an object rule instead.
          return Rule << JSONFalse
                      << (RuleHead << (RuleRef << _(Id))
                                   << (RuleHeadObj
                                       << _(Item) << (AssignOperator << Assign)
                                       << (Group << (JSONTrue ^ "true"))))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Item] * End)) * End)) >>
        [](Match& _) {
          return Rule << JSONFalse
                      << (RuleHead << (RuleRef << _(Id))
                                   << (RuleHeadSet << _(Item)))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (T(Var)[Id] * (T(Array) << (T(Group)[Key] * End)) *
               (T(Assign) / T(Unify)) * ExprToken[Head] * ExprToken++[Tail] *
               ~T(If) * T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << JSONFalse
                      << (RuleHead << (RuleRef << _(Id))
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
          return Rule << JSONFalse
                      << (RuleHead << (RuleRef << _(Id))
                                   << (RuleHeadObj
                                       << _(Key) << (AssignOperator << Assign)
                                       << (Group << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (RuleRefToken[RefHead] * RuleRefToken++[RefArgSeq] * ~T(If) *
               T(UnifyBody)[UnifyBody] * (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node value = Group << JSONTrue;
          return Rule << JSONFalse
                      << (RuleHead << (RuleRef << _(RefHead) << _[RefArgSeq])
                                   << (RuleHeadComp
                                       << (AssignOperator << Assign) << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (RuleRefToken[RefHead] * RuleRefToken++[RefArgSeq] *
               (T(Assign) / T(Unify)) * ExprToken[Head] * ExprToken++[Tail] *
               ~T(If) * T(UnifyBody)[UnifyBody] *
               (T(Else) / T(UnifyBody))++[Else])) >>
        [](Match& _) {
          Node value = (Group << _(Head) << _[Tail]);
          return Rule << JSONFalse
                      << (RuleHead << (RuleRef << _(RefHead) << _[RefArgSeq])
                                   << (RuleHeadComp
                                       << (AssignOperator << Assign) << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (~T(Default)[Default] * RuleRefToken[RefHead] *
               RuleRefToken++[RefArgSeq] * (T(Assign) / T(Unify)) * ~T(If) *
               ExprToken[Head] * ExprToken++[Tail])) >>
        [](Match& _) {
          Node is_default = _(Default) != nullptr ? JSONTrue : JSONFalse;
          return Rule << is_default
                      << (RuleHead
                          << (RuleRef << _(RefHead) << _[RefArgSeq])
                          << (RuleHeadComp << (AssignOperator << Assign)
                                           << (Group << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (~T(Default)[Default] * RuleRefToken[RefHead] *
               RuleRefToken++[RefArgSeq] * (T(Assign) / T(Unify)) *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          // a misclassified single-element set
          Node is_default = _(Default) != nullptr ? JSONTrue : JSONFalse;
          return Rule << is_default
                      << (RuleHead << (RuleRef << _(RefHead) << _[RefArgSeq])
                                   << (RuleHeadComp
                                       << (AssignOperator << Assign)
                                       << (Group << (Set << *_[UnifyBody]))))
                      << Empty << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (RuleRefToken[RefHead] * RuleRefToken++[RefArgSeq] *
               T(Paren)[Paren] * ~T(If) * T(UnifyBody)[UnifyBody] *
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
            Node arg = paren->front();
            if (arg->size() > 0)
            {
              args->push_back(arg);
            }
          }

          Node value = Group << JSONTrue;
          return Rule << JSONFalse
                      << (RuleHead
                          << (RuleRef << _(RefHead) << _[RefArgSeq])
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (RuleRefToken[RefHead] * RuleRefToken++[RefArgSeq] *
               T(Paren)[Paren] * (T(Assign) / T(Unify)) * ExprToken[Head] *
               ExprToken++[Tail] * ~T(If) * T(UnifyBody)[UnifyBody] *
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
            Node arg = paren->front();
            if (arg->size() > 0)
            {
              args->push_back(arg);
            }
          }

          Node value = Group << _(Head) << _[Tail];
          return Rule << JSONFalse
                      << (RuleHead
                          << (RuleRef << _(RefHead) << _[RefArgSeq])
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << value))
                      << _(UnifyBody) << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (~T(Default)[Default] * RuleRefToken[RefHead] *
               RuleRefToken++[RefArgSeq] * T(Paren)[Paren] *
               (T(Assign) / T(Unify)) * ExprToken[Head] * ExprToken++[Tail] *
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
            Node arg = paren->front();
            if (arg->size() > 0)
            {
              args->push_back(arg);
            }
          }

          Node is_default = _(Default) != nullptr ? JSONTrue : JSONFalse;
          Node value = Group << _(Head) << _[Tail];
          return Rule << is_default
                      << (RuleHead
                          << (RuleRef << _(RefHead) << _[RefArgSeq])
                          << (RuleHeadFunc << args << (AssignOperator << Assign)
                                           << value))
                      << Empty << to_elseseq(_[Else], value);
        },

      In(Policy) *
          (T(Group)
           << (RuleRefToken[RefHead] * RuleRefToken++[RefArgSeq] * T(Contains) *
               ExprToken[Head] * ExprToken++[Tail] * ~T(If) *
               T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          return Rule << JSONFalse
                      << (RuleHead
                          << (RuleRef << _(RefHead) << _[RefArgSeq])
                          << (RuleHeadSet << (Group << _(Head) << _[Tail])))
                      << _(UnifyBody) << ElseSeq;
        },

      In(Policy) *
          (T(Group)
           << (RuleRefToken[RefHead] * RuleRefToken++[RefArgSeq] * T(Contains) *
               ExprToken[Head] * ExprToken++[Tail])) >>
        [](Match& _) {
          return Rule << JSONFalse
                      << (RuleHead
                          << (RuleRef << _(RefHead) << _[RefArgSeq])
                          << (RuleHeadSet << (Group << _(Head) << _[Tail])))
                      << Empty << ElseSeq;
        },

      In(RuleRef) * (T(Group) << (RuleRefToken++[RuleRef] * End)) >>
        [](Match& _) { return Seq << _[RuleRef]; },

      // errors

      In(Policy) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid rule"); },

      In(Group) * T(Else)[Else] >>
        [](Match& _) { return err(_(Else), "Invalid else"); },

      In(Group) * T(Default)[Default] >>
        [](Match& _) { return err(_(Default), "Invalid default rule"); },

      In(Group) * T(If)[If] >>
        [](Match& _) { return err(_(If), "Invalid if"); },

      In(RuleRef) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid rule reference"); },

      In(With) * (T(RuleRef)[RuleRef] << (T(Group) << End)) >>
        [](Match& _) { return err(_(RuleRef), "Empty rule ref"); },
    };
  }
}