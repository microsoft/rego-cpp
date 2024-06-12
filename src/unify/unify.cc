#include "unify.hh"

namespace
{
  using namespace rego;

  const inline auto ScalarToken =
    T(Int) / T(Float) / T(True) / T(False) / T(Null);
}

namespace rego
{
  // Performs unification.
  PassDef unifier(BuiltIns builtins)
  {
    PassDef unify = {
      "unify",
      wf_pass_unify,
      dir::bottomup | dir::once,
      {
        In(Array, Set, ObjectItem) * (ScalarToken / T(JSONString))[Scalar] >>
          [](Match& _) {
            ACTION();
            return Term << (Scalar << _(Scalar));
          },
        In(Array, Set, ObjectItem) *
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
        In(ObjectItem) * T(DynamicObject)[DynamicObject] >>
          [](Match& _) {
            ACTION();
            return Term << (Object << *_[DynamicObject]);
          },
        In(Term) * T(DynamicObject)[DynamicObject] >>
          [](Match& _) {
            ACTION();
            return Object << *_[DynamicObject];
          },
        In(ObjectItem) * T(DynamicSet)[DynamicSet] >>
          [](Match& _) {
            ACTION();
            return Term << (Set << *_[DynamicSet]);
          },
        In(Term) * T(DynamicSet)[DynamicSet] >>
          [](Match& _) {
            ACTION();
            return Set << *_[DynamicSet];
          },
      }};

    unify.pre(Rego, [builtins](Node node) {
      logging::Debug() << "vvvvvvvvvvvvvvv Program vvvvvvvvvvvvvvv" << std::endl
                       << RegoStr{node} << std::endl
                       << "^^^^^^^^^^^^^^^ Program ^^^^^^^^^^^^^^^";
      Node query = node / Query;
      try
      {
        Nodes results = Resolver::resolve_query(query, builtins);
        node->replace(query, Query << results);
      }
      catch (const std::exception& e)
      {
        node->replace(query, err(query, e.what()));
      }
      return 0;
    });

    unify.post(Rego, [](Node node) {
      node / Input / Val = Term << (Scalar << (Null ^ "null"));
      node / Data / Val = DataModule;
      node / SkipSeq = SkipSeq;
      return 0;
    });

    return unify;
  }
}