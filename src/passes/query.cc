#include "internal.hh"

namespace rego
{
  // Extracts the query result from the AST.
  PassDef query()
  {
    return {
      "query",
      wf_pass_query,
      dir::topdown,
      {
        In(Top) * (T(Rego) << T(Query)[Query]) >>
          [](Match& _) {
            ACTION();
            Node seq = NodeDef::create(Seq);
            for (auto& child : *_(Query))
            {
              seq->push_back(child);
            }
            return seq;
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