#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  const char* Message = "JSON Web Tokens are not supported";

  BuiltIn decode_factory()
  {
    const Node decode_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "jwt")
                      << (bi::Description ^ "JWT token to decode")
                      << (bi::Type << bi::String)))
      << (bi::Result
          << (bi::Name ^ "output")
          << (bi::Description ^
              "`[header, payload, sig]`, where `header` and `payload` "
              "are objects; `sig` is the hexadecimal representation of "
              "the signature on the token.")
          << (bi::Type
              << (bi::StaticArray
                  << (bi::Type
                      << (bi::DynamicObject << (bi::Type << bi::Any)
                                            << (bi::Type << bi::Any)))
                  << (bi::Type
                      << (bi::DynamicObject << (bi::Type << bi::Any)
                                            << (bi::Type << bi::Any)))
                  << (bi::Type << bi::String))));
    return BuiltInDef::placeholder({"io.jwt.decode"}, decode_decl, Message);
  }

  BuiltIn decode_verify_factory()
  {
    const Node decode_verify_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg
                         << (bi::Name ^ "jwt")
                         << (bi::Description ^
                             "JWT token whose signature is to be verified and "
                             "whose claims are to be checked")
                         << (bi::Type << bi::String))
                     << (bi::Arg
                         << (bi::Name ^ "constraints")
                         << (bi::Description ^ "claim verification constraints")
                         << (bi::Type
                             << (bi::DynamicObject << (bi::Type << bi::String)
                                                   << (bi::Type << bi::Any)))))
      << (bi::Result
          << (bi::Name ^ "output")
          << (bi::Description ^
              "`[valid, header, payload]`: if the input token is verified and "
              "meets the requirements of `constraints` then `valid` is `true`; "
              "`header` and `payload` are objects containing the JOSE header "
              "and "
              "the JWT claim set; otherwise, `valid` is `false`, `header` and "
              "`payload` are `{}`")
          << (bi::Type
              << (bi::StaticArray
                  << (bi::Type << bi::Boolean)
                  << (bi::Type
                      << (bi::DynamicObject << (bi::Type << bi::Any)
                                            << (bi::Type << bi::Any)))
                  << (bi::Type
                      << (bi::DynamicObject << (bi::Type << bi::Any)
                                            << (bi::Type << bi::Any))))));
    return BuiltInDef::placeholder(
      {"io.jwt.decode_verify"}, decode_verify_decl, Message);
  }

  const Node verify_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg
                       << (bi::Name ^ "jwt")
                       << (bi::Description ^
                           "JWT token whose signature is to be verified")
                       << (bi::Type << bi::String))
                   << (bi::Arg
                       << (bi::Name ^ "certificate")
                       << (bi::Description ^
                           "PEM encoded certificate, PEM encoded public key, "
                           "or the JWK key (set) used to verify the signature")
                       << (bi::Type << bi::String)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^
                       "`true` if the signature is valid`, `false` otherwise")
                   << (bi::Type << bi::Boolean));

  BuiltIn encode_sign_factory()
  {
    const Node encode_sign_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "headers")
                      << (bi::Description ^ "JWS Protected header")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::String)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg << (bi::Name ^ "payload")
                      << (bi::Description ^ "JWS Payload")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::String)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg << (bi::Name ^ "key")
                      << (bi::Description ^ "JSON Web Key (RFC7517)")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::String)
                                                << (bi::Type << bi::Any)))))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^ "signed JWT")
                     << (bi::Type << bi::String));
    return BuiltInDef::placeholder(
      {"io.jwt.encode_sign"}, encode_sign_decl, Message);
  }

  BuiltIn encode_sign_raw_factory()
  {
    const Node encode_sign_raw_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "headers")
                                 << (bi::Description ^ "JWS Protected header")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "payload")
                                 << (bi::Description ^ "JWS Payload")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "key")
                                 << (bi::Description ^ "JSON Web Key (RFC7517)")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^ "signed JWT")
                     << (bi::Type << bi::String));
    return BuiltInDef::placeholder(
      {"io.jwt.encode_sign_raw"}, encode_sign_raw_decl, Message);
  }

}

namespace rego
{
  namespace builtins
  {
    BuiltIn jwt(const Location& name)
    {
      assert(name.view().starts_with("io.jwt."));
      std::string_view view = name.view().substr(7); // skip "io.jwt."
      if (view == "decode")
      {
        return decode_factory();
      }
      if (view == "decode_verify")
      {
        return decode_verify_factory();
      }
      if (
        view == "verify_eddsa" || view == "verify_es256" ||
        view == "verify_es384" || view == "verify_es512" ||
        view == "verify_hs256" || view == "verify_hs384" ||
        view == "verify_hs512" || view == "verify_ps256" ||
        view == "verify_ps384" || view == "verify_ps512" ||
        view == "verify_rs256" || view == "verify_rs384" ||
        view == "verify_rs512")
      {
        return BuiltInDef::placeholder(name, verify_decl, Message);
      }
      if (view == "encode_sign")
      {
        return encode_sign_factory();
      }
      if (view == "encode_sign_raw")
      {
        return encode_sign_raw_factory();
      }

      return nullptr;
    }
  }
}