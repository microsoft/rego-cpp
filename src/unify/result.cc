#include "unify.hh"

namespace
{
  const auto FreshVarPattern = R"([[:alnum:]|_]+\$\d+)";
}

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
        In(Top) * (T(Rego) << (T(Query) << T(Result)++[Result])) >>
          [](Match& _) {
            ACTION();
            if (_[Result].empty())
            {
              return Undefined ^ "";
            }
            return Results << _[Result];
          },

        T(ObjectItem)
            << (T(Term) << (T(Scalar) << T(JSONString, FreshVarPattern))) >>
          [](Match&) -> Node { return nullptr; },

        T(Binding) << T(Var, FreshVarPattern) >>
          [](Match&) -> Node { return nullptr; },

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