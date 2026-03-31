// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef REGOCPP_CRYPTO_OPENSSL3

#include "base64/base64.h"
#include "crypto_core.hh"
#include "crypto_utils.hh"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/param_build.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <stdexcept>
#include <trieste/json.h>
#include <vector>

namespace
{
  using rego::crypto_core::to_hex;

  // Safe wrapper around BIO_new_mem_buf that rejects inputs exceeding
  // INT_MAX, preventing undefined behaviour from the size_t → int cast.
  BIO_ptr bio_from_mem(const void* data, size_t len)
  {
    if (len > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
      return {nullptr, BIO_free};
    }
    return {BIO_new_mem_buf(data, static_cast<int>(len)), BIO_free};
  }

  std::string digest_hex(const EVP_MD* md, std::string_view data)
  {
    unsigned char buf[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
      EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx)
    {
      throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (
      EVP_DigestInit_ex(ctx.get(), md, nullptr) != 1 ||
      EVP_DigestUpdate(ctx.get(), data.data(), data.size()) != 1 ||
      EVP_DigestFinal_ex(ctx.get(), buf, &len) != 1)
    {
      throw std::runtime_error("EVP_Digest failed");
    }

    return to_hex(buf, len);
  }

  std::string hmac_hex(
    const EVP_MD* md, std::string_view key, std::string_view data)
  {
    unsigned char buf[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    unsigned char* result = HMAC(
      md,
      key.data(),
      static_cast<int>(key.size()),
      reinterpret_cast<const unsigned char*>(data.data()),
      data.size(),
      buf,
      &len);
    if (!result)
    {
      throw std::runtime_error("HMAC failed");
    }

    return to_hex(buf, len);
  }
}

namespace rego::crypto_core
{
  std::string md5_hex(std::string_view data)
  {
    return digest_hex(EVP_md5(), data);
  }

  std::string sha1_hex(std::string_view data)
  {
    return digest_hex(EVP_sha1(), data);
  }

  std::string sha256_hex(std::string_view data)
  {
    return digest_hex(EVP_sha256(), data);
  }

  std::string hmac_md5_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(EVP_md5(), key, data);
  }

  std::string hmac_sha1_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(EVP_sha1(), key, data);
  }

