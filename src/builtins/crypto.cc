#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  const char* Message = "Cryptography built-ins are not supported";

  namespace hmac
  {
    BuiltIn equal_factory()
    {
      const Node equal_decl = bi::Decl
        << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "mac1")
                                   << (bi::Description ^ "mac1 to compare")
                                   << (bi::Type << bi::String))
                       << (bi::Arg << (bi::Name ^ "mac2")
                                   << (bi::Description ^ "mac2 to compare")
                                   << (bi::Type << bi::String)))
        << (bi::Result << (bi::Name ^ "result")
                       << (bi::Description ^
                           "`true` if the MACs are equals, `false` otherwise")
                       << (bi::Type << bi::Boolean));
      return BuiltInDef::placeholder(
        {"crypto.hmac.equal"}, equal_decl, Message);
    }

    BuiltIn md5_factory()
    {
      const Node md5_decl = bi::Decl
        << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                   << (bi::Description ^ "input string")
                                   << (bi::Type << bi::String))
                       << (bi::Arg << (bi::Name ^ "key")
                                   << (bi::Description ^ "key to use")
                                   << (bi::Type << bi::String)))
        << (bi::Result << (bi::Name ^ "y")
                       << (bi::Description ^ "MD5-HMAC of `x`")
                       << (bi::Type << bi::String));
      return BuiltInDef::placeholder({"crypto.hmac.md5"}, md5_decl, Message);
    }

    BuiltIn sha1_factory()
    {
      const Node sha1_decl = bi::Decl
        << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                   << (bi::Description ^ "input string")
                                   << (bi::Type << bi::String))
                       << (bi::Arg << (bi::Name ^ "key")
                                   << (bi::Description ^ "key to use")
                                   << (bi::Type << bi::String)))
        << (bi::Result << (bi::Name ^ "y")
                       << (bi::Description ^ "SHA1-HMAC of `x`")
                       << (bi::Type << bi::String));
      return BuiltInDef::placeholder({"crypto.hmac.sha1"}, sha1_decl, Message);
    }

    BuiltIn sha256_factory()
    {
      const Node sha256_decl = bi::Decl
        << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                   << (bi::Description ^ "input string")
                                   << (bi::Type << bi::String))
                       << (bi::Arg << (bi::Name ^ "key")
                                   << (bi::Description ^ "key to use")
                                   << (bi::Type << bi::String)))
        << (bi::Result << (bi::Name ^ "y")
                       << (bi::Description ^ "SHA256-HMAC of `x`")
                       << (bi::Type << bi::String));
      return BuiltInDef::placeholder(
        {"crypto.hmac.sha256"}, sha256_decl, Message);
    }

    BuiltIn sha512_factory()
    {
      const Node sha512_decl = bi::Decl
        << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                   << (bi::Description ^ "input string")
                                   << (bi::Type << bi::String))
                       << (bi::Arg << (bi::Name ^ "key")
                                   << (bi::Description ^ "key to use")
                                   << (bi::Type << bi::String)))
        << (bi::Result << (bi::Name ^ "y")
                       << (bi::Description ^ "SHA512-HMAC of `x`")
                       << (bi::Type << bi::String));
      return BuiltInDef::placeholder(
        {"crypto.hmac.sha512"}, sha512_decl, Message);
    }
  } // namespace hmac

  BuiltIn md5_factory()
  {
    const Node md5_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input string")
                               << (bi::Type << bi::String)))
               << (bi::Result << (bi::Name ^ "y")
                              << (bi::Description ^ "MD5-hash of `x`")
                              << (bi::Type << bi::String));
    return BuiltInDef::placeholder({"crypto.md5"}, md5_decl, Message);
  }

  BuiltIn parse_private_keys_factory()
  {
    const Node parse_private_keys_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg
              << (bi::Name ^ "keys")
              << (bi::Description ^
                  "PEM encoded data containing one or more private keys "
                  "as concatenated blocks. Optionally Base64 encoded.")
              << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "parsed private keys represented as objects")
                     << (bi::Type
                         << (bi::DynamicArray
                             << (bi::Type
                                 << (bi::DynamicObject
                                     << (bi::Type << bi::String)
                                     << (bi::Type << bi::Any))))));
    return BuiltInDef::placeholder(
      {"crypto.parse_private_keys"}, parse_private_keys_decl, Message);
  }

  BuiltIn sha1_factory()
  {
    const Node sha1_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input string")
                               << (bi::Type << bi::String)))
               << (bi::Result << (bi::Name ^ "y")
                              << (bi::Description ^ "SHA1-hash of `x`")
                              << (bi::Type << bi::String));
    return BuiltInDef::placeholder({"crypto.sha1"}, sha1_decl, Message);
  }

  BuiltIn sha256_factory()
  {
    const Node sha256_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input string")
                               << (bi::Type << bi::String)))
               << (bi::Result << (bi::Name ^ "y")
                              << (bi::Description ^ "SHA256-hash of `x`")
                              << (bi::Type << bi::String));
    return BuiltInDef::placeholder({"crypto.sha256"}, sha256_decl, Message);
  }

  namespace x509
  {
    BuiltIn parse_and_verify_certificates_factory()
    {
      const Node parse_and_verify_certificates_decl = bi::Decl
        << (bi::ArgSeq
            << (bi::Arg
                << (bi::Name ^ "certs")
                << (bi::Description ^
                    "base64 encoded DER or PEM data containing two or more "
                    "certificates where the first is a root CA, the last is a "
                    "leaf certificate, and all others are intermediate CAs")
                << (bi::Type << bi::String)))
        << (bi::Result
            << (bi::Name ^ "output")
            << (bi::Description ^
                "array of `[valid, certs]`: if the input certificate chain "
                "could "
                "be verified then `valid` is `true` and `certs` is an array of "
                "X.509 certificates represented as objects; if the input "
                "certificate chain could not be verified then `valid` is "
                "`false "
                "and `certs` is `[]`")
            << (bi::Type
                << (bi::StaticArray
                    << (bi::Type << bi::Boolean)
                    << (bi::Type
                        << (bi::DynamicArray
                            << (bi::Type
                                << (bi::DynamicObject
                                    << (bi::Type << bi::String)
                                    << (bi::Type << bi::Any))))))));
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_and_verify_certificates"},
        parse_and_verify_certificates_decl,
        Message);
    }

    BuiltIn parse_and_verify_certificates_with_options_factory()
    {
      const Node parse_and_verify_certificates_with_options_decl = bi::Decl
        << (bi::ArgSeq
            << (bi::Arg
                << (bi::Name ^ "certs")
                << (bi::Description ^
                    "base64 encoded DER or PEM data containing two or more "
                    "certificates where the first is a root CA, the last is a "
                    "leaf certificate, and all others are intermediate CAs")
                << (bi::Type << bi::String))
            << (bi::Arg
                << (bi::Name ^ "options")
                << (bi::Description ^
                    "object containing extra configs to verify the validity of "
                    "certificates. `options` object supports four fields which "
                    "maps to same fields in "
                    "[x509.VerifyOptions](https://pkg.go.dev/crypto/"
                    "x509#VerifyOptions) struct. `DNSName`, `CurrentTime`: "
                    "Nanoseconds since the Unix Epoch as a number, "
                    "`MaxConstraintComparisons` and `KeyUsages`. `KeyUsages` "
                    "is "
                    "list and can have possible values as in: "
                    "`\"KeyUsageAny\"`, "
                    "`\"KeyUsageServerAuth\"`, `\"KeyUsageClientAuth\"`, "
                    "`\"KeyUsageCodeSigning\"`, `\"KeyUsageEmailProtection\"`, "
                    "`\"KeyUsageIPSECEndSystem\"`, `\"KeyUsageIPSECTunnel\"`, "
                    "`\"KeyUsageIPSECUser\"`, `\"KeyUsageTimeStamping\"`, "
                    "`\"KeyUsageOCSPSigning\"`, "
                    "`\"KeyUsageMicrosoftServerGatedCrypto\"`, "
                    "`\"KeyUsageNetscapeServerGatedCrypto\"`, "
                    "`\"KeyUsageMicrosoftCommercialCodeSigning\"`, "
                    "`\"KeyUsageMicrosoftKernelCodeSigning\"`")
                << (bi::Type ^
                    (bi::DynamicObject << (bi::Type << bi::String)
                                       << (bi::Type << bi::Any)))))
        << (bi::Result
            << (bi::Name ^ "output")
            << (bi::Description ^
                "array of `[valid, certs]`: if the input certificate chain "
                "could "
                "be verified then `valid` is `true` and `certs` is an array of "
                "X.509 certificates represented as objects; if the input "
                "certificate chain could not be verified then `valid` is "
                "`false "
                "and `certs` is `[]`")
            << (bi::Type
                << (bi::StaticArray
                    << (bi::Type << bi::Boolean)
                    << (bi::Type
                        << (bi::DynamicArray
                            << (bi::Type
                                << (bi::DynamicObject
                                    << (bi::Type << bi::String)
                                    << (bi::Type << bi::Any))))))));
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_and_verify_certificates_with_options"},
        parse_and_verify_certificates_with_options_decl,
        Message);
    }

    BuiltIn parse_certificate_request_factory()
    {
      const Node parse_certificate_request_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg
                         << (bi::Name ^ "csr")
                         << (bi::Description ^
                             "base64 string containing either a PEM encoded or "
                             "DER CSR or a string containing at PEM CSR")
                         << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "output")
                     << (bi::Description ^ "X.509 CSR represented as an object")
                     << (bi::Type
                         << (bi::DynamicObject << (bi::Type << bi::String)
                                               << (bi::Type << bi::Any))));
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_certificate_request"},
        parse_certificate_request_decl,
        Message);
    }

    BuiltIn parse_certificates_factory()
    {
      const Node parse_certificates_decl = bi::Decl
        << (bi::ArgSeq
            << (bi::Arg
                << (bi::Name ^ "certs")
                << (bi::Description ^
                    "base64 encoded DER or PEM data containing one or more "
                    "certificates or a PEM string of one or more certificates")
                << (bi::Type << bi::String)))
        << (bi::Result << (bi::Name ^ "output")
                       << (bi::Description ^
                           "parse X.509 certificates represented as objects")
                       << (bi::Type
                           << (bi::DynamicArray
                               << (bi::Type
                                   << (bi::DynamicObject
                                       << (bi::Type << bi::String)
                                       << (bi::Type << bi::Any))))));
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_certificates"}, parse_certificates_decl, Message);
    }

    BuiltIn parse_keypair_factory()
    {
      const Node parse_keypair_decl = bi::Decl
        << (bi::ArgSeq
            << (bi::Arg
                << (bi::Name ^ "cert")
                << (bi::Description ^
                    "string containing PEM or base64 encoded DER certificates")
                << (bi::Type << bi::String))
            << (bi::Arg << (bi::Name ^ "pem")
                        << (bi::Description ^
                            "string containing PEM or base64 encoded DER keys")
                        << (bi::Type << bi::String)))
        << (bi::Result << (bi::Name ^ "output")
                       << (bi::Description ^
                           "if key pair is valid, returns the "
                           "tls.certificate(https://pkg.go.dev/crypto/"
                           "tls#Certificate) as an object. If the key pair is "
                           "invalid, nil and an error are returned.")
                       << (bi::Type
                           << (bi::DynamicObject << (bi::Type << bi::String)
                                                 << (bi::Type << bi::Any))));
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_keypair"}, parse_keypair_decl, Message);
    }

    BuiltIn parse_rsa_private_key_factory()
    {
      const Node parse_rsa_private_key_decl = bi::Decl
        << (bi::ArgSeq
            << (bi::Arg
                << (bi::Name ^ "pem")
                << (bi::Description ^
                    "base64 string containing a PEM encoded RSA private key")
                << (bi::Type << (bi::String))))
        << (bi::Result << (bi::Name ^ "output")
                       << (bi::Description ^ "JWK as an object")
                       << (bi::Type
                           << (bi::DynamicObject << (bi::Type << bi::String)
                                                 << (bi::Type << bi::Any))));
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_rsa_private_key"},
        parse_rsa_private_key_decl,
        Message);
    }
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn crypto(const Location& name)
    {
      assert(name.view().starts_with("crypto."));
      std::string_view view = name.view().substr(7); // skip "crypto."
      if (view[0] == 'h')
      {
        assert(view.starts_with("hmac."));
        view = view.substr(5); // skip "hmac."
        if (view == "equal")
        {
          return hmac::equal_factory();
        }
        if (view == "md5")
        {
          return hmac::md5_factory();
        }
        if (view == "sha1")
        {
          return hmac::sha1_factory();
        }
        if (view == "sha256")
        {
          return hmac::sha256_factory();
        }
        if (view == "sha512")
        {
          return hmac::sha512_factory();
        }
      }

      if (view[0] == 'x')
      {
        assert(view.starts_with("x509."));
        view = view.substr(5); // skip "x509."
        if (view == "parse_and_verify_certificates")
        {
          return x509::parse_and_verify_certificates_factory();
        }
        if (view == "parse_and_verify_certificates_with_options")
        {
          return x509::parse_and_verify_certificates_with_options_factory();
        }
        if (view == "parse_certificate_request")
        {
          return x509::parse_certificate_request_factory();
        }
        if (view == "parse_certificates")
        {
          return x509::parse_certificates_factory();
        }
        if (view == "parse_keypair")
        {
          return x509::parse_keypair_factory();
        }
        if (view == "parse_rsa_private_key")
        {
          return x509::parse_rsa_private_key_factory();
        }
      }

      if (view == "md5")
      {
        return md5_factory();
      }
      if (view == "sha1")
      {
        return sha1_factory();
      }
      if (view == "sha256")
      {
        return sha256_factory();
      }
      if (view == "parse_private_keys")
      {
        return parse_private_keys_factory();
      }

      return nullptr;
    }
  }
}