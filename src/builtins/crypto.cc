#include "builtins.hh"

#ifdef REGOCPP_HAS_CRYPTO
#include "crypto_core.hh"
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <trieste/json.h>

#ifdef REGOCPP_HAS_CRYPTO
namespace rego::crypto_core
{
  void secure_erase(std::string& s)
  {
#if defined(_WIN32)
    SecureZeroMemory(s.data(), s.size());
#else
    // volatile prevents the optimizer from removing the memset
    volatile char* p = s.data();
    for (size_t i = 0; i < s.size(); ++i)
    {
      p[i] = 0;
    }
    // Compiler barrier: ensures the volatile writes above are not
    // reordered or eliminated across this point.
    asm volatile("" : : "r"(s.data()) : "memory");
#endif
    s.clear();
  }
} // namespace rego::crypto_core
#endif

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  const char* Message = "Cryptography built-ins are not supported";

#ifdef REGOCPP_HAS_CRYPTO
  Node md5_impl(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("crypto.md5"));
    if (x->type() == Error)
      return x;
    return JSONString ^ crypto_core::md5_hex(get_string(x));
  }

  Node sha1_impl(const Nodes& args)
  {
    Node x =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("crypto.sha1"));
    if (x->type() == Error)
      return x;
    return JSONString ^ crypto_core::sha1_hex(get_string(x));
  }

  Node sha256_impl(const Nodes& args)
  {
    Node x =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("crypto.sha256"));
    if (x->type() == Error)
      return x;
    return JSONString ^ crypto_core::sha256_hex(get_string(x));
  }

  Node hmac_equal_impl(const Nodes& args)
  {
    Node mac1 =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("crypto.hmac.equal"));
    if (mac1->type() == Error)
      return mac1;
    Node mac2 =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("crypto.hmac.equal"));
    if (mac2->type() == Error)
      return mac2;
    if (crypto_core::hmac_equal(get_string(mac1), get_string(mac2)))
      return True ^ "true";
    return False ^ "false";
  }

  Node hmac_md5_impl(const Nodes& args)
  {
    Node x =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("crypto.hmac.md5"));
    if (x->type() == Error)
      return x;
    Node key =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("crypto.hmac.md5"));
    if (key->type() == Error)
      return key;
    return JSONString ^
      crypto_core::hmac_md5_hex(get_string(key), get_string(x));
  }

  Node hmac_sha1_impl(const Nodes& args)
  {
    Node x =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("crypto.hmac.sha1"));
    if (x->type() == Error)
      return x;
    Node key =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("crypto.hmac.sha1"));
    if (key->type() == Error)
      return key;
    return JSONString ^
      crypto_core::hmac_sha1_hex(get_string(key), get_string(x));
  }

  Node hmac_sha256_impl(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("crypto.hmac.sha256"));
    if (x->type() == Error)
      return x;
    Node key = unwrap_arg(
      args, UnwrapOpt(1).type(JSONString).func("crypto.hmac.sha256"));
    if (key->type() == Error)
      return key;
    return JSONString ^
      crypto_core::hmac_sha256_hex(get_string(key), get_string(x));
  }

  Node hmac_sha512_impl(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("crypto.hmac.sha512"));
    if (x->type() == Error)
      return x;
    Node key = unwrap_arg(
      args, UnwrapOpt(1).type(JSONString).func("crypto.hmac.sha512"));
    if (key->type() == Error)
      return key;
    return JSONString ^
      crypto_core::hmac_sha512_hex(get_string(key), get_string(x));
  }
  // Build an Array node from a dynamic vector of nodes
  Node dynamic_array(const Nodes& items)
  {
    Node result = NodeDef::create(Array);
    for (auto& item : items)
    {
      result << Resolver::to_term(item);
    }
    return result;
  }

  // Convert a ParsedCertificate to a Rego object node
  Node cert_to_rego_object(const crypto_core::ParsedCertificate& pc)
  {
    // Build Subject object
    Node subject = object({object_item(
      rego::string("CommonName"), rego::string(pc.subject.common_name))});

    // DNSNames: null if empty, otherwise array of strings
    Node dns;
    if (pc.dns_names.empty())
    {
      dns = null();
    }
    else
    {
      Nodes dns_items;
      for (auto& name : pc.dns_names)
      {
        dns_items.push_back(rego::string(name));
      }
      dns = dynamic_array(dns_items);
    }

    // URIStrings: null if empty, otherwise array of strings
    Node uris;
    if (pc.uri_strings.empty())
    {
      uris = null();
    }
    else
    {
      Nodes uri_items;
      for (auto& uri : pc.uri_strings)
      {
        uri_items.push_back(rego::string(uri));
      }
      uris = dynamic_array(uri_items);
    }

    return object(
      {object_item(rego::string("DNSNames"), dns),
       object_item(rego::string("Subject"), subject),
       object_item(rego::string("URIStrings"), uris)});
  }

  Node rsa_jwk_to_rego_object(const crypto_core::RSAPrivateKeyJWK& jwk)
  {
    return object(
      {object_item(rego::string("d"), rego::string(jwk.d)),
       object_item(rego::string("dp"), rego::string(jwk.dp)),
       object_item(rego::string("dq"), rego::string(jwk.dq)),
       object_item(rego::string("e"), rego::string(jwk.e)),
       object_item(rego::string("kty"), rego::string(jwk.kty)),
       object_item(rego::string("n"), rego::string(jwk.n)),
       object_item(rego::string("p"), rego::string(jwk.p)),
       object_item(rego::string("q"), rego::string(jwk.q)),
       object_item(rego::string("qi"), rego::string(jwk.qi))});
  }

  Node parse_certificates_impl(const Nodes& args)
  {
    Node input = unwrap_arg(
      args,
      UnwrapOpt(0).type(JSONString).func("crypto.x509.parse_certificates"));
    if (input->type() == Error)
      return input;

    auto result =
      crypto_core::parse_certificates(json::unescape(get_string(input)));
    if (!result.error.empty())
    {
      return err(args[0], result.error, EvalBuiltInError);
    }

    Nodes cert_nodes;
    for (auto& pc : result.certs)
    {
      cert_nodes.push_back(cert_to_rego_object(pc));
    }
    return dynamic_array(cert_nodes);
  }

  Node parse_and_verify_certificates_impl(const Nodes& args)
  {
    Node input = unwrap_arg(
      args,
      UnwrapOpt(0)
        .type(JSONString)
        .func("crypto.x509.parse_and_verify_certificates"));
    if (input->type() == Error)
      return input;

    auto result = crypto_core::parse_and_verify_certificates(
      json::unescape(get_string(input)));
    if (!result.error.empty())
    {
      return err(args[0], result.error, EvalBuiltInError);
    }

    Nodes cert_nodes;
    for (auto& pc : result.certs)
    {
      cert_nodes.push_back(cert_to_rego_object(pc));
    }
    return array({boolean(result.valid), dynamic_array(cert_nodes)});
  }

  Node parse_certificate_request_impl(const Nodes& args)
  {
    Node input = unwrap_arg(
      args,
      UnwrapOpt(0)
        .type(JSONString)
        .func("crypto.x509.parse_certificate_request"));
    if (input->type() == Error)
      return input;

    auto result =
      crypto_core::parse_certificate_request(json::unescape(get_string(input)));
    if (!result.error.empty())
    {
      return err(args[0], result.error, EvalBuiltInError);
    }

    Node subject = object({object_item(
      rego::string("CommonName"), rego::string(result.subject.common_name))});
    return object({object_item(rego::string("Subject"), subject)});
  }

  Node parse_keypair_impl(const Nodes& args)
  {
    Node cert_input = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("crypto.x509.parse_keypair"));
    if (cert_input->type() == Error)
      return cert_input;
    Node key_input = unwrap_arg(
      args, UnwrapOpt(1).type(JSONString).func("crypto.x509.parse_keypair"));
    if (key_input->type() == Error)
      return key_input;

    // Parse certificates
    auto cert_result =
      crypto_core::parse_certificates(json::unescape(get_string(cert_input)));
    if (!cert_result.error.empty())
    {
      return err(args[0], cert_result.error, EvalBuiltInError);
    }

    // Parse the private key to verify it's valid
    auto key_result =
      crypto_core::parse_rsa_private_key(json::unescape(get_string(key_input)));
    if (!key_result.error.empty())
    {
      return err(args[1], key_result.error, EvalBuiltInError);
    }

    // Build Certificate array of base64 DER strings
    Nodes der_items;
    for (auto& pc : cert_result.certs)
    {
      der_items.push_back(rego::string(pc.der_b64));
    }

    return object(
      {object_item(rego::string("Certificate"), dynamic_array(der_items))});
  }

  Node parse_rsa_private_key_impl(const Nodes& args)
  {
    Node input = unwrap_arg(
      args,
      UnwrapOpt(0).type(JSONString).func("crypto.x509.parse_rsa_private_key"));
    if (input->type() == Error)
      return input;

    auto result =
      crypto_core::parse_rsa_private_key(json::unescape(get_string(input)));
    if (!result.error.empty())
    {
      return err(args[0], result.error, EvalBuiltInError);
    }

    return rsa_jwk_to_rego_object(result.key);
  }

  Node parse_private_keys_impl(const Nodes& args)
  {
    Node input = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("crypto.parse_private_keys"));
    if (input->type() == Error)
      return input;

    auto result =
      crypto_core::parse_private_keys(json::unescape(get_string(input)));
    if (result.is_empty_input)
    {
      return null();
    }

    Nodes key_nodes;
    for (auto& jwk : result.keys)
    {
      key_nodes.push_back(rsa_jwk_to_rego_object(jwk));
    }
    return dynamic_array(key_nodes);
  }
