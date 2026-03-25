# Crypto & JWT Builtins Implementation Plan

Phased plan for implementing `crypto.*` and `io.jwt.*` builtins in rego-cpp with a shared crypto core and compile-time backend selection.

## Current State

- All 14 `crypto.*` and 17 `io.jwt.*` builtins are **placeholders** (`BuiltInDef::placeholder`)
- No crypto library dependency exists
- 25 OPA conformance test directories cover the full API surface

## CMake Backend Selection

```cmake
set(REGOCPP_CRYPTO_BACKEND "" CACHE STRING
  "Crypto backend for crypto/JWT builtins. Options: openssl3, '' (disabled)")
set_property(CACHE REGOCPP_CRYPTO_BACKEND PROPERTY STRINGS "" "openssl3")

if(REGOCPP_CRYPTO_BACKEND STREQUAL "openssl3")
  find_package(OpenSSL 3.0 REQUIRED)
  target_link_libraries(rego PUBLIC OpenSSL::SSL OpenSSL::Crypto)
  target_compile_definitions(rego PUBLIC REGOCPP_HAS_CRYPTO=1 REGOCPP_CRYPTO_OPENSSL3=1)
elseif(NOT REGOCPP_CRYPTO_BACKEND STREQUAL "")
  message(FATAL_ERROR "Unknown crypto backend: ${REGOCPP_CRYPTO_BACKEND}. Options: openssl3")
endif()
```

When `REGOCPP_CRYPTO_BACKEND` is empty (default), all crypto/JWT builtins remain as placeholders. This preserves the current zero-dependency build.

## Shared Core Architecture

```
src/builtins/
├── crypto_core.hh           ← Backend-agnostic API header
├── crypto_openssl3.cc       ← OpenSSL 3 implementation
├── crypto.cc                ← MODIFY: replace placeholders with real impl
└── jwt.cc                   ← MODIFY: replace placeholders with real impl
```

### `crypto_core.hh` — Backend-Agnostic API

```cpp
#pragma once
#ifdef REGOCPP_HAS_CRYPTO

namespace rego::crypto_core {
  // ── Hashing ──
  std::string md5_hex(std::string_view data);
  std::string sha1_hex(std::string_view data);
  std::string sha256_hex(std::string_view data);

  // ── HMAC ──
  std::string hmac_hex(std::string_view algo, std::string_view data, std::string_view key);
  bool hmac_equal(std::string_view mac1, std::string_view mac2);  // constant-time

  // ── Base64url (raw bytes for JWT) ──
  std::string base64url_encode_nopad(std::string_view data);
  std::string base64url_decode_bytes(std::string_view data);

  // ── Signature Verification ──
  enum class Algorithm { HS256, HS384, HS512, RS256, RS384, RS512,
                         PS256, PS384, PS512, ES256, ES384, ES512, EdDSA };
  Algorithm parse_algorithm(std::string_view name);

  bool verify_signature(Algorithm algo,
                        std::string_view signing_input,
                        std::string_view signature,
                        std::string_view key_or_cert);

  std::string sign(Algorithm algo,
                   std::string_view signing_input,
                   std::string_view key);

  // ── PEM / X.509 ──
  struct ParsedCert { /* X.509 fields as JSON-compatible map */ };
  struct ParsedKey  { /* Key material */ };
  std::vector<ParsedCert> parse_certificates(std::string_view pem_or_der);
  std::pair<bool, std::vector<ParsedCert>>
    parse_and_verify_certificates(std::string_view chain);
  ParsedCert parse_csr(std::string_view pem_or_der);
  std::vector<ParsedKey> parse_private_keys(std::string_view pem);
  ParsedKey parse_rsa_private_key(std::string_view pem);
}

#endif // REGOCPP_HAS_CRYPTO
```

### Shared Core Reuse Matrix

| `crypto_core` Primitive | Used by `crypto.cc` | Used by `jwt.cc` |
|---|---|---|
| `md5_hex` | `crypto.md5` | — |
| `sha1_hex` | `crypto.sha1` | — |
| `sha256_hex` | `crypto.sha256` | — |
| `hmac_hex` | `crypto.hmac.*` | `io.jwt.encode_sign` (HS* signing) |
| `hmac_equal` | `crypto.hmac.equal` | — |
| `verify_signature` | — | All `io.jwt.verify_*`, `io.jwt.decode_verify` |
| `sign` | — | `io.jwt.encode_sign`, `io.jwt.encode_sign_raw` |
| `parse_certificates` | `crypto.x509.parse_certificates` | `io.jwt.decode_verify` (cert constraint) |
| `parse_and_verify_certs` | `crypto.x509.parse_and_verify_*` | — |
| `base64url_encode_nopad` | — | All JWT encode/sign |
| `base64url_decode_bytes` | — | All JWT decode/verify |

## Implementation Phases

### Phase 1: Infrastructure + Hashing (~300 LOC)

**Goal:** Validate dependency integration and shared core pattern.

**Steps:**
1. Add `REGOCPP_CRYPTO_BACKEND` to `CMakeLists.txt` and presets
2. Create `crypto_core.hh` with hash + HMAC signatures
3. Create `crypto_openssl3.cc` implementing hash + HMAC via EVP API
4. Add `crypto_openssl3.cc` to `src/CMakeLists.txt`
5. Replace placeholders in `crypto.cc` for:
   - `crypto.md5`, `crypto.sha1`, `crypto.sha256`
   - `crypto.hmac.md5`, `crypto.hmac.sha1`, `crypto.hmac.sha256`, `crypto.hmac.sha512`
   - `crypto.hmac.equal`

