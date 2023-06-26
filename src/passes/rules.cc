#include "passes.h"
#include "resolver.h"

namespace {
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
  PassDef rules()
  {
    PassDef rules = {
      dir::topdown | dir::once,
      {
        In(DataModuleSeq) * T(DataModule) >>
          ([](Match&) -> Node { return {}; }),
        In(Input) * T(ObjectItemSeq) >>
          ([](Match&) -> Node { return ObjectItemSeq; }),
        In(Data) * T(ObjectItemSeq) >>
          ([](Match&) -> Node { return ObjectItemSeq; }),
        (In(Query) / In(Array) / In(Set) / In(ObjectItem)) *
            (ScalarToken / T(JSONString))[Scalar] >>
          [](Match& _) { return Term << (Scalar << _(Scalar)); },
        (In(Query) / In(Array) / In(Set) / In(ObjectItem)) *
            (T(Object) / T(Array) / T(Set) / T(Scalar))[Term] >>
          [](Match& _) { return Term << _(Term); },

        // errors
        T(Query)[Query] << End >>
          [](Match& _) { return err(_(Query), "no values in query"); },
      }};

    rules.pre(Rego, [](Node node) {
      Node query = node->at(wfi / Rego / Query);
      try
      {
        Resolver::resolve_query(query);
      }
      catch (const std::exception& e)
      {
        query->erase(query->begin(), query->end());
        query->push_back(err(e.what()));
      }
      return 0;
    });

    return rules;
  }
}