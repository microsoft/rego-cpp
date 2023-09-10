#include "internal.h"

namespace
{
  std::string raw_to_json(const std::string& raw_string)
  {
    std::ostringstream buf;
    buf << '"';
    for (auto& c : raw_string)
    {
      switch (c)
      {
        case '`':
          break;

        case '"':
          buf << "\\\"";
          break;
        case '\\':
          buf << "\\\\";
          break;

        case '/':
          buf << "\\/";
          break;

        case '\b':
          buf << "\\b";
          break;

        case '\f':
          buf << "\\f";
          break;

        case '\n':
          buf << "\\n";
          break;

        case '\r':
          buf << "\\r";
          break;

        case '\t':
          buf << "\\t";
          break;

        default:
          buf << c;
          break;
      }
    }

    buf << '"';

    return buf.str();
  }

}

namespace rego
{
  // Convert all RawString nodes to JSONString nodes.
  PassDef strings()
  {
    return {
      In(String) * T(RawString)[RawString] >>
        [](Match& _) {
          std::string raw_string = std::string(_(RawString)->location().view());
          return JSONString ^ raw_to_json(raw_string);
        },

      In(Scalar) * (T(String) << T(JSONString)[JSONString]) >>
        [](Match& _) { return _(JSONString); },
    };
  }
}