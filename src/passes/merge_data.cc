#include "passes.h"

namespace rego
{
  // Merges all the Data nodes in DataSeq into a single Data node.
  PassDef merge_data()
  {
    return {
      In(Input) * T(Object)[Object] >>
        [](Match& _) { return DataObject << *_[Object]; },

      In(Input) * T(Array)[Array] >>
        [](Match& _) { return DataArray << *_[Array]; },

      In(DataSeq) * (T(Data) << T(ObjectItemSeq)[Data]) >>
        [](Match& _) { return DataItemSeq << *_[Data]; },

      In(DataSeq) *
          (T(DataItemSeq)[Lhs] * (T(Data) << T(ObjectItemSeq)[Rhs])) >>
        [](Match& _) { return DataItemSeq << *_[Lhs] << *_[Rhs]; },

      In(DataItemSeq) *
          (T(ObjectItem)
           << ((T(Expr) << (T(Term) << T(Scalar)[Scalar])) *
               (T(Expr) << T(Term)[Term]))) >>
        [](Match& _) {
          std::string key = strip_quotes(to_json(_(Scalar)));
          return DataItem << (Key ^ key) << (DataTerm << _(Term)->front());
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

      In(Rego) * (T(DataSeq) << (T(DataItemSeq)[DataItemSeq] * End)) >>
        [](Match& _) { return Data << (Var ^ "data") << _(DataItemSeq); },

      In(Rego) * (T(DataSeq) << End) >>
        [](Match&) { return Data << (Var ^ "data") << DataItemSeq; },

      // errors

      In(DataItemSeq) * T(ObjectItem)[ObjectItem] >>
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
    };
  }

}