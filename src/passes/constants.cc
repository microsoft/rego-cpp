#include "passes.h"

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
          return UnifyBody << (Literal << (Expr << (RefTerm << (Var ^ value)) << Unify << _(Term))); },

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

      // errors

      In(DefaultRule) * T(Term)[Term] >>
        [](Match& _) {
          return err(_(Term), "Default rule values must be constant");
        }};
  }
}