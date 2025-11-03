#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  BuiltIn send_factory()
  {
    const Node send_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "request")
                       << (bi::Description ^ "the HTTP request object")
                       << (bi::Type
                           << (bi::DynamicObject << (bi::Type << bi::String)
                                                 << (bi::Type << bi::Any)))))
               << (bi::Result
                   << (bi::Name ^ "response")
                   << (bi::Description ^ "the HTTP response object")
                   << (bi::Type
                       << (bi::DynamicObject << (bi::Type << bi::Any)
                                             << (bi::Type << bi::Any))));
    return BuiltInDef::placeholder(
      {"http.send"}, send_decl, "HTTP builtins are not supported");
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn http(const Location& name)
    {
      assert(name.view().starts_with("http."));
      std::string_view view = name.view().substr(5); // skip "http."
      if (view == "send")
      {
        return send_factory();
      }

      return nullptr;
    }
  }
}