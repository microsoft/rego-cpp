#include "lang.h"
#include "passes.h"

#include <sstream>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  void find_assigned_vars(const Node& node, Nodes& vars)
  {
    if (node->type() == Var)
    {
      Nodes defs = node->lookup();
      if (defs.size() == 0)
      {
        vars.push_back(node->clone());
      }
    }
    else
    {
      for (auto& child : *node)
      {
        find_assigned_vars(child, vars);
      }
    }
  }

  // clang-format off
  inline const auto wfi =
      (Top <<= Rego)
    | (Else <<= Expr * UnifyBody)
    ;
  // clang-format on
}

namespace rego
{
  const inline auto LocalToken = T(RefTerm) / T(Term);
  PassDef symbols()
  {
    return {
      In(Query) * (T(Literal)[Head] * T(Literal)++[Tail]) >>
        [](Match& _) { return UnifyBody << _(Head) << _[Tail]; },

      In(ModuleSeq) *
          (T(Module)
           << ((T(Package) << T(Var)[Id]) * T(ImportSeq)[ImportSeq] *
               T(Policy)[Policy])) >>
        [](Match& _) { return Module << _(Id) << _(ImportSeq) << _(Policy); },

      In(Policy) *
          (T(Rule)
           << ((T(RuleHead)
                << (T(Var)[Id] *
                    (T(RuleHeadComp) << (T(AssignOperator) * T(Expr)[Expr])))) *
               (T(Empty) / T(UnifyBody))[Body] * T(ElseSeq)[ElseSeq])) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          seq->push_back(
            RuleComp << _(Id) << _(Body) << _(Expr) << (JSONInt ^ "0"));
          Node elseseq = _(ElseSeq);
          for (std::size_t i = 0; i < elseseq->size(); ++i)
          {
            Node expr = elseseq->at(i)->at(wfi / Else / Expr);
            Node body = elseseq->at(i)->at(wfi / Else / UnifyBody);
            seq->push_back(
              RuleComp << _(Id)->clone() << body << expr
                       << (JSONInt ^ std::to_string(i + 1)));
          }

          return seq;
        },

      In(Policy) *
          (T(Rule)
           << ((T(RuleHead)
                << (T(Var)[Id] *
                    (T(RuleHeadFunc)
                     << (T(RuleArgs)[RuleArgs] * T(AssignOperator) *
                         T(Expr)[Expr])))) *
               (T(Empty) / T(UnifyBody))[Body] * T(ElseSeq)[ElseSeq])) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          seq->push_back(
            RuleFunc << _(Id) << _(RuleArgs) << _(Body) << _(Expr)
                     << (JSONInt ^ "0"));
          Node elseseq = _(ElseSeq);
          for (std::size_t i = 0; i < elseseq->size(); ++i)
          {
            Node expr = elseseq->at(i)->at(wfi / Else / Expr);
            Node body = elseseq->at(i)->at(wfi / Else / UnifyBody);
            seq->push_back(
              RuleFunc << _(Id)->clone() << _(RuleArgs)->clone() << body << expr
                       << (JSONInt ^ std::to_string(i + 1)));
          }

          return seq;
        },

      In(Policy) *
          (T(Rule)
           << ((T(RuleHead)
                << (T(Var)[Id] * (T(RuleHeadSet) << T(Expr)[Expr]))) *
               (T(Empty) / T(UnifyBody))[Body] * T(ElseSeq))) >>
        [](Match& _) { return RuleSet << _(Id) << _(Body) << _(Expr); },

      In(Policy) *
          (T(Rule)
           << ((T(RuleHead)
                << (T(Var)[Id] *
                    (T(RuleHeadObj)
                     << (T(Expr)[Key] * T(AssignOperator) * T(Expr)[Val])))) *
               (T(Empty) / T(UnifyBody))[Body] * T(ElseSeq))) >>
        [](Match& _) {
          return RuleObj << _(Id) << _(Body) << _(Key) << _(Val);
        },

      In(RuleArgs) * (T(Term) << T(Var)[Var]) >>
        [](Match& _) { return ArgVar << _(Var) << Undefined; },

      In(RuleArgs) *
          (T(Term) << (T(Scalar) / T(Array) / T(Object) / T(Set))[Val]) >>
        [](Match& _) { return ArgVal << _(Val); },

      In(ObjectItem) * ((T(ObjectItemHead) << T(Scalar)[Scalar])) >>
        [](Match& _) {
          std::string key = to_json(_(Scalar));
          if (key.starts_with('"'))
          {
            key = key.substr(1, key.size() - 2);
          }

          return Key ^ key;
        },

      In(Object) *
          (T(ObjectItem)
           << ((T(ObjectItemHead) << T(Var)[Var]) * T(Expr)[Expr])) >>
        [](Match& _) {
          return RefObjectItem << (RefTerm << _(Var)) << _(Expr);
        },

