#include "errors.h"
#include "helpers.h"
#include "log.h"
#include "passes.h"
#include "resolver.h"

namespace rego
{
  // Performs unification.
  PassDef unify(const BuiltIns& builtins)
  {
    PassDef unify = {
      dir::topdown | dir::once,
      {
        In(Input) * T(Term) >>
          ([](Match&) -> Node { return Term << (Scalar << JSONNull); }),
        In(Data) * T(DataModule) >> ([](Match&) -> Node { return DataModule; }),
        In(Rego) * T(SkipSeq) >> ([](Match&) -> Node { return SkipSeq; }),
        In(Query, Array, Set, ObjectItem) *
            (ScalarToken / T(JSONString))[Scalar] >>
          [](Match& _) { return Term << (Scalar << _(Scalar)); },
        In(Query, Array, Set, ObjectItem) *
            (T(Object) / T(Array) / T(Set) / T(Scalar))[Term] >>
          [](Match& _) { return Term << _(Term); },
        In(Query, Array, Set, ObjectItem) * T(DataTerm)[DataTerm] >>
          [](Match& _) { return Term << _(DataTerm)->front(); },
        In(Term) * T(DataArray)[DataArray] >>
          [](Match& _) { return Array << *_[DataArray]; },
        In(Term) * T(DataSet)[DataSet] >>
          [](Match& _) { return Set << *_[DataSet]; },
        In(Term) * T(DataObject)[DataObject] >>
          [](Match& _) { return Object << *_[DataObject]; },
        In(Object) * T(DataItem)[DataItem] >>
          [](Match& _) { return ObjectItem << *_[DataItem]; },
      }};

    unify.pre(Rego, [builtins](Node node) {
      LOG_HEADER(" Program ", "vvvvvvvvvvvvvvv");
      LOG(Resolver::rego_str(node));
      LOG_HEADER(" Program ", "^^^^^^^^^^^^^^^");
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