#include "trieste/json.h"

#include "internal.hh"
#include "rego.hh"
#include "unify.hh"

namespace
{
  using namespace trieste;
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
    const auto wf_from_json_dataterm =
    (Top <<= DataTerm)
    | (DataObject <<= DataObjectItem++)
    | (DataObjectItem <<= (Key >>= DataTerm) * (Val >>= DataTerm))
    | (DataTerm <<= Scalar | DataArray | DataObject | DataSet)
    | (DataArray <<= DataTerm++)
    | (DataSet <<= DataTerm++)
    | (Scalar <<= String | Int | Float | True | False | Null)
    | (String <<= JSONString)
    ;
  // clang-format on

  // clang-format off
    const auto wf_from_json_term =
    (Top <<= Term)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= (Key >>= Term) * (Val >>= Term))
    | (Term <<= Scalar | Array | Object | Set)
    | (Array <<= Term++)
    | (Set <<= Term++)
    | (Scalar <<= JSONString | Int | Float | True | False | Null)
    ;
  // clang-format on

  const char* FloatRE =
    R"(\-?[[:digit:]]+\.[[:digit:]]+(?:e[+-]?[[:digit:]]+)?)";
  const char* IntRE = R"(\-?[[:digit:]]+)";

  PassDef from_json_to_dataterm()
  {
    return {
      "from_json",
      wf_from_json_dataterm,
      dir::bottomup | dir::once,
      {
        T(json::String)[String] >>
          [](Match& _) {
            return DataTerm << (Scalar << (String << (JSONString ^ _(String))));
          },

        T(json::Key)[Key] >>
          [](Match& _) {
            return DataTerm << (Scalar << (String << (JSONString ^ _(Key))));
          },

        T(json::Number, FloatRE)[Float] >>
          [](Match& _) { return DataTerm << (Scalar << (Float ^ _(Float))); },

        T(json::Number, IntRE)[Int] >>
          [](Match& _) { return DataTerm << (Scalar << (Int ^ _(Int))); },

        T(json::True)[True] >>
          [](Match& _) { return DataTerm << (Scalar << (True ^ _(True))); },

        T(json::False)[False] >>
          [](Match& _) { return DataTerm << (Scalar << (False ^ _(False))); },

        T(json::Null)[Null] >>
          [](Match& _) { return DataTerm << (Scalar << (Null ^ _(Null))); },

        T(json::Member)[DataObjectItem] >>
          [](Match& _) { return DataObjectItem << *_[DataObjectItem]; },

        T(json::Object)[DataObject] >>
          [](Match& _) { return DataTerm << (DataObject << *_[DataObject]); },

        T(json::Array)[DataArray] >>
          [](Match& _) { return DataTerm << (DataArray << *_[DataArray]); },
      }};
  }

  PassDef from_json_to_term()
  {
    return {
      "from_json",
      wf_from_json_term,
      dir::bottomup | dir::once,
      {
        T(json::String)[String] >>
          [](Match& _) { return Term << (Scalar << (JSONString ^ _(String))); },

        T(json::Key)[Key] >>
          [](Match& _) { return Term << (Scalar << (JSONString ^ _(Key))); },

        T(json::Number, FloatRE)[Float] >>
          [](Match& _) { return Term << (Scalar << (Float ^ _(Float))); },

        T(json::Number, IntRE)[Int] >>
          [](Match& _) { return Term << (Scalar << (Int ^ _(Int))); },

        T(json::True)[True] >>
          [](Match& _) { return Term << (Scalar << (True ^ _(True))); },

        T(json::False)[False] >>
          [](Match& _) { return Term << (Scalar << (False ^ _(False))); },

        T(json::Null)[Null] >>
          [](Match& _) { return Term << (Scalar << (Null ^ _(Null))); },

        T(json::Member)[ObjectItem] >>
          [](Match& _) { return ObjectItem << *_[ObjectItem]; },

        T(json::Object)[Object] >>
          [](Match& _) { return Term << (Object << *_[Object]); },

        T(json::Array)[Array] >>
          [](Match& _) { return Term << (Array << *_[Array]); },
      }};
  }

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

  PassDef to_json_()
  {
    return {
      "to_json",
      json::wf,
      dir::bottomup | dir::once,
      {
        (T(Term) << (T(Scalar) << T(Int, Float)[json::Number])) >>
          [](Match& _) { return json::Number ^ _(json::Number); },

        (T(Term) << (T(Scalar) << T(JSONString)[json::String])) >>
          [](Match& _) { return json::String ^ _(json::String); },

        (T(Term) << (T(Scalar) << T(True)[json::True])) >>
          [](Match& _) { return json::True ^ _(json::True); },

        (T(Term) << (T(Scalar) << T(False)[json::False])) >>
          [](Match& _) { return json::False ^ _(json::False); },

        (T(Term) << (T(Scalar) << T(Null)[json::Null])) >>
          [](Match& _) { return json::Null ^ _(json::Null); },

        (T(Term) << (T(Array, Set)[json::Array])) >>
          [](Match& _) {
            Node array = _(json::Array);
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
                    lhs = json::to_string(a);
                    strs[a] = lhs;
                  }

                  std::string rhs;
                  if (contains(strs, b))
                  {
                    rhs = strs[b];
                  }
                  else
                  {
                    rhs = json::to_string(b);
                    strs[b] = rhs;
                  }

                  return lhs < rhs;
                });
            }
            return json::Array << children;
          },

        (T(Term) << (T(Object)[json::Object])) >>
          [](Match& _) {
            Node object = _(json::Object);
            std::vector<std::pair<std::string, Node>> members;
            for (auto& child : *object)
            {
              std::string key = strip_quotes(json::to_string(child / Key));
              members.push_back({key, child / Val});
            }

            std::sort(members.begin(), members.end(), [](auto& a, auto& b) {
              return a.first < b.first;
            });

            Nodes children;
            std::transform(
              members.begin(),
              members.end(),
              std::back_inserter(children),
              [](auto& member) {
                return json::Member << (json::Key ^ member.first)
                                    << member.second;
              });

            return json::Object << children;
          },
      }};
  }
}

namespace rego
{
  Rewriter from_json(bool as_term)
  {
    auto pass = as_term ? from_json_to_term() : from_json_to_dataterm();
    return {
      "from_json",
      {pass},
      json::wf,
    };
  }

  Rewriter to_json()
  {
    return {
      "to_json",
      {to_json_()},
      wf_binding_term,
    };
  }
}