  std::string hmac_sha256_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(EVP_sha256(), key, data);
  }

  std::string hmac_sha512_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(EVP_sha512(), key, data);
  }

  bool hmac_equal(std::string_view mac1, std::string_view mac2)
  {
    return hmac_equal_impl(mac1, mac2);
  }

  // ── Base64url ──

  std::string base64url_encode_nopad(std::string_view data)
  {
    return base64url_encode_nopad_impl(data);
  }

  std::string base64url_decode(std::string_view data)
  {
    return base64url_decode_impl(data);
  }

  // ── Algorithm parsing ──

  Algorithm parse_algorithm(std::string_view name)
  {
    return parse_algorithm_impl(name);
  }

  // ── Key parsing helpers ──

  using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
  using BIO_ptr = std::unique_ptr<BIO, decltype(&BIO_free)>;
  using BIGNUM_ptr = std::unique_ptr<BIGNUM, decltype(&BN_free)>;
  using X509_ptr = std::unique_ptr<X509, decltype(&X509_free)>;

  EVP_PKEY_ptr pkey_from_pem_pubkey(std::string_view pem)
  {
    BIO_ptr bio = bio_from_mem(pem.data(), pem.size());
    if (!bio)
    {
      return {nullptr, EVP_PKEY_free};
    }
    EVP_PKEY* key = PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);
    return {key, EVP_PKEY_free};
  }

  EVP_PKEY_ptr pkey_from_certificate(std::string_view cert_pem)
  {
    BIO_ptr bio = bio_from_mem(cert_pem.data(), cert_pem.size());
    if (!bio)
    {
      return {nullptr, EVP_PKEY_free};
    }
    X509_ptr cert(
      PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr), X509_free);
    if (!cert)
    {
      return {nullptr, EVP_PKEY_free};
    }
    EVP_PKEY* key = X509_get_pubkey(cert.get());
    return {key, EVP_PKEY_free};
  }

  // Decode a base64url-encoded big integer into a BIGNUM
  BIGNUM_ptr bn_from_base64url(std::string_view b64)
  {
    crypto_core::SecureString raw(::base64_decode(b64));
    BIGNUM* bn = BN_bin2bn(
      reinterpret_cast<const unsigned char*>(raw.data()), raw.size(), nullptr);
    return {bn, BN_free};
  }

  // Parse a JWK RSA key (kty=RSA) into an EVP_PKEY (public key only)
  // Uses OpenSSL 3.0 EVP_PKEY_fromdata API (no deprecated
  // RSA_new/RSA_set0_key).
  EVP_PKEY_ptr pkey_from_jwk_rsa(std::string_view n_b64, std::string_view e_b64)
  {
    BIGNUM_ptr n = bn_from_base64url(n_b64);
    BIGNUM_ptr e = bn_from_base64url(e_b64);
    if (!n || !e)
    {
      return {nullptr, EVP_PKEY_free};
    }

    std::unique_ptr<OSSL_PARAM_BLD, decltype(&OSSL_PARAM_BLD_free)> bld(
      OSSL_PARAM_BLD_new(), OSSL_PARAM_BLD_free);
    if (!bld)
    {
      return {nullptr, EVP_PKEY_free};
    }
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_N, n.get());
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_E, e.get());

    OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(bld.get());
    if (!params)
    {
      return {nullptr, EVP_PKEY_free};
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
      EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr), EVP_PKEY_CTX_free);
    if (!ctx || EVP_PKEY_fromdata_init(ctx.get()) != 1)
    {
      OSSL_PARAM_free(params);
      return {nullptr, EVP_PKEY_free};
    }

    EVP_PKEY* raw_pkey = nullptr;
    int rc =
      EVP_PKEY_fromdata(ctx.get(), &raw_pkey, EVP_PKEY_PUBLIC_KEY, params);
    OSSL_PARAM_free(params);
    if (rc != 1 || !raw_pkey)
    {
      return {nullptr, EVP_PKEY_free};
    }
    return {raw_pkey, EVP_PKEY_free};
  }

  // Map JWK curve name to OpenSSL group name for EVP_PKEY_fromdata.
  const char* ec_group_name(std::string_view crv)
  {
    if (crv == "P-256")
      return SN_X9_62_prime256v1;
    if (crv == "P-384")
      return SN_secp384r1;
    if (crv == "P-521")
      return SN_secp521r1;
    return nullptr;
  }

  // Parse a JWK EC key (kty=EC) into an EVP_PKEY (public key only)
  // Uses OpenSSL 3.0 EVP_PKEY_fromdata API (no deprecated EC_KEY*).
  EVP_PKEY_ptr pkey_from_jwk_ec(
    std::string_view crv, std::string_view x_b64, std::string_view y_b64)
  {
    const char* group_name = ec_group_name(crv);
    if (!group_name)
    {
      return {nullptr, EVP_PKEY_free};
    }

    std::string x_raw = ::base64_decode(x_b64);
    std::string y_raw = ::base64_decode(y_b64);

    // Build uncompressed point: 0x04 || x || y
    std::vector<unsigned char> point;
    point.reserve(1 + x_raw.size() + y_raw.size());
    point.push_back(0x04);
    point.insert(
      point.end(),
      reinterpret_cast<const unsigned char*>(x_raw.data()),
      reinterpret_cast<const unsigned char*>(x_raw.data()) + x_raw.size());
    point.insert(
      point.end(),
      reinterpret_cast<const unsigned char*>(y_raw.data()),
      reinterpret_cast<const unsigned char*>(y_raw.data()) + y_raw.size());

    std::unique_ptr<OSSL_PARAM_BLD, decltype(&OSSL_PARAM_BLD_free)> bld(
      OSSL_PARAM_BLD_new(), OSSL_PARAM_BLD_free);
    if (!bld)
    {
      return {nullptr, EVP_PKEY_free};
    }
    OSSL_PARAM_BLD_push_utf8_string(
      bld.get(), OSSL_PKEY_PARAM_GROUP_NAME, group_name, 0);
    OSSL_PARAM_BLD_push_octet_string(
      bld.get(), OSSL_PKEY_PARAM_PUB_KEY, point.data(), point.size());

    OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(bld.get());
    if (!params)
    {
      return {nullptr, EVP_PKEY_free};
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
      EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr), EVP_PKEY_CTX_free);
    if (!ctx || EVP_PKEY_fromdata_init(ctx.get()) != 1)
    {
      OSSL_PARAM_free(params);
      return {nullptr, EVP_PKEY_free};
    }

    EVP_PKEY* raw_pkey = nullptr;
    int rc =
      EVP_PKEY_fromdata(ctx.get(), &raw_pkey, EVP_PKEY_PUBLIC_KEY, params);
    OSSL_PARAM_free(params);
    if (rc != 1 || !raw_pkey)
    {
      return {nullptr, EVP_PKEY_free};
    }
    return {raw_pkey, EVP_PKEY_free};
  }

  // Parse a JWK OKP key (kty=OKP, crv=Ed25519) into an EVP_PKEY
  EVP_PKEY_ptr pkey_from_jwk_okp(std::string_view x_b64)
  {
    std::string x_raw = ::base64_decode(x_b64);
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
      EVP_PKEY_ED25519,
      nullptr,
      reinterpret_cast<const unsigned char*>(x_raw.data()),
      x_raw.size());
    return {pkey, EVP_PKEY_free};
  }

  using crypto_core::extract_jwks_keys;
  using crypto_core::json_select_string;
  using crypto_core::parse_json;

  // Parse a JWK JSON AST into an EVP_PKEY (public key)
  EVP_PKEY_ptr pkey_from_jwk_ast(const trieste::Node& ast)
  {
    using rego::crypto_core::MaxECComponentB64Len;
    using rego::crypto_core::MaxOKPComponentB64Len;
    using rego::crypto_core::MaxRSAComponentB64Len;
    std::string_view kty = json_select_string(ast, "/kty");
    if (kty == "RSA")
    {
      std::string_view n = json_select_string(ast, "/n");
      std::string_view e = json_select_string(ast, "/e");
      if (
        n.empty() || e.empty() || n.size() > MaxRSAComponentB64Len ||
        e.size() > MaxRSAComponentB64Len)
      {
        return {nullptr, EVP_PKEY_free};
      }
      return pkey_from_jwk_rsa(n, e);
    }
    if (kty == "EC")
    {
      std::string_view crv = json_select_string(ast, "/crv");
      std::string_view x = json_select_string(ast, "/x");
      std::string_view y = json_select_string(ast, "/y");
      if (
        crv.empty() || x.empty() || y.empty() ||
        x.size() > MaxECComponentB64Len || y.size() > MaxECComponentB64Len)
      {
        return {nullptr, EVP_PKEY_free};
      }
      return pkey_from_jwk_ec(crv, x, y);
    }
    if (kty == "OKP")
    {
      std::string_view x = json_select_string(ast, "/x");
      if (x.empty() || x.size() > MaxOKPComponentB64Len)
      {
        return {nullptr, EVP_PKEY_free};
      }
      return pkey_from_jwk_okp(x);
    }
    return {nullptr, EVP_PKEY_free};
  }

  // Parse a JWK JSON string into an EVP_PKEY (public key)
  EVP_PKEY_ptr pkey_from_jwk(std::string_view jwk_json)
  {
    auto ast = parse_json(jwk_json);
    return pkey_from_jwk_ast(ast);
  }

  // Auto-detect key format and load EVP_PKEY.
  // Handles: PEM cert, PEM pubkey, JWK object, JWKS set.
  EVP_PKEY_ptr load_public_key(
    std::string_view key_or_cert, std::string_view kid = {})
  {
    // PEM certificate
    if (
      key_or_cert.find("-----BEGIN CERTIFICATE-----") != std::string_view::npos)
    {
      return pkey_from_certificate(key_or_cert);
    }

    // PEM public key
    if (
      key_or_cert.find("-----BEGIN PUBLIC KEY-----") != std::string_view::npos)
    {
      return pkey_from_pem_pubkey(key_or_cert);
    }

    // Try JSON (JWK or JWKS)
    auto ast = parse_json(key_or_cert);
    if (!ast)
    {
      return {nullptr, EVP_PKEY_free};
    }

    // Try JWKS first — look for "keys" array
    auto keys = extract_jwks_keys(ast);
    if (!keys.empty())
    {
      // If kid specified, find matching key; otherwise use first
      for (auto& key_ast : keys)
      {
        if (kid.empty())
        {
          return pkey_from_jwk_ast(key_ast);
        }
        std::string_view key_kid = json_select_string(key_ast, "/kid");
        if (key_kid == kid)
        {
          return pkey_from_jwk_ast(key_ast);
        }
      }
      return {nullptr, EVP_PKEY_free};
    }

    // Try single JWK object
    std::string_view kty = json_select_string(ast, "/kty");
    if (!kty.empty())
    {
      return pkey_from_jwk_ast(ast);
    }

    return {nullptr, EVP_PKEY_free};
  }

  // Get the EVP_MD for an algorithm (nullptr for EdDSA which uses no digest)
  const EVP_MD* md_for_algo(Algorithm algo)
  {
    switch (algo)
    {
      case Algorithm::HS256:
      case Algorithm::RS256:
      case Algorithm::PS256:
      case Algorithm::ES256:
        return EVP_sha256();
      case Algorithm::HS384:
      case Algorithm::RS384:
      case Algorithm::PS384:
      case Algorithm::ES384:
        return EVP_sha384();
      case Algorithm::HS512:
      case Algorithm::RS512:
      case Algorithm::PS512:
      case Algorithm::ES512:
        return EVP_sha512();
      case Algorithm::EdDSA:
        return nullptr;
    }
    return nullptr;
  }

  bool verify_hmac(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view sig_bytes,
    std::string_view secret)
  {
    const EVP_MD* md = md_for_algo(algo);
    unsigned char buf[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    unsigned char* result = HMAC(
      md,
      secret.data(),
      static_cast<int>(secret.size()),
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      buf,
      &len);
    if (!result)
    {
      return false;
    }

    // Constant-time comparison
    if (len != sig_bytes.size())
    {
      return false;
    }
    return CRYPTO_memcmp(buf, sig_bytes.data(), len) == 0;
  }

  bool verify_rsa_pkcs1(
    const EVP_MD* md,
    EVP_PKEY* pkey,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
      EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx)
    {
      return false;
    }
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, md, nullptr, pkey) != 1)
    {
      return false;
    }
    return EVP_DigestVerify(
             ctx.get(),
             reinterpret_cast<const unsigned char*>(sig_bytes.data()),
             sig_bytes.size(),
             reinterpret_cast<const unsigned char*>(signing_input.data()),
             signing_input.size()) == 1;
  }

  bool verify_rsa_pss(
    const EVP_MD* md,
    EVP_PKEY* pkey,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
      EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx)
    {
      return false;
    }
    EVP_PKEY_CTX* pctx = nullptr;
    if (EVP_DigestVerifyInit(ctx.get(), &pctx, md, nullptr, pkey) != 1)
    {
      return false;
    }
    if (EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING) != 1)
    {
      return false;
    }
    if (EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, RSA_PSS_SALTLEN_AUTO) != 1)
    {
      return false;
    }
    return EVP_DigestVerify(
             ctx.get(),
             reinterpret_cast<const unsigned char*>(sig_bytes.data()),
             sig_bytes.size(),
             reinterpret_cast<const unsigned char*>(signing_input.data()),
             signing_input.size()) == 1;
  }

  // Wrapper for OPENSSL_free (which is a macro and can't have its address
  // taken)
  void openssl_free(unsigned char* ptr)
  {
    OPENSSL_free(ptr);
  }

  bool verify_ecdsa(
    const EVP_MD* md,
    EVP_PKEY* pkey,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    // Validate signature length against the expected curve size.
    // ES256 (SHA-256, P-256): 64 bytes, ES384 (SHA-384, P-384): 96 bytes,
    // ES512 (SHA-512, P-521): 132 bytes.
    size_t expected_sig_len = 0;
    int md_nid = EVP_MD_nid(md);
    switch (md_nid)
    {
      case NID_sha256:
        expected_sig_len = 64;
        break;
      case NID_sha384:
        expected_sig_len = 96;
        break;
      case NID_sha512:
        expected_sig_len = 132;
        break;
      default:
        return false;
    }
    if (sig_bytes.empty() || sig_bytes.size() != expected_sig_len)
    {
      return false;
    }

    // ECDSA JWT signatures are in R||S raw format; OpenSSL expects DER.
    // Convert raw (r || s) to DER-encoded signature.
    size_t half = sig_bytes.size() / 2;
    BIGNUM_ptr r(
      BN_bin2bn(
        reinterpret_cast<const unsigned char*>(sig_bytes.data()),
        half,
        nullptr),
      BN_free);
    BIGNUM_ptr s(
      BN_bin2bn(
        reinterpret_cast<const unsigned char*>(sig_bytes.data()) + half,
        half,
        nullptr),
      BN_free);
    if (!r || !s)
    {
      return false;
    }

    ECDSA_SIG* ecdsa_sig = ECDSA_SIG_new();
    // ECDSA_SIG_set0 takes ownership
    ECDSA_SIG_set0(ecdsa_sig, r.release(), s.release());

    unsigned char* der = nullptr;
    int der_len = i2d_ECDSA_SIG(ecdsa_sig, &der);
    ECDSA_SIG_free(ecdsa_sig);
    if (der_len <= 0)
    {
      return false;
    }

    std::unique_ptr<unsigned char, decltype(&openssl_free)> der_guard(
      der, openssl_free);

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
      EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx)
    {
      return false;
    }
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, md, nullptr, pkey) != 1)
    {
      return false;
    }
    return EVP_DigestVerify(
             ctx.get(),
             der,
             der_len,
             reinterpret_cast<const unsigned char*>(signing_input.data()),
             signing_input.size()) == 1;
  }

  bool verify_eddsa(
    EVP_PKEY* pkey, std::string_view signing_input, std::string_view sig_bytes)
  {
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
      EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx)
    {
      return false;
    }
    // EdDSA: md is NULL
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey) != 1)
    {
      return false;
    }
    return EVP_DigestVerify(
             ctx.get(),
             reinterpret_cast<const unsigned char*>(sig_bytes.data()),
             sig_bytes.size(),
             reinterpret_cast<const unsigned char*>(signing_input.data()),
             signing_input.size()) == 1;
  }

  // ── Signature verification dispatch ──

  // Validate and classify PEM data. Returns an error string if the PEM
  // is malformed, or empty string if OK (or not PEM at all).
  VerifyResult verify_signature(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view signature_bytes,
    std::string_view key_or_cert)
  {
    // HMAC algorithms use the key directly as a secret
    if (
      algo == Algorithm::HS256 || algo == Algorithm::HS384 ||
      algo == Algorithm::HS512)
    {
      bool ok = verify_hmac(algo, signing_input, signature_bytes, key_or_cert);
      return {ok, {}};
    }

    // Validate PEM structure before attempting to parse
    std::string pem_err = validate_pem(key_or_cert);
    if (!pem_err.empty())
    {
      return {false, pem_err};
    }

    // Asymmetric: load the public key
    EVP_PKEY_ptr pkey = load_public_key(key_or_cert);
    if (!pkey)
    {
      // Determine what kind of key was attempted
      if (
        key_or_cert.find("-----BEGIN CERTIFICATE-----") !=
        std::string_view::npos)
      {
        // Valid PEM structure (passed validate_pem) but OpenSSL couldn't parse
        return {false, "failed to parse a PEM certificate"};
      }
      if (
        key_or_cert.find("-----BEGIN PUBLIC KEY-----") !=
        std::string_view::npos)
      {
        return {false, "failed to parse a PEM key"};
      }
      if (
        key_or_cert.find("\"kty\"") != std::string_view::npos ||
        key_or_cert.find("\"keys\"") != std::string_view::npos)
      {
        return {false, "failed to parse a JWK key (set)"};
      }
      return {false, {}};
    }

    const EVP_MD* md = md_for_algo(algo);
    bool ok = false;

    switch (algo)
    {
      case Algorithm::RS256:
      case Algorithm::RS384:
      case Algorithm::RS512:
        ok = verify_rsa_pkcs1(md, pkey.get(), signing_input, signature_bytes);
        break;

      case Algorithm::PS256:
      case Algorithm::PS384:
      case Algorithm::PS512:
        ok = verify_rsa_pss(md, pkey.get(), signing_input, signature_bytes);
        break;

      case Algorithm::ES256:
      case Algorithm::ES384:
      case Algorithm::ES512:
        ok = verify_ecdsa(md, pkey.get(), signing_input, signature_bytes);
        break;

      case Algorithm::EdDSA:
        ok = verify_eddsa(pkey.get(), signing_input, signature_bytes);
        break;

      default:
        break;
    }

    return {ok, {}};
  }

  VerifyResult verify_signature_any_key(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view signature_bytes,
    std::string_view key_or_cert)
  {
    // Parse to check if this is a JWKS with multiple keys
    auto ast = parse_json(key_or_cert);
    if (!ast)
    {
      // Not JSON — use normal path (may be PEM)
      return verify_signature(
        algo, signing_input, signature_bytes, key_or_cert);
    }

    auto keys = extract_jwks_keys(ast);
    if (keys.size() <= 1)
    {
      // Not a JWKS or single key — use normal path
      return verify_signature(
        algo, signing_input, signature_bytes, key_or_cert);
    }

    // Try each key; return success on first valid signature
    for (auto& key_ast : keys)
    {
      auto pkey = pkey_from_jwk_ast(key_ast);
      if (!pkey)
      {
        continue;
      }

      const EVP_MD* md = md_for_algo(algo);
      bool ok = false;

      switch (algo)
      {
        case Algorithm::RS256:
        case Algorithm::RS384:
        case Algorithm::RS512:
          ok = verify_rsa_pkcs1(md, pkey.get(), signing_input, signature_bytes);
          break;
        case Algorithm::PS256:
        case Algorithm::PS384:
        case Algorithm::PS512:
          ok = verify_rsa_pss(md, pkey.get(), signing_input, signature_bytes);
          break;
        case Algorithm::ES256:
        case Algorithm::ES384:
        case Algorithm::ES512:
          ok = verify_ecdsa(md, pkey.get(), signing_input, signature_bytes);
          break;
        case Algorithm::EdDSA:
          ok = verify_eddsa(pkey.get(), signing_input, signature_bytes);
          break;
        default:
          break;
      }

      if (ok)
      {
        return {true, {}};
      }
    }

    return {false, {}};
  }

  // ── Private key loading (for signing) ──

  EVP_PKEY_ptr pkey_from_jwk_rsa_private(
    std::string_view n_b64,
    std::string_view e_b64,
    std::string_view d_b64,
    std::string_view p_b64,
    std::string_view q_b64,
    std::string_view dp_b64,
    std::string_view dq_b64,
    std::string_view qi_b64)
  {
    BIGNUM_ptr n = bn_from_base64url(n_b64);
    BIGNUM_ptr e = bn_from_base64url(e_b64);
    BIGNUM_ptr d = bn_from_base64url(d_b64);
    BIGNUM_ptr p = bn_from_base64url(p_b64);
    BIGNUM_ptr q = bn_from_base64url(q_b64);
    BIGNUM_ptr dp = bn_from_base64url(dp_b64);
    BIGNUM_ptr dq = bn_from_base64url(dq_b64);
    BIGNUM_ptr qi = bn_from_base64url(qi_b64);
    if (!n || !e || !d || !p || !q || !dp || !dq || !qi)
    {
      return {nullptr, EVP_PKEY_free};
    }

    std::unique_ptr<OSSL_PARAM_BLD, decltype(&OSSL_PARAM_BLD_free)> bld(
      OSSL_PARAM_BLD_new(), OSSL_PARAM_BLD_free);
    if (!bld)
    {
      return {nullptr, EVP_PKEY_free};
    }
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_N, n.get());
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_E, e.get());
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_D, d.get());
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_FACTOR1, p.get());
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_FACTOR2, q.get());
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_EXPONENT1, dp.get());
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_EXPONENT2, dq.get());
    OSSL_PARAM_BLD_push_BN(
      bld.get(), OSSL_PKEY_PARAM_RSA_COEFFICIENT1, qi.get());

    OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(bld.get());
    if (!params)
    {
      return {nullptr, EVP_PKEY_free};
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
      EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr), EVP_PKEY_CTX_free);
    if (!ctx || EVP_PKEY_fromdata_init(ctx.get()) != 1)
    {
      OSSL_PARAM_free(params);
      return {nullptr, EVP_PKEY_free};
    }

    EVP_PKEY* raw_pkey = nullptr;
    int rc = EVP_PKEY_fromdata(ctx.get(), &raw_pkey, EVP_PKEY_KEYPAIR, params);
    OSSL_PARAM_free(params);
    if (rc != 1 || !raw_pkey)
    {
      return {nullptr, EVP_PKEY_free};
    }
    return {raw_pkey, EVP_PKEY_free};
  }

  EVP_PKEY_ptr pkey_from_jwk_ec_private(
    std::string_view crv,
    std::string_view x_b64,
    std::string_view y_b64,
    std::string_view d_b64)
  {
    const char* group_name = ec_group_name(crv);
    if (!group_name)
    {
      return {nullptr, EVP_PKEY_free};
    }

    crypto_core::SecureString x_raw(::base64_decode(x_b64));
    crypto_core::SecureString y_raw(::base64_decode(y_b64));
    crypto_core::SecureString d_raw(::base64_decode(d_b64));

    // Build uncompressed point: 0x04 || x || y
    std::vector<unsigned char> point;
    point.reserve(1 + x_raw.size() + y_raw.size());
    point.push_back(0x04);
    point.insert(
      point.end(),
      reinterpret_cast<const unsigned char*>(x_raw.data()),
      reinterpret_cast<const unsigned char*>(x_raw.data()) + x_raw.size());
    point.insert(
      point.end(),
      reinterpret_cast<const unsigned char*>(y_raw.data()),
      reinterpret_cast<const unsigned char*>(y_raw.data()) + y_raw.size());

    BIGNUM_ptr d_bn(
      BN_bin2bn(
        reinterpret_cast<const unsigned char*>(d_raw.data()),
        d_raw.size(),
        nullptr),
      BN_free);
    if (!d_bn)
    {
      return {nullptr, EVP_PKEY_free};
    }

    std::unique_ptr<OSSL_PARAM_BLD, decltype(&OSSL_PARAM_BLD_free)> bld(
      OSSL_PARAM_BLD_new(), OSSL_PARAM_BLD_free);
    if (!bld)
    {
      return {nullptr, EVP_PKEY_free};
    }
    OSSL_PARAM_BLD_push_utf8_string(
      bld.get(), OSSL_PKEY_PARAM_GROUP_NAME, group_name, 0);
    OSSL_PARAM_BLD_push_octet_string(
      bld.get(), OSSL_PKEY_PARAM_PUB_KEY, point.data(), point.size());
    OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_PRIV_KEY, d_bn.get());

    OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(bld.get());
    if (!params)
    {
      return {nullptr, EVP_PKEY_free};
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
      EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr), EVP_PKEY_CTX_free);
    if (!ctx || EVP_PKEY_fromdata_init(ctx.get()) != 1)
    {
      OSSL_PARAM_free(params);
      return {nullptr, EVP_PKEY_free};
    }

    EVP_PKEY* raw_pkey = nullptr;
    int rc = EVP_PKEY_fromdata(ctx.get(), &raw_pkey, EVP_PKEY_KEYPAIR, params);
    OSSL_PARAM_free(params);
    if (rc != 1 || !raw_pkey)
    {
      return {nullptr, EVP_PKEY_free};
    }
    return {raw_pkey, EVP_PKEY_free};
  }

  EVP_PKEY_ptr pkey_from_jwk_okp_private(std::string_view d_b64)
  {
    crypto_core::SecureString d_raw(::base64_decode(d_b64));
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
      EVP_PKEY_ED25519,
      nullptr,
      reinterpret_cast<const unsigned char*>(d_raw.data()),
      d_raw.size());
    return {pkey, EVP_PKEY_free};
  }

  // Load a private key from a JWK JSON AST
  EVP_PKEY_ptr load_private_key_ast(const trieste::Node& ast)
  {
    using rego::crypto_core::MaxECComponentB64Len;
    using rego::crypto_core::MaxOKPComponentB64Len;
    using rego::crypto_core::MaxRSAComponentB64Len;
    std::string_view kty = json_select_string(ast, "/kty");
    if (kty == "RSA")
    {
      std::string_view n = json_select_string(ast, "/n");
      std::string_view e = json_select_string(ast, "/e");
      std::string_view d = json_select_string(ast, "/d");
      std::string_view p = json_select_string(ast, "/p");
      std::string_view q = json_select_string(ast, "/q");
      std::string_view dp = json_select_string(ast, "/dp");
      std::string_view dq = json_select_string(ast, "/dq");
      std::string_view qi = json_select_string(ast, "/qi");
      if (n.empty() || e.empty() || d.empty())
      {
        return {nullptr, EVP_PKEY_free};
      }
      for (auto sv : {n, e, d, p, q, dp, dq, qi})
      {
        if (sv.size() > MaxRSAComponentB64Len)
        {
          return {nullptr, EVP_PKEY_free};
        }
      }
      return pkey_from_jwk_rsa_private(n, e, d, p, q, dp, dq, qi);
    }
    if (kty == "EC")
    {
      std::string_view crv = json_select_string(ast, "/crv");
      std::string_view x = json_select_string(ast, "/x");
      std::string_view y = json_select_string(ast, "/y");
      std::string_view d = json_select_string(ast, "/d");
      if (crv.empty() || x.empty() || y.empty() || d.empty())
      {
        return {nullptr, EVP_PKEY_free};
      }
      if (
        x.size() > MaxECComponentB64Len || y.size() > MaxECComponentB64Len ||
        d.size() > MaxECComponentB64Len)
      {
        return {nullptr, EVP_PKEY_free};
      }
      return pkey_from_jwk_ec_private(crv, x, y, d);
    }
    if (kty == "OKP")
    {
      std::string_view d = json_select_string(ast, "/d");
      if (d.empty() || d.size() > MaxOKPComponentB64Len)
      {
        return {nullptr, EVP_PKEY_free};
      }
      return pkey_from_jwk_okp_private(d);
    }
    return {nullptr, EVP_PKEY_free};
  }

  // Load a private key from a JWK JSON string
  EVP_PKEY_ptr load_private_key(std::string_view jwk_json)
  {
    auto ast = parse_json(jwk_json);
    return load_private_key_ast(ast);
  }

  // ── Signing functions ──

  std::string sign_hmac(
    Algorithm algo, std::string_view signing_input, std::string_view secret)
  {
    const EVP_MD* md = md_for_algo(algo);
    unsigned char buf[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    unsigned char* result = HMAC(
      md,
      secret.data(),
      static_cast<int>(secret.size()),
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      buf,
      &len);
    if (!result)
    {
      throw std::runtime_error("HMAC signing failed");
    }
    return std::string(reinterpret_cast<char*>(buf), len);
  }

  std::string sign_asymmetric(
    const EVP_MD* md,
    EVP_PKEY* pkey,
    std::string_view signing_input,
    int padding = 0)
  {
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
      EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx)
    {
      throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    EVP_PKEY_CTX* pctx = nullptr;
    if (EVP_DigestSignInit(ctx.get(), &pctx, md, nullptr, pkey) != 1)
    {
      throw std::runtime_error("EVP_DigestSignInit failed");
    }

    if (padding == RSA_PKCS1_PSS_PADDING)
    {
      EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING);
      EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, RSA_PSS_SALTLEN_DIGEST);
    }

    // Determine signature length
    size_t sig_len = 0;
    if (
      EVP_DigestSign(
        ctx.get(),
        nullptr,
        &sig_len,
        reinterpret_cast<const unsigned char*>(signing_input.data()),
        signing_input.size()) != 1)
    {
      throw std::runtime_error("EVP_DigestSign (length) failed");
    }

    std::vector<unsigned char> sig(sig_len);
    if (
      EVP_DigestSign(
        ctx.get(),
        sig.data(),
        &sig_len,
        reinterpret_cast<const unsigned char*>(signing_input.data()),
        signing_input.size()) != 1)
    {
      throw std::runtime_error("EVP_DigestSign failed");
    }
    sig.resize(sig_len);
    return std::string(reinterpret_cast<char*>(sig.data()), sig.size());
  }

  // Convert DER-encoded ECDSA signature to raw R||S format
  std::string ecdsa_der_to_raw(
    const std::string& der_sig, size_t component_size)
  {
    const unsigned char* p =
      reinterpret_cast<const unsigned char*>(der_sig.data());
    ECDSA_SIG* ecdsa_sig = d2i_ECDSA_SIG(nullptr, &p, der_sig.size());
    if (!ecdsa_sig)
    {
      throw std::runtime_error("failed to decode ECDSA DER signature");
    }

    const BIGNUM* r = nullptr;
    const BIGNUM* s = nullptr;
    ECDSA_SIG_get0(ecdsa_sig, &r, &s);

    std::vector<unsigned char> raw(component_size * 2, 0);
    BN_bn2binpad(r, raw.data(), component_size);
    BN_bn2binpad(s, raw.data() + component_size, component_size);
    ECDSA_SIG_free(ecdsa_sig);

    return std::string(reinterpret_cast<char*>(raw.data()), raw.size());
  }

  size_t ecdsa_component_size(Algorithm algo)
  {
    switch (algo)
    {
      case Algorithm::ES256:
        return 32;
      case Algorithm::ES384:
        return 48;
      case Algorithm::ES512:
        return 66;
      default:
        return 0;
    }
  }

  // ── sign() dispatch ──

  std::string sign(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view key_jwk_json)
  {
    auto ast = parse_json(key_jwk_json);
    if (!ast)
    {
      throw std::runtime_error("failed to parse JWK JSON");
    }

    // HMAC: extract the "k" field (base64url-encoded secret)
    if (
      algo == Algorithm::HS256 || algo == Algorithm::HS384 ||
      algo == Algorithm::HS512)
    {
      std::string_view k = json_select_string(ast, "/k");
      if (k.empty())
      {
        throw std::runtime_error("missing 'k' in oct JWK");
      }
      crypto_core::SecureString secret(::base64_decode(k));
      return sign_hmac(algo, signing_input, secret.value);
    }

    // Asymmetric: load private key
    EVP_PKEY_ptr pkey = load_private_key_ast(ast);
    if (!pkey)
    {
      throw std::runtime_error("failed to load private key from JWK");
    }

    const EVP_MD* md = md_for_algo(algo);

    switch (algo)
    {
      case Algorithm::RS256:
      case Algorithm::RS384:
      case Algorithm::RS512:
        return sign_asymmetric(md, pkey.get(), signing_input);

      case Algorithm::PS256:
      case Algorithm::PS384:
      case Algorithm::PS512:
        return sign_asymmetric(
          md, pkey.get(), signing_input, RSA_PKCS1_PSS_PADDING);

      case Algorithm::ES256:
      case Algorithm::ES384:
      case Algorithm::ES512: {
        std::string der = sign_asymmetric(md, pkey.get(), signing_input);
        return ecdsa_der_to_raw(der, ecdsa_component_size(algo));
      }

      case Algorithm::EdDSA:
        return sign_asymmetric(nullptr, pkey.get(), signing_input);

      default:
        throw std::runtime_error("unsupported algorithm for signing");
    }
  }

  // ── X.509 Certificate Parsing ──

  using X509_STORE_ptr =
    std::unique_ptr<X509_STORE, decltype(&X509_STORE_free)>;
  using X509_STORE_CTX_ptr =
    std::unique_ptr<X509_STORE_CTX, decltype(&X509_STORE_CTX_free)>;
  using X509_REQ_ptr = std::unique_ptr<X509_REQ, decltype(&X509_REQ_free)>;

  // Extract CommonName from an X509_NAME
  std::string get_common_name(X509_NAME* name)
  {
    if (!name)
    {
      return {};
    }
    int idx = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
    if (idx < 0)
    {
      return {};
    }
    X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, idx);
    if (!entry)
    {
      return {};
    }
    ASN1_STRING* data = X509_NAME_ENTRY_get_data(entry);
    if (!data)
    {
      return {};
    }
    unsigned char* utf8 = nullptr;
    int len = ASN1_STRING_to_UTF8(&utf8, data);
    if (len < 0)
    {
      return {};
    }
    std::string result(reinterpret_cast<char*>(utf8), static_cast<size_t>(len));
    OPENSSL_free(utf8);
    return result;
  }

  // Extract DNS names and URI strings from certificate SANs
  void extract_sans(
    X509* cert,
    std::vector<std::string>& dns_names,
    std::vector<std::string>& uri_strings)
  {
    GENERAL_NAMES* sans = static_cast<GENERAL_NAMES*>(
      X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));
    if (!sans)
    {
      return;
    }

    for (int i = 0; i < sk_GENERAL_NAME_num(sans); ++i)
    {
      GENERAL_NAME* gen = sk_GENERAL_NAME_value(sans, i);
      if (gen->type == GEN_DNS)
      {
        unsigned char* utf8 = nullptr;
        int len = ASN1_STRING_to_UTF8(&utf8, gen->d.dNSName);
        if (len >= 0)
        {
          dns_names.emplace_back(
            reinterpret_cast<char*>(utf8), static_cast<size_t>(len));
          OPENSSL_free(utf8);
        }
      }
      else if (gen->type == GEN_URI)
      {
        unsigned char* utf8 = nullptr;
        int len = ASN1_STRING_to_UTF8(&utf8, gen->d.uniformResourceIdentifier);
        if (len >= 0)
        {
          uri_strings.emplace_back(
            reinterpret_cast<char*>(utf8), static_cast<size_t>(len));
          OPENSSL_free(utf8);
        }
      }
    }

    GENERAL_NAMES_free(sans);
  }

  // Convert an X509* to base64-encoded DER
  std::string cert_to_der_b64(X509* cert)
  {
    unsigned char* buf = nullptr;
    int len = i2d_X509(cert, &buf);
    if (len <= 0)
    {
      return {};
    }
    std::string_view der(
      reinterpret_cast<char*>(buf), static_cast<size_t>(len));
    std::string b64 = ::base64_encode(der, false);
    OPENSSL_free(buf);
    return b64;
  }

  // Parse a single X509* into a ParsedCertificate
  ParsedCertificate cert_to_parsed(X509* cert)
  {
    ParsedCertificate pc;
    pc.subject.common_name = get_common_name(X509_get_subject_name(cert));
    extract_sans(cert, pc.dns_names, pc.uri_strings);
    pc.der_b64 = cert_to_der_b64(cert);
    return pc;
  }

  // Parse PEM data into X509 objects by extracting DER from PEM blocks
  // manually and feeding to d2i_X509, mirroring OPA's Go approach.
  std::vector<X509_ptr> parse_pem_certs(std::string_view pem_data)
  {
    std::vector<X509_ptr> certs;
    auto der_blocks = extract_pem_der_blocks(pem_data, "CERTIFICATE");

    for (auto& der : der_blocks)
    {
      const unsigned char* p =
        reinterpret_cast<const unsigned char*>(der.data());
      X509* cert = d2i_X509(nullptr, &p, static_cast<long>(der.size()));
      if (cert)
      {
        certs.emplace_back(cert, X509_free);
      }
      else
      {
        ERR_clear_error();
      }
    }
    return certs;
  }

  // Parse DER data into X509 objects (may be multiple concatenated)
  std::vector<X509_ptr> parse_der_certs(std::string_view der_data)
  {
    std::vector<X509_ptr> certs;
    const unsigned char* p =
      reinterpret_cast<const unsigned char*>(der_data.data());
    const unsigned char* end = p + der_data.size();

    while (p < end)
    {
      const unsigned char* start = p;
      X509* cert = d2i_X509(nullptr, &p, static_cast<long>(end - start));
      if (!cert)
      {
        ERR_clear_error();
        break;
      }
      certs.emplace_back(cert, X509_free);
    }
    return certs;
  }

  ParseCertsResult parse_certificates(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {{}, decoded.error};
    }

    std::vector<X509_ptr> certs;
    if (decoded.is_pem)
    {
      certs = parse_pem_certs(decoded.data);
    }
    else
    {
      certs = parse_der_certs(decoded.data);
    }

    if (certs.empty())
    {
      return {{}, "x509: malformed certificate"};
    }

    ParseCertsResult result;
    for (auto& cert : certs)
    {
      result.certs.push_back(cert_to_parsed(cert.get()));
    }
    return result;
  }

  ParseCSRResult parse_certificate_request(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {{}, decoded.error};
    }

    X509_REQ* req = nullptr;
    if (decoded.is_pem)
    {
      BIO_ptr bio = bio_from_mem(decoded.data.data(), decoded.data.size());
      if (bio)
      {
        req = PEM_read_bio_X509_REQ(bio.get(), nullptr, nullptr, nullptr);
      }
    }
    else
    {
      const unsigned char* p =
        reinterpret_cast<const unsigned char*>(decoded.data.data());
      req = d2i_X509_REQ(nullptr, &p, static_cast<long>(decoded.data.size()));
    }

    if (!req)
    {
      ERR_clear_error();
      return {{}, "asn1: structure error"};
    }

    X509_REQ_ptr req_ptr(req, X509_REQ_free);
    ParseCSRResult result;
    result.subject.common_name =
      get_common_name(X509_REQ_get_subject_name(req));
    return result;
  }

  VerifyCertsResult parse_and_verify_certificates(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {false, {}, decoded.error};
    }

    std::vector<X509_ptr> certs;
    if (decoded.is_pem)
    {
      certs = parse_pem_certs(decoded.data);
    }
    else
    {
      certs = parse_der_certs(decoded.data);
    }

    if (certs.size() < 2)
    {
      // Need at least a root CA and a leaf
      VerifyCertsResult result;
      result.valid = false;
      for (auto& cert : certs)
      {
        result.certs.push_back(cert_to_parsed(cert.get()));
      }
      return result;
    }

    // Build the trust store and intermediate chain.
    // OPA convention: last cert is leaf, first may be root, intermediates in
    // between. We try to verify the chain by adding potential CA certs to the
    // store and intermediates.
    X509_STORE_ptr store(X509_STORE_new(), X509_STORE_free);
    if (!store)
    {
      return {false, {}, "failed to create X509_STORE"};
    }

    // Add all certificates except the leaf as trusted or intermediates
    STACK_OF(X509)* chain = sk_X509_new_null();
    for (size_t i = 0; i + 1 < certs.size(); ++i)
    {
      // Check if it's self-signed (potential root)
      if (X509_check_issued(certs[i].get(), certs[i].get()) == X509_V_OK)
      {
        X509_STORE_add_cert(store.get(), certs[i].get());
      }
      else
      {
        sk_X509_push(chain, certs[i].get());
      }
    }

    // Leaf is the last certificate
    X509* leaf = certs.back().get();

    X509_STORE_CTX_ptr ctx(X509_STORE_CTX_new(), X509_STORE_CTX_free);
    if (!ctx || !X509_STORE_CTX_init(ctx.get(), store.get(), leaf, chain))
    {
      sk_X509_free(chain);
      return {false, {}, "failed to init X509_STORE_CTX"};
    }

    // NOTE: CRL and OCSP revocation checking is not performed (matching OPA).
    // A revoked certificate will be accepted if otherwise valid.
    int verify_ok = X509_verify_cert(ctx.get());
    sk_X509_free(chain);

    VerifyCertsResult result;
    result.valid = (verify_ok == 1);

    if (result.valid)
    {
      // Return the verified chain in leaf-first order (matching OPA behavior).
      // X509_STORE_CTX_get0_chain returns the chain built by OpenSSL:
      // leaf, intermediates..., root.
      STACK_OF(X509)* verified_chain = X509_STORE_CTX_get0_chain(ctx.get());
      if (verified_chain)
      {
        int chain_len = sk_X509_num(verified_chain);
        if (chain_len > static_cast<int>(MaxCertChainLen))
        {
          result.valid = false;
          result.certs.clear();
          for (auto& cert : certs)
          {
            result.certs.push_back(cert_to_parsed(cert.get()));
          }
          return result;
        }
        for (int i = 0; i < chain_len; ++i)
        {
          result.certs.push_back(
            cert_to_parsed(sk_X509_value(verified_chain, i)));
        }
      }
    }
    else
    {
      // On failure, return certs in input order
      for (auto& cert : certs)
      {
        result.certs.push_back(cert_to_parsed(cert.get()));
      }
    }
    return result;
  }

  // Extract an RSA BIGNUM component and return base64url-encoded (no padding)
  std::string bn_to_base64url(const BIGNUM* bn)
  {
    if (!bn)
    {
      return {};
    }
    int num_bytes = BN_num_bytes(bn);
    std::vector<unsigned char> buf(static_cast<size_t>(num_bytes));
    BN_bn2bin(bn, buf.data());
    std::string_view sv(reinterpret_cast<char*>(buf.data()), buf.size());
    return base64url_encode_nopad(sv);
  }

  ParseRSAKeyResult parse_rsa_private_key(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {{}, decoded.error};
    }

    EVP_PKEY* pkey = nullptr;
    if (decoded.is_pem)
    {
      BIO_ptr bio = bio_from_mem(decoded.data.data(), decoded.data.size());
      if (bio)
      {
        pkey = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
      }
    }
    else
    {
      const unsigned char* p =
        reinterpret_cast<const unsigned char*>(decoded.data.data());
      pkey = d2i_PrivateKey(
        EVP_PKEY_RSA, nullptr, &p, static_cast<long>(decoded.data.size()));
    }

    if (!pkey)
    {
      ERR_clear_error();
      return {{}, "failed to parse RSA private key"};
    }

    EVP_PKEY_ptr pkey_ptr(pkey, EVP_PKEY_free);

    // Extract RSA components using EVP_PKEY_get_bn_param (OpenSSL 3.0)
    BIGNUM* n_bn = nullptr;
    BIGNUM* e_bn = nullptr;
    BIGNUM* d_bn = nullptr;
    BIGNUM* p_bn = nullptr;
    BIGNUM* q_bn = nullptr;
    BIGNUM* dp_bn = nullptr;
    BIGNUM* dq_bn = nullptr;
    BIGNUM* qi_bn = nullptr;

    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_N, &n_bn);
    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_E, &e_bn);
    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_D, &d_bn);
    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_FACTOR1, &p_bn);
    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_FACTOR2, &q_bn);
    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_EXPONENT1, &dp_bn);
    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_EXPONENT2, &dq_bn);
    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_COEFFICIENT1, &qi_bn);

    RSAPrivateKeyJWK jwk;
    jwk.kty = "RSA";
    jwk.n = bn_to_base64url(n_bn);
    jwk.e = bn_to_base64url(e_bn);
    jwk.d = bn_to_base64url(d_bn);
    jwk.p = bn_to_base64url(p_bn);
    jwk.q = bn_to_base64url(q_bn);
    jwk.dp = bn_to_base64url(dp_bn);
    jwk.dq = bn_to_base64url(dq_bn);
    jwk.qi = bn_to_base64url(qi_bn);

    BN_free(n_bn);
    BN_free(e_bn);
    BN_free(d_bn);
    BN_free(p_bn);
    BN_free(q_bn);
    BN_free(dp_bn);
    BN_free(dq_bn);
    BN_free(qi_bn);

    return {jwk, {}};
  }

  ParsePrivateKeysResult parse_private_keys(std::string_view input)
  {
    if (input.empty())
    {
      return {{}, true, {}};
    }

    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      // For parse_private_keys, invalid input returns empty array, not error
      return {{}, false, {}};
    }

    if (!decoded.is_pem)
    {
      // Must be PEM for private keys
      return {{}, false, {}};
    }

    BIO_ptr bio = bio_from_mem(decoded.data.data(), decoded.data.size());
    if (!bio)
    {
      return {{}, false, {}};
    }

    ParsePrivateKeysResult result;
    result.is_empty_input = false;

    while (true)
    {
      EVP_PKEY* pkey =
        PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
      if (!pkey)
      {
        ERR_clear_error();
        break;
      }

      EVP_PKEY_ptr pkey_ptr(pkey, EVP_PKEY_free);

      if (EVP_PKEY_is_a(pkey, "RSA"))
      {
        BIGNUM* n_bn = nullptr;
        BIGNUM* e_bn = nullptr;
        BIGNUM* d_bn = nullptr;
        BIGNUM* p_bn = nullptr;
        BIGNUM* q_bn = nullptr;
        BIGNUM* dp_bn = nullptr;
        BIGNUM* dq_bn = nullptr;
        BIGNUM* qi_bn = nullptr;

        EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_N, &n_bn);
        EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_E, &e_bn);
        EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_D, &d_bn);
        EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_FACTOR1, &p_bn);
        EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_FACTOR2, &q_bn);
        EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_EXPONENT1, &dp_bn);
        EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_EXPONENT2, &dq_bn);
        EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_COEFFICIENT1, &qi_bn);

        RSAPrivateKeyJWK jwk;
        jwk.kty = "RSA";
        jwk.n = bn_to_base64url(n_bn);
        jwk.e = bn_to_base64url(e_bn);
        jwk.d = bn_to_base64url(d_bn);
        jwk.p = bn_to_base64url(p_bn);
        jwk.q = bn_to_base64url(q_bn);
        jwk.dp = bn_to_base64url(dp_bn);
        jwk.dq = bn_to_base64url(dq_bn);
        jwk.qi = bn_to_base64url(qi_bn);

        BN_free(n_bn);
        BN_free(e_bn);
        BN_free(d_bn);
        BN_free(p_bn);
        BN_free(q_bn);
        BN_free(dp_bn);
        BN_free(dq_bn);
        BN_free(qi_bn);

        result.keys.push_back(jwk);
      }
      // Note: EC/OKP keys could be added here in the future
    }

    return result;
  }
}

#endif // REGOCPP_CRYPTO_OPENSSL3
