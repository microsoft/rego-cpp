#include "internal.hh"

namespace rego
{
  // Performs unification.
  PassDef unify(const BuiltIns& builtins)
  {
    PassDef unify = {
      "unify",
      wf_pass_unify,
      dir::bottomup | dir::once,
      {
        In(Input) * T(Term) >>
          ([](Match&) -> Node { return Term << (Scalar << Null); }),
        In(Data) * T(DataModule) >> ([](Match&) -> Node { return DataModule; }),
        In(Rego) * T(SkipSeq) >> ([](Match&) -> Node { return SkipSeq; }),
        In(Query, Array, Set, ObjectItem) *
            (ScalarToken / T(JSONString))[Scalar] >>
          [](Match& _) {
            ACTION();
            return Term << (Scalar << _(Scalar));
          },
        In(Query, Array, Set, ObjectItem) *
            (T(Object) / T(Array) / T(Set) / T(Scalar))[Term] >>
          [](Match& _) {
            ACTION();
            return Term << _(Term);
          },
        In(Query, Array, Set, ObjectItem) * T(DataTerm)[DataTerm] >>
          [](Match& _) {
            ACTION();
            return Term << _(DataTerm)->front();
          },
        In(Term) * T(DataArray)[DataArray] >>
          [](Match& _) {
            ACTION();
            return Array << *_[DataArray];
          },
        In(Term) * T(DataSet)[DataSet] >>
          [](Match& _) {
            ACTION();
            return Set << *_[DataSet];
          },
        In(Term) * T(DataObject)[DataObject] >>
          [](Match& _) {
            ACTION();
            return Object << *_[DataObject];
          },
        In(Object) * T(DataItem)[DataItem] >>
          [](Match& _) {
            ACTION();
            return ObjectItem << *_[DataItem];
          },
      }};

    unify.pre(Rego, [builtins](Node node) {
      logging::Debug() << "vvvvvvvvvvvvvvv Program vvvvvvvvvvvvvvv" << std::endl
                       << RegoStr{node} << std::endl
                       << "^^^^^^^^^^^^^^^ Program ^^^^^^^^^^^^^^^";
      Node query = node / Query;
      try
      {
        Node result = Resolver::resolve_query(query, builtins);
        node->replace(query, result);
      }
      catch (const std::exception& e)
      {
        node->replace(query, err(query, e.what()));
      }
      return 0;
    });

    return unify;
  }
}