# Phase 5: Windows BCrypt Backend — Implementation Guide

## Overview

Implement the Windows BCrypt/CNG crypto backend in `src/builtins/crypto_bcrypt.cc`. The file already has working implementations for hashing and HMAC (8 functions), plus fully-stubbed TODO implementations for the remaining 8 functions (verify, sign, X.509, RSA key parsing).

## Architecture

```
crypto_core.hh          ← Backend-agnostic API (16 functions)
crypto_utils.hh         ← Shared platform-independent utilities (inline)
crypto_openssl3.cc      ← OpenSSL 3 backend (Linux/macOS) — REFERENCE IMPLEMENTATION
crypto_bcrypt.cc        ← Windows BCrypt backend — THIS FILE
```

**Build command:**
```cmd
cmake -B build -DREGOCPP_CRYPTO_BACKEND=bcrypt -DREGOCPP_BUILD_TESTS=ON -DREGOCPP_BUILD_TOOLS=ON -DREGOCPP_OPA_TESTS=ON
cmake --build build
```

**CMake defines set:** `REGOCPP_HAS_CRYPTO=1`, `REGOCPP_CRYPTO_BCRYPT=1`
**Libraries linked:** `bcrypt`, `crypt32`

## What's Already Done

### Fully implemented (8 functions):
- `md5_hex`, `sha1_hex`, `sha256_hex` — via `BCryptHash`
- `hmac_md5_hex`, `hmac_sha1_hex`, `hmac_sha256_hex`, `hmac_sha512_hex` — via `BCryptCreateHash` with HMAC flag
- `hmac_equal` — constant-time compare (shared via `crypto_utils.hh`)

### Delegated to shared utilities (3 functions):
- `base64url_encode_nopad`, `base64url_decode` — pure C++ (shared header)
- `parse_algorithm` — string→enum (shared header)

### Stubbed with TODO + strategy notes (8 functions):
- `verify_signature` — JWT signature verification
- `verify_signature_any_key` — JWKS multi-key verification
- `sign` — JWT signing
- `parse_certificates` — X.509 certificate parsing
- `parse_and_verify_certificates` — X.509 chain validation
- `parse_certificate_request` — CSR parsing
- `parse_rsa_private_key` — RSA private key → JWK
- `parse_private_keys` — multiple private keys → JWK array

## Implementation Order (recommended)

### Step 1: verify_signature (HMAC subset first)
Start with HMAC verification (HS256/384/512) since you already have `hmac_hex()`:
1. For HMAC algos: compute HMAC of `signing_input`, compare raw bytes against `signature_bytes`
2. Test: `cd build && .\tests\rego_test -wf opa\v1\test\cases\testdata\v1\jwtverifyhs256`

### Step 2: verify_signature (RSA)
1. Parse PEM public key or PEM certificate from `key_or_cert`
2. For PEM cert: `CertCreateCertificateContext` → `CryptImportPublicKeyInfoEx2` → `BCRYPT_KEY_HANDLE`
3. For PEM pubkey: `CryptDecodeObjectEx(X509_PUBLIC_KEY_INFO)` → `CryptImportPublicKeyInfoEx2`
4. For JWK: manually construct `BCRYPT_RSAPUBLIC_BLOB` from n/e components
5. `BCryptVerifySignature` with `BCRYPT_PAD_PKCS1` (RS*) or `BCRYPT_PAD_PSS` (PS*)
6. Test: `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\jwtverifyrsa`

### Step 3: verify_signature (ECDSA + EdDSA)
1. EC key import from PEM/JWK → `BCRYPT_ECCPUBLIC_BLOB`
2. **Important:** JWT ECDSA signatures are raw `r||s` concatenation, NOT DER. Convert accordingly.
3. EdDSA requires Windows 10 1903+ (`BCRYPT_ECC_CURVE_25519`)
4. Test: `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\jwtverifyeddsa`

### Step 4: verify_signature_any_key
1. Copy the JWKS iteration logic from `crypto_openssl3.cc` — it's mostly platform-independent JSON parsing
2. Use `::json::reader().synthetic(key_or_cert).read()` then `::json::select(ast, {"/keys"})`
3. For each key in the JSON array, extract the JWK string and call `verify_signature()`
4. Test: `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\jwtdecodeverify`

### Step 5: sign
1. Parse JWK private key JSON
2. Import into `BCRYPT_KEY_HANDLE` (differs per algorithm family)
3. `BCryptSignHash` for RSA/EC, `hmac_hex` for HMAC
4. Test: `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\jwtencodesign`

