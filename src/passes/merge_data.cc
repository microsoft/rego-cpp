#include "internal.hh"

namespace rego
{
  // Merges all the Data nodes in DataSeq into a single Data node with
  // hierarchical structure.
  PassDef merge_data()
  {
    return {
      "merge_data",
      wf_pass_merge_data,
      dir::topdown,
      {
        In(Input) * T(Term)[DataTerm] >>
          [](Match& _) {
            ACTION();
            return DataTerm << *_[DataTerm];
          },

        In(DataSeq) * (T(Data) << T(ObjectItemSeq)[Data]) >>
          [](Match& _) {
            ACTION();
            return DataModule << *_[Data];
          },

        In(DataSeq) *
            (T(DataModule)[Lhs] * (T(Data) << T(ObjectItemSeq)[Rhs])) >>
          [](Match& _) {
            ACTION();
            return DataModule << *_[Lhs] << *_[Rhs];
          },

        In(DataModule) *
            (T(ObjectItem)
             << ((T(Expr) << (T(Term) << T(Scalar)[Scalar])) *
                 (T(Expr) << (T(Term) << T(Object)[DataModule])))) >>
          [](Match& _) {
            ACTION();
            std::string key = strip_quotes(to_json(_(Scalar)));
            return Submodule << (Key ^ key) << (DataModule << *_[DataModule]);
          },

        In(DataModule) *
            (T(ObjectItem)
             << ((T(Expr) << (T(Term) << T(Scalar)[Scalar])) *
                 (T(Expr)
                  << (T(Term) << (T(Array) / T(Set) / T(Scalar))[Term])))) >>
          [](Match& _) {
            ACTION();
            std::string key = strip_quotes(to_json(_(Scalar)));
            return DataRule << (Var ^ key) << (DataTerm << _(Term));
          },

        In(DataObject) *
            (T(ObjectItem)
             << ((T(Expr) << T(Term)[Key]) * (T(Expr) << T(Term)[Val]))) >>
          [](Match& _) {
            ACTION();
            return DataObjectItem << (DataTerm << _(Key)->front())
                                  << (DataTerm << _(Val)->front());
          },

        In(DataTerm) * T(Array)[Array] >>
          [](Match& _) {
            ACTION();
            return DataArray << *_[Array];
          },

        In(DataTerm) * T(Set)[Set] >>
          [](Match& _) {
            ACTION();
            return DataSet << *_[Set];
          },

        In(DataTerm) * T(Object)[Object] >>
          [](Match& _) {
            ACTION();
            return DataObject << *_[Object];
          },

        In(DataArray, DataSet) * (T(Expr) << T(Term)[Term]) >>
          [](Match& _) {
            ACTION();
            return DataTerm << _(Term)->front();
          },

        In(Rego) * (T(DataSeq) << (T(DataModule)[DataModule] * End)) >>
          [](Match& _) {
            ACTION();
            return Data << (Key ^ "data") << _(DataModule);
          },

        In(Rego) * (T(DataSeq) << End) >>
          [](Match&) {
            ACTION();
            return Data << (Key ^ "data") << DataModule;
          },

        In(RuleArgs) * (T(Term) << T(Var)[Var]) >>
          [](Match& _) {
            ACTION();
            return ArgVar << _(Var) << Undefined;
          },

        In(RuleArgs) *
            (T(Term) << (T(Scalar) / T(Array) / T(Object) / T(Set))[Val]) >>
          [](Match& _) {
            ACTION();
            return ArgVal << _(Val);
          },

        // errors

        In(DataModule) * T(ObjectItem)[ObjectItem] >>
          [](Match& _) {
            ACTION();
            return err(_(ObjectItem), "Syntax error: unexpected object item");
          },

        In(DataItem) * T(Expr)[Expr] >>
          [](Match& _) {
            ACTION();
            return err(_(Expr), "Syntax error: unexpected expression");
          },

        In(DataTerm) * T(Var)[Var] >>
          [](Match& _) {
            ACTION();
            return err(_(Var), "Syntax error: unexpected variable");
          },

        In(DataTerm) * (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr] >>
          [](Match& _) {
            ACTION();
            return err(_(Compr), "Syntax error: unexpected comprehension");
          },

        In(DataArray, DataSet) * T(Expr)[Expr] >>
          [](Match& _) {
            ACTION();
            return err(_(Expr), "Syntax error: unexpected expression");
          },

        In(DataObject) * T(ObjectItem)[ObjectItem] >>
          [](Match& _) {
            ACTION();
            return err(_(ObjectItem), "Syntax error: unexpected object item");
          },

        In(DataTerm) * T(Ref)[Ref] >>
          [](Match& _) {
            ACTION();
            return err(_(Ref), "Syntax error: unexpected ref");
          },

        In(RuleArgs) * T(Term)[Term] >>
          [](Match& _) {
            ACTION();
            return err(_(Term), "Invalid rule function argument");
          },
      }};
  }

}