**OPA tests (8 directories):**
```
cryptomd5, cryptosha1, cryptosha256,
cryptohmacmd5, cryptohmacsha1, cryptohmacsha256, cryptohmacsha512,
cryptohmacequal
```

### Phase 2: JWT Decode + Verify (~500 LOC)

**Goal:** Implement all JWT verification and decoding.

**Steps:**
1. Add JWT token parsing helpers to `crypto_core` (split on `.`, base64url decode)
2. Add signature verification to `crypto_openssl3.cc` (EVP_DigestVerify for RSA/ECDSA/EdDSA, HMAC for HS*)
3. Add JWK → EVP_PKEY conversion (oct, RSA, EC, OKP key types)
4. Implement in `jwt.cc`:
   - `io.jwt.decode` — split + decode + hex-encode sig
   - `io.jwt.verify_hs256/384/512` — HMAC verify
   - `io.jwt.verify_rs256/384/512` — RSA PKCS#1 v1.5 verify
   - `io.jwt.verify_ps256/384/512` — RSA-PSS verify
   - `io.jwt.verify_es256/384/512` — ECDSA verify
   - `io.jwt.verify_eddsa` — Ed25519 verify
   - `io.jwt.decode_verify` — verify + claim validation (exp, nbf, iss, aud, time)

**Key subtleties:**
- `decode_verify` returns `[false, {}, {}]` on failure (not an error)
- JWK key sets matched by `kid` header
- PEM certificates parsed to extract public key

**OPA tests (7 directories):**
```
jwtbuiltins, jwtverifyhs256, jwtverifyhs384, jwtverifyhs512,
jwtverifyrsa, jwtverifyeddsa, jwtdecodeverify
```

### Phase 3: JWT Encode + Sign (~300 LOC)

**Goal:** Complete JWT support.

**Steps:**
1. Add signing to `crypto_openssl3.cc` (EVP_DigestSign, HMAC)
2. Implement in `jwt.cc`:
   - `io.jwt.encode_sign` — JSON-serialize header+payload, base64url-encode, sign
   - `io.jwt.encode_sign_raw` — same but skip JSON re-serialization

**OPA tests (4 directories):**
```
jwtencodesign, jwtencodesignraw,
jwtencodesignheadererrors, jwtencodesignpayloaderrors
```

### Phase 4: X.509 + Key Parsing (~800 LOC, highest complexity)

**Goal:** Complete all crypto builtins.

**Steps:**
1. Add X.509 parsing to `crypto_openssl3.cc` (PEM/DER → structured objects)
2. Implement in `crypto.cc`:
   - `crypto.x509.parse_certificates`
   - `crypto.x509.parse_and_verify_certificates`
   - `crypto.x509.parse_and_verify_certificates_with_options`
   - `crypto.x509.parse_certificate_request`
   - `crypto.x509.parse_keypair`
   - `crypto.x509.parse_rsa_private_key`
   - `crypto.parse_private_keys`

**This is the hardest phase** — X.509 certificate output must match OPA's Go `x509.Certificate` struct serialization field-by-field. Expect significant iteration matching field names, date formats, extension representations, and OID handling.

**OPA tests (6 directories):**
```
cryptox509parsecertificates, cryptox509parseandverifycertificates,
cryptox509parsecertificaterequest, cryptox509parsekeypair,
cryptox509parsersaprivatekey, cryptoparsersaprivatekeys
```

## Phase 5: Windows Native Backend (BCrypt/CNG)

**Goal:** Provide a Windows-native crypto backend using BCrypt/CNG so that Windows builds don't require an external OpenSSL install.

**Steps:**
1. Add `bcrypt` option to `REGOCPP_CRYPTO_BACKEND` in `CMakeLists.txt`
2. Create `src/builtins/crypto_bcrypt.cc` implementing the `crypto_core.hh` API using Windows BCrypt/CNG
3. Wire up CMake: link `bcrypt.lib`, define `REGOCPP_CRYPTO_BCRYPT=1`
4. Add Windows-specific presets (e.g., `debug-msvc-opa`, `release-msvc-opa`)
5. Validate all OPA conformance tests pass on Windows

**Notes:**
- BCrypt/CNG is available on Windows Vista+ (no external dependency)
- EdDSA (Ed25519) support may require Windows 10 1903+ or a fallback
- The `crypto_core.hh` abstraction layer is designed for this — only the new `.cc` file touches Windows APIs

## Risk Assessment

| Risk | Mitigation |
|---|---|
| X.509 output format mismatch | Inspect OPA test expected output field-by-field before implementing |
| OpenSSL API differences | Require OpenSSL ≥ 3.0; use EVP API exclusively |
| EdDSA support | Available in OpenSSL 3.0+ via `EVP_PKEY_ED25519` |
| JWK parsing edge cases | OPA tests cover RSA, EC, oct, OKP; implement incrementally |
| Cross-platform OpenSSL availability | `REGOCPP_CRYPTO_BACKEND=""` preserves zero-dependency build |
| Error message matching | Conformance tests compare strings literally; verify against OPA output |
| Future backend portability | Backend-agnostic API in `crypto_core.hh`; only `crypto_openssl3.cc` touches OpenSSL |
| BCrypt EdDSA gaps | Ed25519 requires Windows 10 1903+; may need version check or graceful fallback |