      In(Object) *
          (T(ObjectItem)
           << ((T(ObjectItemHead) << T(Ref)[Ref]) * T(Expr)[Expr])) >>
        [](Match& _) {
          return RefObjectItem << (RefTerm << _(Ref)) << _(Expr);
        },

      In(Expr) * (T(Expr) << (T(Term)[Term] * End)) >>
        [](Match& _) { return _(Term); },

      In(Expr) * (T(Term) << (T(Ref) / T(Var))[Val]) >>
        [](Match& _) { return RefTerm << _(Val); },

      In(Expr) * (T(Term) << (T(Scalar) << (T(JSONInt) / T(JSONFloat))[Val])) >>
        [](Match& _) { return NumTerm << _(Val); },

      In(RefArgBrack) * T(Var)[Var] >>
        [](Match& _) { return RefTerm << _(Var); },

      In(UnifyBody) *
          (T(Literal)
           << (T(SomeDecl)
               << (T(VarSeq)[VarSeq] * (T(InSome) << T(Undefined))))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          for (auto& var : *_(VarSeq))
          {
            seq->push_back(Local << var << Undefined);
          }
          return seq;
        },

      In(UnifyBody) *
          (T(Literal)
           << (T(SomeDecl)
               << ((T(VarSeq) << (T(Var)[Val] * End)) *
                   (T(InSome) << T(Expr)[Expr])))) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          return Seq << (Local << _(Val) << Undefined)
                     << (Local << (Var ^ item) << Undefined)
                     << (Literal
                         << (Expr << (RefTerm << (Var ^ item)) << Unify
                                  << (Enumerate << _(Expr))))
                     << (Literal
                         << (Expr
                             << (RefTerm << _(Val)->clone()) << Unify
                             << (RefTerm
                                 << (Ref << (RefHead << (Var ^ item))
                                         << (RefArgSeq
                                             << (RefArgBrack
                                                 << (Scalar
                                                     << (JSONInt ^ "1"))))))));
        },

      In(UnifyBody) *
          (T(Literal)
           << (T(SomeDecl)
               << ((T(VarSeq) << (T(Var)[Idx] * T(Var)[Val] * End)) *
                   (T(InSome) << T(Expr)[Expr])))) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          return Seq << (Local << _(Idx) << Undefined)
                     << (Local << _(Val) << Undefined)
                     << (Local << (Var ^ item) << Undefined)
                     << (Literal
                         << (Expr << (RefTerm << (Var ^ item)) << Unify
                                  << (Enumerate << _(Expr))))
                     << (Literal
                         << (Expr
                             << (RefTerm << _(Idx)->clone()) << Unify
                             << (RefTerm
                                 << (Ref << (RefHead << (Var ^ item))
                                         << (RefArgSeq
                                             << (RefArgBrack
                                                 << (Scalar
                                                     << (JSONInt ^ "0"))))))))
                     << (Literal
                         << (Expr
                             << (RefTerm << _(Val)->clone()) << Unify
                             << (RefTerm
                                 << (Ref << (RefHead << (Var ^ item))
                                         << (RefArgSeq
                                             << (RefArgBrack
                                                 << (Scalar
                                                     << (JSONInt ^ "1"))))))));
        },

      In(UnifyBody) *
          (T(Literal)
           << (T(Expr) << (LocalToken++[Head] * T(Assign) * Any++[Tail]))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Nodes vars;
          for (auto& node : *_(Head))
          {
            find_assigned_vars(node, vars);
          }

          for (auto& var : vars)
          {
            seq->push_back(Local << var << Undefined);
          }

          seq->push_back(Literal << (Expr << _[Head] << Unify << _[Tail]));
          return seq;
        },

      // errors

      In(UnifyBody) * (T(RuleHead)[RuleHead] << (T(Var) * T(RuleHeadFunc))) >>
        [](Match& _) {
          return err(_(RuleHead), "No rule functions allowed in rule bodies");
        },

      In(ObjectItem) * T(ObjectItemHead)[ObjectItemHead] >>
        [](Match& _) { return err(_(ObjectItemHead), "Invalid object key"); },

      In(Term) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Invalid ref term"); },

      In(Term) * T(Var)[Var] >>
        [](Match& _) { return err(_(Var), "Invalid var term"); },

      In(RuleArgs) * T(Term)[Term] >>
        [](Match& _) { return err(_(Term), "Invalid rule function argument"); },

      In(Literal) * T(SomeDecl)[SomeDecl] >>
        [](Match& _) { return err(_(SomeDecl), "Invalid some declaration"); },

      In(Expr) * T(Assign)[Assign] >>
        [](Match& _) { return err(_(Assign), "Invalid assignment"); },

      T(UnifyBody)[UnifyBody] << End >>
        [](Match& _) { return err(_(UnifyBody), "Invalid unification body"); },

      In(RuleFunc) * T(Empty)[Empty] >>
        [](Match& _) {
          return err(_(Empty), "RuleFunc cannot have an empty body");
        },
      
      In(Expr) * T(Dot)[Dot] >>
        [](Match& _) { return err(_(Dot), "Invalid dot expression"); },
    };
  }

}