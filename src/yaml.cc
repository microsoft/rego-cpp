#include "trieste/yaml.h"

#include "trieste/json.h"
#include "unify.hh"

namespace
{
  using namespace trieste;
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  const auto wf_binding_term =
    (Top <<= Term)
    | (Term <<= Scalar | Array | Object | Set)
    | (Scalar <<= Int | Float | True | False | Null | JSONString)
    | (Array <<= Term++)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= (Key >>= Term) * (Val >>= Term))
    | (Set <<= Term++)
    ;
  // clang-format on

  PassDef to_yaml_()
  {
    return {
      "to_yaml",
      yaml::wf,
      dir::topdown,
      {
        In(Top) *
            T(yaml::Int,
              yaml::Float,
              yaml::Value,
              yaml::True,
              yaml::False,
              yaml::Null,
              yaml::Mapping,
              yaml::Sequence)[yaml::Document] >>
          [](Match& _) {
            return yaml::Stream
              << yaml::Directives
              << (yaml::Documents
                  << (yaml::Document << yaml::Directives << yaml::DocumentStart
                                     << _(yaml::Document)
                                     << yaml::DocumentEnd));
          },

        (T(Term) << (T(Scalar) << T(Int)[Int])) >>
          [](Match& _) { return yaml::Int ^ _(Int); },

        (T(Term) << (T(Scalar) << T(Float)[Float])) >>
          [](Match& _) { return yaml::Float ^ _(Float); },

        (T(Term) << (T(Scalar) << T(JSONString)[JSONString])) >>
          [](Match& _) { return yaml::Value ^ _(JSONString); },

        (T(Term) << (T(Scalar) << T(True)[True])) >>
          [](Match& _) { return yaml::True ^ _(True); },

        (T(Term) << (T(Scalar) << T(False)[False])) >>
          [](Match& _) { return yaml::False ^ _(False); },

        (T(Term) << (T(Scalar) << T(Null)[Null])) >>
          [](Match& _) { return yaml::Null ^ _(Null); },

        (T(Term) << (T(Object)[Object])) >>
          [](Match& _) { return yaml::Mapping << *_[Object]; },

        T(ObjectItem)[ObjectItem] >>
          [](Match& _) { return yaml::MappingItem << *_[ObjectItem]; },

        (T(Term) << (T(Array, Set)[Array])) >>
          [](Match& _) {
            Node array = _(Array);
            Nodes children(array->begin(), array->end());
            if (array == Set)
            {
              NodeMap<std::string> strs;
              std::sort(
                children.begin(), children.end(), [&strs](Node a, Node b) {
                  std::string lhs;
                  if (contains(strs, a))
                  {
                    lhs = strs[a];
                  }
                  else
                  {
                    lhs = rego::to_key(a);
                    strs[a] = lhs;
                  }

                  std::string rhs;
                  if (contains(strs, b))
                  {
                    rhs = strs[b];
                  }
                  else
                  {
                    rhs = rego::to_key(b);
                    strs[b] = rhs;
                  }

                  return lhs < rhs;
                });
            }
            return yaml::Sequence << children;
          },
      }};
  }
}

namespace rego
{
  Rewriter to_yaml()
  {
    return {
      "to_yaml",
      {to_yaml_()},
      wf_binding_term,
    };
  }
}