### Step 6: parse_certificates
1. Use `decode_cert_input()` + `extract_pem_der_blocks("CERTIFICATE")` from `crypto_utils.hh`
2. For each DER block: `CertCreateCertificateContext(X509_ASN_ENCODING, ...)`
3. Extract CN: `CertGetNameStringA(pCert, CERT_NAME_ATTR_TYPE, 0, szOID_COMMON_NAME, ...)`
4. Extract SANs: `CertFindExtension(szOID_SUBJECT_ALT_NAME2)` → `CryptDecodeObjectEx` → `CERT_ALT_NAME_INFO`
5. DER base64: `::base64_encode(der_block, false)`
6. Test: `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\cryptox509parsecertificates`

### Step 7: parse_and_verify_certificates
1. Parse certs as in Step 6
2. Build cert chain: add root to temp cert store, create chain with `CertGetCertificateChain`
3. OPA convention: input order is root first, leaf last
4. **Return verified chain in leaf-first order** (matching OPA behavior)
5. Test: `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\cryptox509parseandverifycertificates`

### Step 8: parse_certificate_request + RSA key parsing
1. CSR: `CryptDecodeObjectEx(X509_CERT_REQUEST_TO_BE_SIGNED)` or manually parse ASN.1
2. RSA key: `CryptDecodeObjectEx(PKCS_RSA_PRIVATE_KEY)` for PKCS#1, or decode PKCS#8 wrapper first
3. Export components via `BCryptExportKey(BCRYPT_RSAFULLPRIVATE_BLOB)` to get n/e/d/p/q/dp/dq/qi
4. Convert each to base64url for JWK format
5. Tests:
   - `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\cryptox509parsecertificaterequest`
   - `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\cryptox509parsersaprivatekey`
   - `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\cryptoparsersaprivatekeys`
   - `.\tests\rego_test -wf opa\v1\test\cases\testdata\v1\cryptox509parsekeypair`

## Key Patterns from the OpenSSL Reference

### PEM key parsing (reusable across backends)
The shared `extract_pem_der_blocks()` in `crypto_utils.hh` handles PEM → DER extraction. Use it for certificates, keys, and CSRs.

### JWK parsing
Use Trieste JSON: `auto ast = ::json::reader().synthetic(json_str).read()`. Then `::json::select_string(ast, {"/kty"})` etc.

### Error messages must match OPA exactly
Conformance tests compare error strings literally. Check OPA's error messages in the test YAML files and match them exactly.

### Input unescaping
The builtin implementations in `crypto.cc` and `jwt.cc` already call `json::unescape(get_string(...))` before passing strings to the backend. You do NOT need to handle JSON escaping in `crypto_bcrypt.cc`.

## Test Directories (all under `build/opa/v1/test/cases/testdata/v1/`)

| Directory | Tests | Category |
|-----------|-------|----------|
| `cryptomd5` | 1 | Hashing |
| `cryptosha1` | 2 | Hashing |
| `cryptosha256` | 5 | Hashing |
| `cryptohmacmd5` | 2 | HMAC |
| `cryptohmacsha1` | 2 | HMAC |
| `cryptohmacsha256` | 2 | HMAC |
| `cryptohmacsha512` | 1 | HMAC |
| `cryptohmacequal` | 1 | HMAC |
| `jwtverifyhs256` | 5 | JWT verify (HMAC) |
| `jwtverifyhs384` | 5 | JWT verify (HMAC) |
| `jwtverifyhs512` | 5 | JWT verify (HMAC) |
| `jwtverifyrsa` | 47 | JWT verify (RSA) |
| `jwtverifyeddsa` | 5 | JWT verify (EdDSA) |
| `jwtdecodeverify` | 47 | JWT decode+verify |
| `jwtbuiltins` | 3 | JWT misc |
| `jwtencodesign` | 5 | JWT sign |
| `jwtencodesignraw` | 7 | JWT sign raw |
| `jwtencodesignheadererrors` | 6 | JWT sign errors |
| `jwtencodesignpayloaderrors` | 5 | JWT sign errors |
| `cryptox509parsecertificates` | 10 | X.509 |
| `cryptox509parseandverifycertificates` | 2 | X.509 verify |
| `cryptox509parsecertificaterequest` | 5 | CSR |
| `cryptox509parsekeypair` | 2 | Keypair |
| `cryptox509parsersaprivatekey` | 2 | RSA key |
| `cryptoparsersaprivatekeys` | 3 | Private keys |

**Target: 168/168 tests passing** (current score on Linux with OpenSSL backend).

## Files You'll Modify

| File | What to change |
|------|---------------|
| `src/builtins/crypto_bcrypt.cc` | Implement the 8 TODO functions |

Files you should NOT need to modify:
- `crypto_core.hh` — API is stable
- `crypto_utils.hh` — shared utilities are complete
- `crypto.cc` / `jwt.cc` — builtin dispatch is backend-agnostic
- `CMakeLists.txt` / `src/CMakeLists.txt` — build config is ready
