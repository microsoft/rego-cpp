#include "unify.hh"

#include <trieste/json.h>

namespace rego
{
  // Convert all RawString nodes to JSONString nodes.
  PassDef strings()
  {
    return {
      "strings",
      wf_pass_strings,
      dir::bottomup | dir::once,
      {
        In(Scalar) * (T(String) << T(RawString)[RawString]) >>
          [](Match& _) {
            ACTION();
            std::string raw_string =
              std::string(_(RawString)->location().view());
            raw_string = raw_string.substr(1, raw_string.size() - 2);
            std::string json_string = '"' + json::escape(raw_string) + '"';
            return JSONString ^ json_string;
          },

        In(Scalar) * (T(String) << T(JSONString)[JSONString]) >>
          [](Match& _) {
            ACTION();
            return _(JSONString);
          },
      }};
  }
}