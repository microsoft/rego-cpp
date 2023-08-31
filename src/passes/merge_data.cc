#include "errors.h"
#include "passes.h"
#include "utils.h"

namespace rego
{
  // Merges all the Data nodes in DataSeq into a single Data node.
  PassDef merge_data()
  {
    return {
      In(Input) * T(Term)[DataTerm] >>
        [](Match& _) { return DataTerm << *_[DataTerm]; },

      In(DataSeq) * (T(Data) << T(ObjectItemSeq)[Data]) >>
        [](Match& _) { return DataModule << *_[Data]; },

      In(DataSeq) * (T(DataModule)[Lhs] * (T(Data) << T(ObjectItemSeq)[Rhs])) >>
        [](Match& _) { return DataModule << *_[Lhs] << *_[Rhs]; },

      In(DataModule) *
          (T(ObjectItem)
           << ((T(Expr) << (T(Term) << T(Scalar)[Scalar])) *
               (T(Expr) << (T(Term) << T(Object)[DataModule])))) >>
        [](Match& _) {
          std::string key = strip_quotes(to_json(_(Scalar)));
          return Submodule << (Key ^ key) << (DataModule << *_[DataModule]);
        },

      In(DataModule) *
          (T(ObjectItem)
           << ((T(Expr) << (T(Term) << T(Scalar)[Scalar])) *
               (T(Expr)
                << (T(Term) << (T(Array) / T(Set) / T(Scalar))[Term])))) >>
        [](Match& _) {
          std::string key = strip_quotes(to_json(_(Scalar)));
          return DataRule << (Var ^ key) << (DataTerm << _(Term));
        },

      In(DataObject) *
          (T(ObjectItem)
           << ((T(Expr) << T(Term)[Key]) * (T(Expr) << T(Term)[Val]))) >>
        [](Match& _) {
          return DataObjectItem << (DataTerm << _(Key)->front())
                                << (DataTerm << _(Val)->front());
        },

      In(DataTerm) * T(Array)[Array] >>
        [](Match& _) { return DataArray << *_[Array]; },

      In(DataTerm) * T(Set)[Set] >> [](Match& _) { return DataSet << *_[Set]; },

      In(DataTerm) * T(Object)[Object] >>
        [](Match& _) { return DataObject << *_[Object]; },

      (In(DataArray) / In(DataSet)) * (T(Expr) << T(Term)[Term]) >>
        [](Match& _) { return DataTerm << _(Term)->front(); },

      In(Rego) * (T(DataSeq) << (T(DataModule)[DataModule] * End)) >>
        [](Match& _) { return Data << (Key ^ "data") << _(DataModule); },

      In(Rego) * (T(DataSeq) << End) >>
        [](Match&) { return Data << (Key ^ "data") << DataModule; },

      In(RuleArgs) * (T(Term) << T(Var)[Var]) >>
        [](Match& _) { return ArgVar << _(Var) << Undefined; },

      In(RuleArgs) *
          (T(Term) << (T(Scalar) / T(Array) / T(Object) / T(Set))[Val]) >>
        [](Match& _) { return ArgVal << _(Val); },

      // errors

      In(DataModule) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) {
          return err(_(ObjectItem), "Syntax error: unexpected object item");
        },

      In(DataItem) * T(Expr)[Expr] >>
        [](Match& _) {
          return err(_(Expr), "Syntax error: unexpected expression");
        },

      In(DataTerm) * T(Var)[Var] >>
        [](Match& _) {
          return err(_(Var), "Syntax error: unexpected variable");
        },

      In(DataTerm) * (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr] >>
        [](Match& _) {
          return err(_(Compr), "Syntax error: unexpected comprehension");
        },

      (In(DataArray) / In(DataSet)) * T(Expr)[Expr] >>
        [](Match& _) {
          return err(_(Expr), "Syntax error: unexpected expression");
        },

      In(DataObject) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) {
          return err(_(ObjectItem), "Syntax error: unexpected object item");
        },

      In(DataTerm) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Syntax error: unexpected ref"); },

      In(RuleArgs) * T(Term)[Term] >>
        [](Match& _) { return err(_(Term), "Invalid rule function argument"); },
    };
  }

}