// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#ifdef REGOCPP_HAS_CRYPTO

#include <string>
#include <string_view>
#include <vector>

namespace rego::crypto_core
{
  // ── Hashing ──
  std::string md5_hex(std::string_view data);
  std::string sha1_hex(std::string_view data);
  std::string sha256_hex(std::string_view data);

  // ── HMAC ──
  std::string hmac_md5_hex(std::string_view key, std::string_view data);
  std::string hmac_sha1_hex(std::string_view key, std::string_view data);
  std::string hmac_sha256_hex(std::string_view key, std::string_view data);
  std::string hmac_sha512_hex(std::string_view key, std::string_view data);
  bool hmac_equal(std::string_view mac1, std::string_view mac2);

  // ── Base64url ──
  std::string base64url_encode_nopad(std::string_view data);
  std::string base64url_decode(std::string_view data);

  // ── Signature Verification ──
  enum class Algorithm
  {
    HS256,
    HS384,
    HS512,
    RS256,
    RS384,
    RS512,
    PS256,
    PS384,
    PS512,
    ES256,
    ES384,
    ES512,
    EdDSA
  };

  // Returns the algorithm enum for a JWT "alg" header value.
  // Throws std::invalid_argument if unknown.
  Algorithm parse_algorithm(std::string_view name);

  // Result of signature verification.
  struct VerifyResult
  {
    bool valid; // true if signature verified successfully
    std::string error; // non-empty if key parsing / crypto failed (not a
                       // mismatch, but an actual error)
  };

  // Verify a JWT signature. key_or_cert is a PEM certificate, PEM public key,
  // or JWK JSON string. signing_input is "header.payload" (the first two
  // base64url sections). signature is the raw decoded signature bytes.
  VerifyResult verify_signature(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view signature_bytes,
    std::string_view key_or_cert);

  // Verify a JWT signature, trying all keys in a JWKS "keys" array.
  // Returns valid=true on the first key that succeeds.
  // If key_or_cert is not a JWKS, falls back to verify_signature.
  VerifyResult verify_signature_any_key(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view signature_bytes,
    std::string_view key_or_cert);

  // ── Signing ──

  // Sign data with a JWK private key.
  // Returns the raw signature bytes, or throws on error.
  // key_jwk_json is the full JWK JSON string containing private key material.
  std::string sign(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view key_jwk_json);

  // ── X.509 Certificate Parsing ──

  struct X509Name
  {
    std::string common_name;
    // Extend with Organization, Country, etc. as needed
  };

  struct ParsedCertificate
  {
    X509Name subject;
    std::vector<std::string> dns_names; // empty if none
    std::vector<std::string> uri_strings; // empty if none
    std::string der_b64; // base64-encoded DER (for keypair output)
  };

  struct ParseCertsResult
  {
    std::vector<ParsedCertificate> certs;
    std::string error; // non-empty on failure
  };

  // Parse one or more X.509 certificates from input.
  // Input can be: PEM string, base64-encoded PEM, base64-encoded DER, or
  // concatenated DER bytes.
  ParseCertsResult parse_certificates(std::string_view input);

  struct ParseCSRResult
  {
    X509Name subject;
    std::string error; // non-empty on failure
  };

  // Parse an X.509 Certificate Signing Request.
  ParseCSRResult parse_certificate_request(std::string_view input);

  struct VerifyCertsResult
  {
    bool valid;
    std::vector<ParsedCertificate> certs;
    std::string error; // non-empty on failure
  };

  // Parse and verify a certificate chain.
  // Follows OPA convention: the last certificate in the PEM bundle is treated
  // as the leaf (workload) certificate; all preceding certificates are treated
  // as CA or intermediate certificates. Self-signed certificates in the
  // preceding set are used as trust anchors.
  // NOTE: CRL and OCSP revocation checking is not performed (matching OPA).
  VerifyCertsResult parse_and_verify_certificates(std::string_view input);

  struct RSAPrivateKeyJWK
  {
    std::string kty; // "RSA"
    std::string e, n, d, p, q, dp, dq, qi; // base64url-encoded
  };

  struct ParseRSAKeyResult
  {
    RSAPrivateKeyJWK key;
    std::string error; // non-empty on failure
  };

  // Parse a PEM-encoded RSA private key and return as JWK.
  ParseRSAKeyResult parse_rsa_private_key(std::string_view input);

  struct ParsePrivateKeysResult
  {
    std::vector<RSAPrivateKeyJWK> keys;
    bool is_empty_input; // true if input was empty string → null result
    std::string error; // non-empty on failure
  };

  // Parse one or more PEM private keys.
  ParsePrivateKeysResult parse_private_keys(std::string_view input);
}

#endif // REGOCPP_HAS_CRYPTO
