#include "passes.h"
#include "resolver.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  inline const auto wfi =
      (Top <<= Rego)
    | (Rego <<= Query * Input * Data)
    ;
  // clang-format on

}

namespace rego
{
  // Performs unification.
  PassDef unify(const BuiltIns& builtins)
  {
    PassDef unify = {
      dir::topdown | dir::once,
      {
        In(Input) * T(DataItemSeq) >>
          ([](Match&) -> Node { return DataItemSeq; }),
        In(Data) * T(DataItemSeq) >>
          ([](Match&) -> Node { return DataItemSeq; }),
        In(Rego) * T(SkipSeq) >> ([](Match&) -> Node { return SkipSeq; }),
        (In(Query) / In(Array) / In(Set) / In(ObjectItem)) *
            (ScalarToken / T(JSONString))[Scalar] >>
          [](Match& _) { return Term << (Scalar << _(Scalar)); },
        (In(Query) / In(Array) / In(Set) / In(ObjectItem)) *
            (T(Object) / T(Array) / T(Set) / T(Scalar))[Term] >>
          [](Match& _) { return Term << _(Term); },
        (In(Query) / In(Array) / In(Set) / In(ObjectItem)) *
            T(DataTerm)[DataTerm] >>
          [](Match& _) { return Term << _(DataTerm)->front(); },
        In(Term) * T(DataArray)[DataArray] >>
          [](Match& _) { return Array << *_[DataArray]; },
        In(Term) * T(DataSet)[DataSet] >>
          [](Match& _) { return Set << *_[DataSet]; },
        In(Term) * T(DataObject)[DataObject] >>
          [](Match& _) { return Object << *_[DataObject]; },
        In(Object) * T(DataItem)[DataItem] >>
          [](Match& _) { return ObjectItem << *_[DataItem]; },

        // errors
        T(Query)[Query] << End >>
          [](Match& _) { return err(_(Query), "no values in query"); },
      }};

    unify.pre(Rego, [builtins](Node node) {
      Node query = wfi / node / Query;
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