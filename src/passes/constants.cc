#include "errors.h"
#include "passes.h"
#include "utils.h"

namespace rego
{
  // Converts all rules with constant terms to be DataTerm nodes.
  PassDef constants()
  {
    return {
      (In(RuleComp) / In(RuleFunc) / In(RuleSet) / In(DefaultRule)) *
          T(Term)[Term]([](auto& n) { return is_constant(*n.first); }) >>
        [](Match& _) { return DataTerm << *_[Term]; },

      (In(RuleComp) / In(RuleFunc)) *
          T(Term)[Term]([](auto& n) { return !is_constant(*n.first); }) >>
        [](Match& _) {
          Location value = _.fresh({"value"});
          return UnifyBody
            << (Literal
                << (Expr << (RefTerm << (Var ^ value)) << Unify << _(Term)));
        },

      In(RuleSet) *
          T(Term)[Term]([](auto& n) { return !is_constant(*n.first); }) >>
        [](Match& _) { return Expr << _(Term); },

      In(RuleObj) * (T(Term)[Key] * T(Term)[Val])([](auto& n) {
        Node key = n.first[0];
        Node val = n.first[1];
        return is_constant(key) && is_constant(val);
      }) >>
        [](Match& _) {
          std::string key = strip_quotes(to_json(_(Key)));
          return Seq << (DataTerm << *_[Key]) << (DataTerm << *_[Val]);
        },

      In(RuleObj) * (T(Term)[Key] * T(Term)[Val])([](auto& n) {
        Node key = n.first[0];
        Node val = n.first[1];
        return !is_constant(key) || !is_constant(val);
      }) >>
        [](Match& _) { return Seq << (Expr << _(Key)) << (Expr << _(Val)); },

      In(RuleObj) * (T(Expr)[Key] * T(Term)[Val]) >>
        [](Match& _) { return Seq << _(Key) << (Expr << _(Val)); },

      In(RuleObj) * (T(Term)[Key] * T(Expr)[Val]) >>
        [](Match& _) { return Seq << (Expr << _(Key)) << _(Val); },

      In(DataTerm) * T(Array)[Array] >>
        [](Match& _) { return DataArray << *_[Array]; },

      In(DataTerm) * T(Set)[Set] >> [](Match& _) { return DataSet << *_[Set]; },

      In(DataTerm) * T(Object)[Object] >>
        [](Match& _) { return DataObject << *_[Object]; },

      (In(DataArray) / In(DataSet)) * (T(Expr) << T(Term)[Term]) >>
        [](Match& _) { return DataTerm << _(Term)->front(); },

      (In(DataArray) / In(DataSet)) * (T(Expr) << T(NumTerm)[NumTerm]) >>
        [](Match& _) { return DataTerm << (Scalar << _(NumTerm)->front()); },

      In(DataObject) *
          (T(ObjectItem)
           << ((T(Expr) << (T(Term) / T(NumTerm))[Key]) *
               (T(Expr) << (T(Term) / T(NumTerm))[Val]))) >>
        [](Match& _) {
          Node key = _(Key);
          if (key->type() == NumTerm)
          {
            key = Scalar << key->front();
          }
          else
          {
            key = key->front();
          }

          Node val = _(Val);
          if (val->type() == NumTerm)
          {
            val = Scalar << val->front();
          }
          else
          {
            val = val->front();
          }

          return DataObjectItem << (DataTerm << key) << (DataTerm << val);
        },

      In(Expr) *
          (T(ExprEvery)([](auto& n) { return is_in(*n.first, {Policy}) && is_in(*n.first, {UnifyBody}); })
           << ((T(VarSeq) << (T(Var)[Val] * End)) * T(UnifyBody)[UnifyBody] *
               (T(IsIn) << T(Expr)[Expr]))) >>
        [](Match& _) {
          // lifts this every statement into a rule function
          Location item = _.fresh({"item"});
          Location every = _.fresh({"every"});

          Node undefbody = UnifyBody
            << (Literal << (Expr << Not << (RefTerm << _(Val)->clone())));
          Node val = Expr
            << (RefTerm
                << (Ref << (RefHead << (Var ^ item))
                        << (RefArgSeq
                            << (RefArgBrack << (Scalar << (JSONInt ^ "1"))))));
          return Seq
            << (Lift << Policy
                     << (RuleFunc
                         << (Var ^ every)
                         << (RuleArgs << (ArgVar << _(Val) << Undefined))
                         << _(UnifyBody)
                         << (DataTerm << (Scalar << (JSONTrue ^ "true")))
                         << (JSONInt ^ "0")))
            << (Lift << Policy
                     << (RuleFunc
                         << (Var ^ every)
                         << (RuleArgs
                             << (ArgVar << _(Val)->clone() << Undefined))
                         << undefbody
                         << (DataTerm << (Scalar << (JSONTrue ^ "true")))
                         << (JSONInt ^ "1")))
            << (Lift << UnifyBody << (Local << (Var ^ item) << Undefined))
            << (Lift << UnifyBody << (LiteralEnum << (Var ^ item) << _(Expr)))
            << (ExprCall << (RuleRef << (Var ^ every)) << (ArgSeq << val));
        },

      In(Expr) *
          (T(ExprEvery)([](auto& n) { return is_in(*n.first, {Policy}) && is_in(*n.first, {UnifyBody}); })
           << ((T(VarSeq) << (T(Var)[Idx] * T(Var)[Val] * End)) *
               T(UnifyBody)[UnifyBody] * (T(IsIn) << T(Expr)[Expr]))) >>
        [](Match& _) {
          // lifts this every statement into a rule function
          Location item = _.fresh({"item"});
          Location every = _.fresh({"every"});

          Node undefbody = UnifyBody
            << (Literal << (Expr << Not << (RefTerm << _(Val)->clone())));
          Node idx = Expr
            << (RefTerm
                << (Ref << (RefHead << (Var ^ item))
                        << (RefArgSeq
                            << (RefArgBrack << (Scalar << (JSONInt ^ "0"))))));
          Node val = Expr
            << (RefTerm
                << (Ref << (RefHead << (Var ^ item))
                        << (RefArgSeq
                            << (RefArgBrack << (Scalar << (JSONInt ^ "1"))))));
          return Seq
            << (Lift << Policy
                     << (RuleFunc
                         << (Var ^ every)
                         << (RuleArgs << (ArgVar << _(Idx) << Undefined)
                                      << (ArgVar << _(Val) << Undefined))
                         << _(UnifyBody)
                         << (DataTerm << (Scalar << (JSONTrue ^ "true")))
                         << (JSONInt ^ "0")))
            << (Lift << Policy
                     << (RuleFunc
                         << (Var ^ every)
                         << (RuleArgs
                             << (ArgVar << _(Idx)->clone() << Undefined)
                             << (ArgVar << _(Val)->clone() << Undefined))
                         << undefbody
                         << (DataTerm << (Scalar << (JSONTrue ^ "true")))
                         << (JSONInt ^ "1")))
            << (Lift << UnifyBody << (Local << (Var ^ item) << Undefined))
            << (Lift << UnifyBody << (LiteralEnum << (Var ^ item) << _(Expr)))
            << (ExprCall << (RuleRef << (Var ^ every))
                         << (ArgSeq << idx << val));
        },

      // errors

      In(Expr) * T(ExprEvery)[ExprEvery] >>
        [](Match& _) { return err(_(ExprEvery), "Invalid every expression"); },
    };
  }
}