#endif // REGOCPP_HAS_CRYPTO

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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder(
        {"crypto.hmac.equal"}, equal_decl, Message);
#else
      return BuiltInDef::create(
        {"crypto.hmac.equal"}, equal_decl, hmac_equal_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder({"crypto.hmac.md5"}, md5_decl, Message);
#else
      return BuiltInDef::create({"crypto.hmac.md5"}, md5_decl, hmac_md5_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder({"crypto.hmac.sha1"}, sha1_decl, Message);
#else
      return BuiltInDef::create(
        {"crypto.hmac.sha1"}, sha1_decl, hmac_sha1_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder(
        {"crypto.hmac.sha256"}, sha256_decl, Message);
#else
      return BuiltInDef::create(
        {"crypto.hmac.sha256"}, sha256_decl, hmac_sha256_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder(
        {"crypto.hmac.sha512"}, sha512_decl, Message);
#else
      return BuiltInDef::create(
        {"crypto.hmac.sha512"}, sha512_decl, hmac_sha512_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
    return BuiltInDef::placeholder({"crypto.md5"}, md5_decl, Message);
#else
    return BuiltInDef::create({"crypto.md5"}, md5_decl, md5_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
    return BuiltInDef::placeholder(
      {"crypto.parse_private_keys"}, parse_private_keys_decl, Message);
#else
    return BuiltInDef::create(
      {"crypto.parse_private_keys"},
      parse_private_keys_decl,
      parse_private_keys_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
    return BuiltInDef::placeholder({"crypto.sha1"}, sha1_decl, Message);
#else
    return BuiltInDef::create({"crypto.sha1"}, sha1_decl, sha1_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
    return BuiltInDef::placeholder({"crypto.sha256"}, sha256_decl, Message);
#else
    return BuiltInDef::create({"crypto.sha256"}, sha256_decl, sha256_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_and_verify_certificates"},
        parse_and_verify_certificates_decl,
        Message);
#else
      return BuiltInDef::create(
        {"crypto.x509.parse_and_verify_certificates"},
        parse_and_verify_certificates_decl,
        parse_and_verify_certificates_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_certificate_request"},
        parse_certificate_request_decl,
        Message);
#else
      return BuiltInDef::create(
        {"crypto.x509.parse_certificate_request"},
        parse_certificate_request_decl,
        parse_certificate_request_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_certificates"}, parse_certificates_decl, Message);
#else
      return BuiltInDef::create(
        {"crypto.x509.parse_certificates"},
        parse_certificates_decl,
        parse_certificates_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_keypair"}, parse_keypair_decl, Message);
#else
      return BuiltInDef::create(
        {"crypto.x509.parse_keypair"}, parse_keypair_decl, parse_keypair_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
      return BuiltInDef::placeholder(
        {"crypto.x509.parse_rsa_private_key"},
        parse_rsa_private_key_decl,
        Message);
#else
      return BuiltInDef::create(
        {"crypto.x509.parse_rsa_private_key"},
        parse_rsa_private_key_decl,
        parse_rsa_private_key_impl);
#endif
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