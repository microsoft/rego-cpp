#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  BuiltIn sign_req_factory()
  {
    Node sign_req_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "request")
                      << (bi::Description ^ "HTTP request object")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::String)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg << (bi::Name ^ "aws_config")
                      << (bi::Description ^ "AWS configuraiton object")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::String)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg << (bi::Name ^ "time_ns")
                      << (bi::Description ^ "nanoseconds since the epoch")
                      << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "signed_request")
                     << (bi::Description ^
                         "HTTP request object with `Authorization` header")
                     << (bi::Type
                         << (bi::DynamicObject << (bi::Type << bi::Any)
                                               << (bi::Type << bi::Any))));
    return BuiltInDef::placeholder(
      {"providers.aws.sign_req"}, sign_req_decl, "AWS provider not supported");
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn aws(const Location& name)
    {
      assert(name.view().starts_with("providers.aws."));
      std::string_view view = name.view().substr(14); // skip "providers.aws."
      if (view == "sign_req")
      {
        return sign_req_factory();
      }

      return nullptr;
    }
  }
}