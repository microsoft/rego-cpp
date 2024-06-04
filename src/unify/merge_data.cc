#include "unify.hh"

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
        T(Input) << T(DataTerm, Undefined)[Val] >>
          [](Match& _) {
            ACTION();
            return Input << (Key ^ "input") << _(Val);
          },

        In(DataSeq) * (T(Data) << (T(DataTerm) << T(DataObject)[Data])) >>
          [](Match& _) {
            ACTION();
            return DataModule << *_[Data];
          },

        In(DataSeq) *
            (T(DataModule)[Lhs] *
             (T(Data) << (T(DataTerm) << T(DataObject)[Rhs]))) >>
          [](Match& _) {
            ACTION();
            return DataModule << *_[Lhs] << *_[Rhs];
          },

        In(DataModule) *
            (T(DataObjectItem)
             << ((T(DataTerm) << T(Scalar)[Scalar]) *
                 (T(DataTerm) << T(DataObject)[DataModule]))) >>
          [](Match& _) {
            ACTION();
            std::string key = strip_quotes(to_key(_(Scalar)));
            return Submodule << (Key ^ key) << (DataModule << *_[DataModule]);
          },

        In(DataModule) *
            (T(DataObjectItem)
             << ((T(DataTerm) << T(Scalar)[Scalar]) *
                 (T(DataTerm)
                  << (T(DataArray) / T(DataSet) / T(Scalar))[Term]))) >>
          [](Match& _) {
            ACTION();
            std::string key = strip_quotes(to_key(_(Scalar)));
            return DataRule << (Var ^ key) << (DataTerm << _(Term));
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

        In(DataSeq) *
            (T(Data) << (T(DataTerm) << T(DataArray, DataSet, Scalar)[Data])) >>
          [](Match& _) {
            ACTION();
            return err(_(Data), "Invalid data node");
          },

        In(DataModule) *
            (T(DataObjectItem)
             << (T(DataTerm) << T(DataArray, DataSet, DataObject)[Scalar])) >>
          [](Match& _) {
            ACTION();
            return err(_(Scalar), "Invalid data rule name");
          },

      }};
  }

}