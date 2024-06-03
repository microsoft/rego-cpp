#include "unify.hh"

namespace rego
{
  // Extracts the query result from the AST.
  PassDef result()
  {
    return {
      "result",
      wf_result,
      dir::topdown,
      {
        In(Top) * (T(Rego) << T(Query)[Query]) >>
          [](Match& _) {
            ACTION();
            return Result << *_[Query];
          },

        (T(Array) / T(Set)) * T(Error)[Error] >>
          [](Match& _) {
            ACTION();
            return _(Error);
          },

        T(Object) * (T(ObjectItem) << (T(Key) * T(Error)[Error])) >>
          [](Match& _) {
            ACTION();
            return _(Error);
          },

        T(Scalar) * T(Error)[Error] >>
          [](Match& _) {
            ACTION();
            return _(Error);
          },

        T(Term) * T(Error)[Error] >>
          [](Match& _) {
            ACTION();
            return _(Error);
          },
      }};
  }
}