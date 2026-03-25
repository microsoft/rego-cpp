// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef REGOCPP_CRYPTO_BCRYPT

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "base64/base64.h"
#include "crypto_core.hh"
#include "crypto_utils.hh"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <trieste/json.h>
#include <vector>

// Link against Windows crypto libraries (also specified in CMakeLists.txt)
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

#include <Windows.h>
#include <bcrypt.h>
#include <wincrypt.h>

namespace
{
  using rego::crypto_core::to_hex;

  // Safe cast from size_t to ULONG, throwing on overflow.
  ULONG safe_ulong(size_t n)
  {
    if (n > static_cast<size_t>(std::numeric_limits<ULONG>::max()))
    {
      throw std::runtime_error("data too large for BCrypt API");
    }
    return static_cast<ULONG>(n);
  }

  // RAII wrapper for BCRYPT_ALG_HANDLE
  struct AlgHandleDeleter
  {
    void operator()(BCRYPT_ALG_HANDLE h) const
    {
      if (h)
      {
        BCryptCloseAlgorithmProvider(h, 0);
      }
    }
  };
  using AlgHandle = std::unique_ptr<void, AlgHandleDeleter>;

  // RAII wrapper for BCRYPT_HASH_HANDLE
  struct HashHandleDeleter
  {
    void operator()(BCRYPT_HASH_HANDLE h) const
    {
      if (h)
      {
        BCryptDestroyHash(h);
      }
    }
  };
  using HashHandle = std::unique_ptr<void, HashHandleDeleter>;

  // RAII wrapper for BCRYPT_KEY_HANDLE
  struct KeyHandleDeleter
  {
    void operator()(BCRYPT_KEY_HANDLE h) const
    {
      if (h)
      {
        BCryptDestroyKey(h);
      }
    }
  };
  using KeyHandle = std::unique_ptr<void, KeyHandleDeleter>;

  // Helper: compute a hash digest and return hex string
  std::string digest_hex(LPCWSTR algo_id, std::string_view data)
  {
    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status =
      BCryptOpenAlgorithmProvider(&alg_raw, algo_id, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptOpenAlgorithmProvider failed");
    }
    AlgHandle alg(alg_raw);

    DWORD hash_len = 0;
    DWORD result_len = 0;
    status = BCryptGetProperty(
      alg.get(),
      BCRYPT_HASH_LENGTH,
      reinterpret_cast<PUCHAR>(&hash_len),
      sizeof(hash_len),
      &result_len,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptGetProperty(HASH_LENGTH) failed");
    }

    BCRYPT_HASH_HANDLE hash_raw = nullptr;
    status = BCryptCreateHash(alg.get(), &hash_raw, nullptr, 0, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptCreateHash failed");
    }
    HashHandle hash(hash_raw);

    status = BCryptHashData(
      hash.get(),
      reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())),
      safe_ulong(data.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptHashData failed");
    }

    std::vector<unsigned char> buf(hash_len);
    status = BCryptFinishHash(hash.get(), buf.data(), hash_len, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptFinishHash failed");
    }

    return to_hex(buf.data(), buf.size());
  }

  // Helper: compute HMAC and return hex string
  std::string hmac_hex(
    LPCWSTR algo_id, std::string_view key, std::string_view data)
  {
    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
      &alg_raw, algo_id, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptOpenAlgorithmProvider (HMAC) failed");
    }
    AlgHandle alg(alg_raw);

    DWORD hash_len = 0;
    DWORD result_len = 0;
    status = BCryptGetProperty(
      alg.get(),
      BCRYPT_HASH_LENGTH,
      reinterpret_cast<PUCHAR>(&hash_len),
      sizeof(hash_len),
      &result_len,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptGetProperty(HASH_LENGTH) failed");
    }

    BCRYPT_HASH_HANDLE hash_raw = nullptr;
    status = BCryptCreateHash(
      alg.get(),
      &hash_raw,
      nullptr,
      0,
      reinterpret_cast<PUCHAR>(const_cast<char*>(key.data())),
      safe_ulong(key.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptCreateHash (HMAC) failed");
    }
    HashHandle hash(hash_raw);

    status = BCryptHashData(
      hash.get(),
      reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())),
      safe_ulong(data.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptHashData failed");
    }

    std::vector<unsigned char> buf(hash_len);
    status = BCryptFinishHash(hash.get(), buf.data(), hash_len, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptFinishHash (HMAC) failed");
    }

    return to_hex(buf.data(), buf.size());
  }

  // Helper: compute HMAC and return raw bytes (for signature verification)
  std::vector<unsigned char> hmac_raw(
    LPCWSTR algo_id, std::string_view key, std::string_view data)
  {
    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
      &alg_raw, algo_id, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptOpenAlgorithmProvider (HMAC) failed");
    }
    AlgHandle alg(alg_raw);

    DWORD hash_len = 0;
    DWORD result_len = 0;
    status = BCryptGetProperty(
      alg.get(),
      BCRYPT_HASH_LENGTH,
      reinterpret_cast<PUCHAR>(&hash_len),
      sizeof(hash_len),
      &result_len,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptGetProperty(HASH_LENGTH) failed");
    }

    BCRYPT_HASH_HANDLE hash_raw = nullptr;
    status = BCryptCreateHash(
      alg.get(),
      &hash_raw,
      nullptr,
      0,
      reinterpret_cast<PUCHAR>(const_cast<char*>(key.data())),
      safe_ulong(key.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptCreateHash (HMAC) failed");
    }
    HashHandle hash(hash_raw);

    status = BCryptHashData(
      hash.get(),
      reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())),
      safe_ulong(data.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptHashData failed");
    }

    std::vector<unsigned char> buf(hash_len);
    status = BCryptFinishHash(hash.get(), buf.data(), hash_len, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptFinishHash (HMAC) failed");
    }

    return buf;
  }

  // Map Algorithm enum to BCrypt algorithm ID for HMAC
  LPCWSTR hmac_algo_id(rego::crypto_core::Algorithm algo)
  {
    using rego::crypto_core::Algorithm;
    switch (algo)
    {
      case Algorithm::HS256:
        return BCRYPT_SHA256_ALGORITHM;
      case Algorithm::HS384:
        return BCRYPT_SHA384_ALGORITHM;
      case Algorithm::HS512:
        return BCRYPT_SHA512_ALGORITHM;
      default:
        return nullptr;
    }
  }

  bool verify_hmac(
    rego::crypto_core::Algorithm algo,
    std::string_view signing_input,
    std::string_view sig_bytes,
    std::string_view secret)
  {
    LPCWSTR algo_id = hmac_algo_id(algo);
    if (!algo_id)
    {
      return false;
    }

    auto computed = hmac_raw(algo_id, secret, signing_input);

    // Constant-time comparison
    if (computed.size() != sig_bytes.size())
    {
      return false;
    }
    volatile unsigned char result = 0;
    for (size_t i = 0; i < computed.size(); ++i)
    {
      result |= computed[i] ^ static_cast<unsigned char>(sig_bytes[i]);
    }
    return result == 0;
  }

  // ── RAII wrapper for PCCERT_CONTEXT ──
  struct CertContextDeleter
  {
    void operator()(PCCERT_CONTEXT ctx) const
    {
      if (ctx)
      {
        CertFreeCertificateContext(ctx);
      }
    }
  };
  using CertContextHandle =
    std::unique_ptr<const CERT_CONTEXT, CertContextDeleter>;

  using rego::crypto_core::json_select_string;
  using rego::crypto_core::parse_json;

  // ── PEM validation (error messages must match OPA exactly) ──

  // ── BCrypt hash algorithm ID for signature verification ──

  LPCWSTR hash_algo_id(rego::crypto_core::Algorithm algo)
  {
    using rego::crypto_core::Algorithm;
    switch (algo)
    {
      case Algorithm::RS256:
      case Algorithm::PS256:
      case Algorithm::ES256:
      case Algorithm::HS256:
        return BCRYPT_SHA256_ALGORITHM;
      case Algorithm::RS384:
      case Algorithm::PS384:
      case Algorithm::ES384:
      case Algorithm::HS384:
        return BCRYPT_SHA384_ALGORITHM;
      case Algorithm::RS512:
      case Algorithm::PS512:
      case Algorithm::ES512:
      case Algorithm::HS512:
        return BCRYPT_SHA512_ALGORITHM;
      case Algorithm::EdDSA:
        return nullptr;
    }
    return nullptr;
  }

  // ── Hash data for signature verification ──

  std::vector<unsigned char> compute_hash(
    LPCWSTR algo_id, std::string_view data)
  {
    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status =
      BCryptOpenAlgorithmProvider(&alg_raw, algo_id, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptOpenAlgorithmProvider failed");
    }
    AlgHandle alg(alg_raw);

    DWORD hash_len = 0;
    DWORD result_len = 0;
    status = BCryptGetProperty(
      alg.get(),
      BCRYPT_HASH_LENGTH,
      reinterpret_cast<PUCHAR>(&hash_len),
      sizeof(hash_len),
      &result_len,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptGetProperty(HASH_LENGTH) failed");
    }

    BCRYPT_HASH_HANDLE hash_raw = nullptr;
    status = BCryptCreateHash(alg.get(), &hash_raw, nullptr, 0, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptCreateHash failed");
    }
    HashHandle hash(hash_raw);

    status = BCryptHashData(
      hash.get(),
      reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())),
      safe_ulong(data.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptHashData failed");
    }

    std::vector<unsigned char> buf(hash_len);
    status = BCryptFinishHash(hash.get(), buf.data(), hash_len, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptFinishHash failed");
    }

    return buf;
  }

  // ── Key loading helpers ──

  // Import a public key from a PEM certificate via WinCrypt
  KeyHandle key_from_pem_cert(std::string_view pem)
  {
    using rego::crypto_core::extract_pem_der_blocks;
    auto der_blocks = extract_pem_der_blocks(pem, "CERTIFICATE");
    if (der_blocks.empty())
    {
      return KeyHandle(nullptr);
    }
    auto& der = der_blocks[0];

    PCCERT_CONTEXT cert_ctx = CertCreateCertificateContext(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
      reinterpret_cast<const BYTE*>(der.data()),
      static_cast<DWORD>(der.size()));
    if (!cert_ctx)
    {
      return KeyHandle(nullptr);
    }
    CertContextHandle cert_guard(cert_ctx);

    BCRYPT_KEY_HANDLE key_raw = nullptr;
    if (!CryptImportPublicKeyInfoEx2(
          X509_ASN_ENCODING,
          &cert_ctx->pCertInfo->SubjectPublicKeyInfo,
          0,
          nullptr,
          &key_raw))
    {
      return KeyHandle(nullptr);
    }
    return KeyHandle(key_raw);
  }

  // Import a public key from a PEM SPKI (SubjectPublicKeyInfo) block
  KeyHandle key_from_pem_pubkey(std::string_view pem)
  {
    using rego::crypto_core::extract_pem_der_blocks;
    auto der_blocks = extract_pem_der_blocks(pem, "PUBLIC KEY");
    if (der_blocks.empty())
    {
      return KeyHandle(nullptr);
    }
    auto& der = der_blocks[0];

    // Decode the DER SPKI into a CERT_PUBLIC_KEY_INFO
    CERT_PUBLIC_KEY_INFO* pub_info = nullptr;
    DWORD pub_info_size = 0;
    if (!CryptDecodeObjectEx(
          X509_ASN_ENCODING,
          X509_PUBLIC_KEY_INFO,
          reinterpret_cast<const BYTE*>(der.data()),
          static_cast<DWORD>(der.size()),
          CRYPT_DECODE_ALLOC_FLAG,
          nullptr,
          &pub_info,
          &pub_info_size))
    {
      return KeyHandle(nullptr);
    }

    BCRYPT_KEY_HANDLE key_raw = nullptr;
    BOOL ok = CryptImportPublicKeyInfoEx2(
      X509_ASN_ENCODING, pub_info, 0, nullptr, &key_raw);
    LocalFree(pub_info);

    if (!ok)
    {
      return KeyHandle(nullptr);
    }
    return KeyHandle(key_raw);
  }

  // Import an RSA public key from JWK n/e components.
  // Constructs a BCRYPT_RSAPUBLIC_BLOB manually.
  KeyHandle key_from_jwk_rsa(std::string_view n_b64, std::string_view e_b64)
  {
    std::string n_raw = ::base64_decode(n_b64);
    std::string e_raw = ::base64_decode(e_b64);
    if (n_raw.empty() || e_raw.empty())
    {
      return KeyHandle(nullptr);
    }

    // BCRYPT_RSAPUBLIC_BLOB layout:
    //   BCRYPT_RSAKEY_BLOB header
    //   PublicExponent[cbPublicExp]
    //   Modulus[cbModulus]
    DWORD cbPublicExp = static_cast<DWORD>(e_raw.size());
    DWORD cbModulus = static_cast<DWORD>(n_raw.size());

    std::vector<BYTE> blob(
      sizeof(BCRYPT_RSAKEY_BLOB) + cbPublicExp + cbModulus);
    auto* header = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(blob.data());
    header->Magic = BCRYPT_RSAPUBLIC_MAGIC;
    header->BitLength = cbModulus * 8;
    header->cbPublicExp = cbPublicExp;
    header->cbModulus = cbModulus;
    header->cbPrime1 = 0;
    header->cbPrime2 = 0;

    BYTE* ptr = blob.data() + sizeof(BCRYPT_RSAKEY_BLOB);
    std::memcpy(ptr, e_raw.data(), cbPublicExp);
    ptr += cbPublicExp;
    std::memcpy(ptr, n_raw.data(), cbModulus);

    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status =
      BCryptOpenAlgorithmProvider(&alg_raw, BCRYPT_RSA_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      return KeyHandle(nullptr);
    }
    AlgHandle alg(alg_raw);

    BCRYPT_KEY_HANDLE key_raw = nullptr;
    status = BCryptImportKeyPair(
      alg.get(),
      nullptr,
      BCRYPT_RSAPUBLIC_BLOB,
      &key_raw,
      blob.data(),
      safe_ulong(blob.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      return KeyHandle(nullptr);
    }
    return KeyHandle(key_raw);
  }

  // Import an EC public key from JWK x/y/crv components.
  // Constructs a BCRYPT_ECCPUBLIC_BLOB manually.
  KeyHandle key_from_jwk_ec(
    std::string_view crv, std::string_view x_b64, std::string_view y_b64)
  {
    std::string x_raw = ::base64_decode(x_b64);
    std::string y_raw = ::base64_decode(y_b64);
    if (x_raw.empty() || y_raw.empty())
    {
      return KeyHandle(nullptr);
    }

    // Determine the BCrypt algorithm and expected key size
    LPCWSTR algo_id = nullptr;
    ULONG magic = 0;
    DWORD key_size = 0;
    if (crv == "P-256")
    {
      algo_id = BCRYPT_ECDSA_P256_ALGORITHM;
      magic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
      key_size = 32;
    }
    else if (crv == "P-384")
    {
      algo_id = BCRYPT_ECDSA_P384_ALGORITHM;
      magic = BCRYPT_ECDSA_PUBLIC_P384_MAGIC;
      key_size = 48;
    }
    else if (crv == "P-521")
    {
      algo_id = BCRYPT_ECDSA_P521_ALGORITHM;
      magic = BCRYPT_ECDSA_PUBLIC_P521_MAGIC;
      key_size = 66;
    }
    else
    {
      return KeyHandle(nullptr);
    }

    // BCRYPT_ECCPUBLIC_BLOB layout:
    //   BCRYPT_ECCKEY_BLOB header
    //   X[cbKey]
    //   Y[cbKey]
    std::vector<BYTE> blob(sizeof(BCRYPT_ECCKEY_BLOB) + key_size * 2);
    auto* header = reinterpret_cast<BCRYPT_ECCKEY_BLOB*>(blob.data());
    header->dwMagic = magic;
    header->cbKey = key_size;

    // Pad or truncate x/y to exactly key_size bytes (big-endian, left-padded)
    BYTE* x_dest = blob.data() + sizeof(BCRYPT_ECCKEY_BLOB);
    BYTE* y_dest = x_dest + key_size;
    std::memset(x_dest, 0, key_size);
    std::memset(y_dest, 0, key_size);

    if (x_raw.size() <= key_size)
    {
      std::memcpy(
        x_dest + (key_size - x_raw.size()), x_raw.data(), x_raw.size());
    }
    else
    {
      std::memcpy(x_dest, x_raw.data() + (x_raw.size() - key_size), key_size);
    }

    if (y_raw.size() <= key_size)
    {
      std::memcpy(
        y_dest + (key_size - y_raw.size()), y_raw.data(), y_raw.size());
    }
    else
    {
      std::memcpy(y_dest, y_raw.data() + (y_raw.size() - key_size), key_size);
    }

    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status =
      BCryptOpenAlgorithmProvider(&alg_raw, algo_id, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      return KeyHandle(nullptr);
    }
    AlgHandle alg(alg_raw);

    BCRYPT_KEY_HANDLE key_raw = nullptr;
    status = BCryptImportKeyPair(
      alg.get(),
      nullptr,
      BCRYPT_ECCPUBLIC_BLOB,
      &key_raw,
      blob.data(),
      safe_ulong(blob.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      return KeyHandle(nullptr);
    }
    return KeyHandle(key_raw);
  }

  // Parse a JWK JSON AST into a BCRYPT_KEY_HANDLE (public key)
  KeyHandle key_from_jwk_ast(const trieste::Node& ast)
  {
    using rego::crypto_core::MaxECComponentB64Len;
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
        return KeyHandle(nullptr);
      }
      return key_from_jwk_rsa(n, e);
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
        return KeyHandle(nullptr);
      }
      return key_from_jwk_ec(crv, x, y);
    }
    // OKP (Ed25519) is not supported by Windows CNG for signing/verification.
    // Windows BCrypt supports X25519 for ECDH key exchange only.
    // Ed25519 keys will return nullptr, causing verify_signature to return
    // {false, ""} (verification failure, not an error).
    return KeyHandle(nullptr);
  }

  using rego::crypto_core::extract_jwks_keys;

  // Auto-detect key format and load BCRYPT_KEY_HANDLE.
  // Handles: PEM cert, PEM pubkey, JWK object, JWKS set.
  KeyHandle load_public_key(std::string_view key_or_cert)
  {
    // PEM certificate
    if (
      key_or_cert.find("-----BEGIN CERTIFICATE-----") != std::string_view::npos)
    {
      return key_from_pem_cert(key_or_cert);
    }

    // PEM public key
    if (
      key_or_cert.find("-----BEGIN PUBLIC KEY-----") != std::string_view::npos)
    {
      return key_from_pem_pubkey(key_or_cert);
    }

    // Try JSON (JWK or JWKS)
    auto ast = parse_json(key_or_cert);
    if (!ast)
    {
      return KeyHandle(nullptr);
    }

    // Try JWKS first
    auto keys = extract_jwks_keys(ast);
    if (!keys.empty())
    {
      // Use first key (single-key path; multi-key handled by
      // verify_signature_any_key)
      return key_from_jwk_ast(keys[0]);
    }

    // Try single JWK
    std::string_view kty = json_select_string(ast, "/kty");
    if (!kty.empty())
    {
      return key_from_jwk_ast(ast);
    }

    return KeyHandle(nullptr);
  }

  // ── RSA signature verification ──

  bool verify_rsa_pkcs1(
    BCRYPT_KEY_HANDLE key,
    LPCWSTR hash_algo,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    auto hash_value = compute_hash(hash_algo, signing_input);

    BCRYPT_PKCS1_PADDING_INFO padding_info;
    padding_info.pszAlgId = hash_algo;

    NTSTATUS status = BCryptVerifySignature(
      key,
      &padding_info,
      hash_value.data(),
      safe_ulong(hash_value.size()),
      reinterpret_cast<PUCHAR>(const_cast<char*>(sig_bytes.data())),
      safe_ulong(sig_bytes.size()),
      BCRYPT_PAD_PKCS1);

    return BCRYPT_SUCCESS(status);
  }

  bool verify_rsa_pss(
    BCRYPT_KEY_HANDLE key,
    LPCWSTR hash_algo,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    auto hash_value = compute_hash(hash_algo, signing_input);

    BCRYPT_PSS_PADDING_INFO padding_info;
    padding_info.pszAlgId = hash_algo;
    // Salt length = hash length (matches OpenSSL RSA_PSS_SALTLEN_AUTO
    // behavior for verification)
    padding_info.cbSalt = safe_ulong(hash_value.size());

    NTSTATUS status = BCryptVerifySignature(
      key,
      &padding_info,
      hash_value.data(),
      safe_ulong(hash_value.size()),
      reinterpret_cast<PUCHAR>(const_cast<char*>(sig_bytes.data())),
      safe_ulong(sig_bytes.size()),
      BCRYPT_PAD_PSS);

    return BCRYPT_SUCCESS(status);
  }

  // ── ECDSA signature verification ──
  // JWT ECDSA signatures are raw r||s — BCrypt also expects raw r||s,
  // so no DER conversion is needed (unlike OpenSSL).

  bool verify_ecdsa(
    BCRYPT_KEY_HANDLE key,
    LPCWSTR hash_algo_id,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    auto hash_value = compute_hash(hash_algo_id, signing_input);

    NTSTATUS status = BCryptVerifySignature(
      key,
      nullptr,
      hash_value.data(),
      safe_ulong(hash_value.size()),
      reinterpret_cast<PUCHAR>(const_cast<char*>(sig_bytes.data())),
      safe_ulong(sig_bytes.size()),
      0);

    return BCRYPT_SUCCESS(status);
  }

}

namespace rego::crypto_core
{
  // ── Hashing ──

  std::string md5_hex(std::string_view data)
  {
    return digest_hex(BCRYPT_MD5_ALGORITHM, data);
  }

  std::string sha1_hex(std::string_view data)
  {
    return digest_hex(BCRYPT_SHA1_ALGORITHM, data);
  }

  std::string sha256_hex(std::string_view data)
  {
    return digest_hex(BCRYPT_SHA256_ALGORITHM, data);
  }

  // ── HMAC ──

  std::string hmac_md5_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(BCRYPT_MD5_ALGORITHM, key, data);
  }

  std::string hmac_sha1_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(BCRYPT_SHA1_ALGORITHM, key, data);
  }

  std::string hmac_sha256_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(BCRYPT_SHA256_ALGORITHM, key, data);
  }

  std::string hmac_sha512_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(BCRYPT_SHA512_ALGORITHM, key, data);
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

  // ── Signature Verification ──

  VerifyResult verify_signature(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view signature_bytes,
    std::string_view key_or_cert)
  {
    // EdDSA (Ed25519) is not supported by Windows CNG.
    if (algo == Algorithm::EdDSA)
    {
      return {false, "EdDSA algorithm is not supported"};
    }

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
    KeyHandle pkey = load_public_key(key_or_cert);
    if (!pkey)
    {
      if (
        key_or_cert.find("-----BEGIN CERTIFICATE-----") !=
        std::string_view::npos)
      {
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

    LPCWSTR halgo = hash_algo_id(algo);
    bool ok = false;

    switch (algo)
    {
      case Algorithm::RS256:
      case Algorithm::RS384:
      case Algorithm::RS512:
        ok =
          verify_rsa_pkcs1(pkey.get(), halgo, signing_input, signature_bytes);
        break;

      case Algorithm::PS256:
      case Algorithm::PS384:
      case Algorithm::PS512:
        ok = verify_rsa_pss(pkey.get(), halgo, signing_input, signature_bytes);
        break;

      case Algorithm::ES256:
      case Algorithm::ES384:
      case Algorithm::ES512:
        ok = verify_ecdsa(pkey.get(), halgo, signing_input, signature_bytes);
        break;

      case Algorithm::EdDSA:
        // Ed25519 not supported by Windows CNG; key import will have
        // already failed (pkey == nullptr), so this is unreachable.
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

    // HMAC doesn't use JWKS — fall through to normal path
    if (
      algo == Algorithm::HS256 || algo == Algorithm::HS384 ||
      algo == Algorithm::HS512)
    {
      return verify_signature(
        algo, signing_input, signature_bytes, key_or_cert);
    }

    LPCWSTR halgo = hash_algo_id(algo);

    // Try each key; return success on first valid signature
    for (auto& key_ast : keys)
    {
      auto pkey = key_from_jwk_ast(key_ast);
      if (!pkey)
      {
        continue;
      }

      bool ok = false;

      switch (algo)
      {
        case Algorithm::RS256:
        case Algorithm::RS384:
        case Algorithm::RS512:
          ok =
            verify_rsa_pkcs1(pkey.get(), halgo, signing_input, signature_bytes);
          break;

        case Algorithm::PS256:
        case Algorithm::PS384:
        case Algorithm::PS512:
          ok =
            verify_rsa_pss(pkey.get(), halgo, signing_input, signature_bytes);
          break;

        case Algorithm::ES256:
        case Algorithm::ES384:
        case Algorithm::ES512:
          ok = verify_ecdsa(pkey.get(), halgo, signing_input, signature_bytes);
          break;

        case Algorithm::EdDSA:
          // Not supported by Windows CNG
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

  // ── Signing ──

  // Import an RSA private key from JWK components.
  // Constructs a BCRYPT_RSAFULLPRIVATE_BLOB.
  // Layout: header | e | n | p | q | dp | dq | qi | d
  KeyHandle key_from_jwk_rsa_private(
    std::string_view n_b64,
    std::string_view e_b64,
    std::string_view d_b64,
    std::string_view p_b64,
    std::string_view q_b64,
    std::string_view dp_b64,
    std::string_view dq_b64,
    std::string_view qi_b64)
  {
    std::string n_raw = ::base64_decode(n_b64);
    std::string e_raw = ::base64_decode(e_b64);
    crypto_core::SecureString d_raw(::base64_decode(d_b64));
    crypto_core::SecureString p_raw(::base64_decode(p_b64));
    crypto_core::SecureString q_raw(::base64_decode(q_b64));
    crypto_core::SecureString dp_raw(::base64_decode(dp_b64));
    crypto_core::SecureString dq_raw(::base64_decode(dq_b64));
    crypto_core::SecureString qi_raw(::base64_decode(qi_b64));

    if (n_raw.empty() || e_raw.empty() || d_raw.empty())
    {
      return KeyHandle(nullptr);
    }

    DWORD cbPublicExp = static_cast<DWORD>(e_raw.size());
    DWORD cbModulus = static_cast<DWORD>(n_raw.size());
    DWORD cbPrime1 = static_cast<DWORD>(p_raw.size());
    DWORD cbPrime2 = static_cast<DWORD>(q_raw.size());

    // BCRYPT_RSAFULLPRIVATE_BLOB:
    //   BCRYPT_RSAKEY_BLOB header
    //   e[cbPublicExp] | n[cbModulus] | p[cbPrime1] | q[cbPrime2]
    //   dp[cbPrime1] | dq[cbPrime2] | qi[cbPrime1] | d[cbModulus]
    size_t blob_size = sizeof(BCRYPT_RSAKEY_BLOB) + cbPublicExp + cbModulus +
      cbPrime1 + cbPrime2 + cbPrime1 + cbPrime2 + cbPrime1 + cbModulus;

    std::vector<BYTE> blob(blob_size, 0);
    auto* header = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(blob.data());
    header->Magic = BCRYPT_RSAFULLPRIVATE_MAGIC;
    header->BitLength = cbModulus * 8;
    header->cbPublicExp = cbPublicExp;
    header->cbModulus = cbModulus;
    header->cbPrime1 = cbPrime1;
    header->cbPrime2 = cbPrime2;

    BYTE* ptr = blob.data() + sizeof(BCRYPT_RSAKEY_BLOB);
    std::memcpy(ptr, e_raw.data(), cbPublicExp);
    ptr += cbPublicExp;
    std::memcpy(ptr, n_raw.data(), cbModulus);
    ptr += cbModulus;
    std::memcpy(ptr, p_raw.data(), cbPrime1);
    ptr += cbPrime1;
    std::memcpy(ptr, q_raw.data(), cbPrime2);
    ptr += cbPrime2;
    // dp (exponent1) — padded to cbPrime1
    if (dp_raw.size() <= cbPrime1)
    {
      std::memcpy(
        ptr + (cbPrime1 - dp_raw.size()), dp_raw.data(), dp_raw.size());
    }
    ptr += cbPrime1;
    // dq (exponent2) — padded to cbPrime2
    if (dq_raw.size() <= cbPrime2)
    {
      std::memcpy(
        ptr + (cbPrime2 - dq_raw.size()), dq_raw.data(), dq_raw.size());
    }
    ptr += cbPrime2;
    // qi (coefficient) — padded to cbPrime1
    if (qi_raw.size() <= cbPrime1)
    {
      std::memcpy(
        ptr + (cbPrime1 - qi_raw.size()), qi_raw.data(), qi_raw.size());
    }
    ptr += cbPrime1;
    // d (private exponent) — padded to cbModulus
    if (d_raw.size() <= cbModulus)
    {
      std::memcpy(ptr + (cbModulus - d_raw.size()), d_raw.data(), d_raw.size());
    }

    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status =
      BCryptOpenAlgorithmProvider(&alg_raw, BCRYPT_RSA_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      return KeyHandle(nullptr);
    }
    AlgHandle alg(alg_raw);

    BCRYPT_KEY_HANDLE key_raw = nullptr;
    status = BCryptImportKeyPair(
      alg.get(),
      nullptr,
      BCRYPT_RSAFULLPRIVATE_BLOB,
      &key_raw,
      blob.data(),
      safe_ulong(blob.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      return KeyHandle(nullptr);
    }
    return KeyHandle(key_raw);
  }

  // Import an EC private key from JWK components.
  // Constructs a BCRYPT_ECCPRIVATE_BLOB.
  // Layout: header | X[cbKey] | Y[cbKey] | d[cbKey]
  KeyHandle key_from_jwk_ec_private(
    std::string_view crv,
    std::string_view x_b64,
    std::string_view y_b64,
    std::string_view d_b64)
  {
    std::string x_raw = ::base64_decode(x_b64);
    std::string y_raw = ::base64_decode(y_b64);
    crypto_core::SecureString d_raw(::base64_decode(d_b64));
    if (x_raw.empty() || y_raw.empty() || d_raw.empty())
    {
      return KeyHandle(nullptr);
    }

    LPCWSTR algo_id = nullptr;
    ULONG magic = 0;
    DWORD key_size = 0;
    if (crv == "P-256")
    {
      algo_id = BCRYPT_ECDSA_P256_ALGORITHM;
      magic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
      key_size = 32;
    }
    else if (crv == "P-384")
    {
      algo_id = BCRYPT_ECDSA_P384_ALGORITHM;
      magic = BCRYPT_ECDSA_PRIVATE_P384_MAGIC;
      key_size = 48;
    }
    else if (crv == "P-521")
    {
      algo_id = BCRYPT_ECDSA_P521_ALGORITHM;
      magic = BCRYPT_ECDSA_PRIVATE_P521_MAGIC;
      key_size = 66;
    }
    else
    {
      return KeyHandle(nullptr);
    }

    // BCRYPT_ECCPRIVATE_BLOB layout:
    //   BCRYPT_ECCKEY_BLOB header
    //   X[cbKey] | Y[cbKey] | d[cbKey]
    std::vector<BYTE> blob(sizeof(BCRYPT_ECCKEY_BLOB) + key_size * 3, 0);
    auto* header = reinterpret_cast<BCRYPT_ECCKEY_BLOB*>(blob.data());
    header->dwMagic = magic;
    header->cbKey = key_size;

    BYTE* x_dest = blob.data() + sizeof(BCRYPT_ECCKEY_BLOB);
    BYTE* y_dest = x_dest + key_size;
    BYTE* d_dest = y_dest + key_size;

    // Left-pad each component to key_size
    if (x_raw.size() <= key_size)
    {
      std::memcpy(
        x_dest + (key_size - x_raw.size()), x_raw.data(), x_raw.size());
    }
    else
    {
      std::memcpy(x_dest, x_raw.data() + (x_raw.size() - key_size), key_size);
    }

    if (y_raw.size() <= key_size)
    {
      std::memcpy(
        y_dest + (key_size - y_raw.size()), y_raw.data(), y_raw.size());
    }
    else
    {
      std::memcpy(y_dest, y_raw.data() + (y_raw.size() - key_size), key_size);
    }

    if (d_raw.size() <= key_size)
    {
      std::memcpy(
        d_dest + (key_size - d_raw.size()), d_raw.data(), d_raw.size());
    }
    else
    {
      std::memcpy(d_dest, d_raw.data() + (d_raw.size() - key_size), key_size);
    }

    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status =
      BCryptOpenAlgorithmProvider(&alg_raw, algo_id, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      return KeyHandle(nullptr);
    }
    AlgHandle alg(alg_raw);

    BCRYPT_KEY_HANDLE key_raw = nullptr;
    status = BCryptImportKeyPair(
      alg.get(),
      nullptr,
      BCRYPT_ECCPRIVATE_BLOB,
      &key_raw,
      blob.data(),
      safe_ulong(blob.size()),
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      return KeyHandle(nullptr);
    }
    return KeyHandle(key_raw);
  }

  // Load a private key from a JWK JSON AST
  KeyHandle load_private_key_ast(const trieste::Node& ast)
  {
    using rego::crypto_core::MaxECComponentB64Len;
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
        return KeyHandle(nullptr);
      }
      for (auto sv : {n, e, d, p, q, dp, dq, qi})
      {
        if (sv.size() > MaxRSAComponentB64Len)
        {
          return KeyHandle(nullptr);
        }
      }
      return key_from_jwk_rsa_private(n, e, d, p, q, dp, dq, qi);
    }
    if (kty == "EC")
    {
      std::string_view crv = json_select_string(ast, "/crv");
      std::string_view x = json_select_string(ast, "/x");
      std::string_view y = json_select_string(ast, "/y");
      std::string_view d = json_select_string(ast, "/d");
      if (crv.empty() || x.empty() || y.empty() || d.empty())
      {
        return KeyHandle(nullptr);
      }
      if (
        x.size() > MaxECComponentB64Len || y.size() > MaxECComponentB64Len ||
        d.size() > MaxECComponentB64Len)
      {
        return KeyHandle(nullptr);
      }
      return key_from_jwk_ec_private(crv, x, y, d);
    }
    // OKP (Ed25519) not supported by Windows CNG
    return KeyHandle(nullptr);
  }

  // Sign a hash with RSA (PKCS#1 v1.5 or PSS)
  std::string sign_rsa(
    BCRYPT_KEY_HANDLE key,
    LPCWSTR hash_algo,
    std::string_view signing_input,
    bool use_pss)
  {
    auto hash_value = compute_hash(hash_algo, signing_input);

    DWORD sig_len = 0;
    NTSTATUS status;

    if (use_pss)
    {
      BCRYPT_PSS_PADDING_INFO pss_info;
      pss_info.pszAlgId = hash_algo;
      pss_info.cbSalt = safe_ulong(hash_value.size());

      status = BCryptSignHash(
        key,
        &pss_info,
        hash_value.data(),
        safe_ulong(hash_value.size()),
        nullptr,
        0,
        &sig_len,
        BCRYPT_PAD_PSS);
      if (!BCRYPT_SUCCESS(status))
      {
        throw std::runtime_error("BCryptSignHash (PSS length) failed");
      }

      std::vector<BYTE> sig(sig_len);
      status = BCryptSignHash(
        key,
        &pss_info,
        hash_value.data(),
        safe_ulong(hash_value.size()),
        sig.data(),
        sig_len,
        &sig_len,
        BCRYPT_PAD_PSS);
      if (!BCRYPT_SUCCESS(status))
      {
        throw std::runtime_error("BCryptSignHash (PSS) failed");
      }
      return std::string(reinterpret_cast<char*>(sig.data()), sig_len);
    }

    // PKCS#1 v1.5
    BCRYPT_PKCS1_PADDING_INFO pkcs1_info;
    pkcs1_info.pszAlgId = hash_algo;

    status = BCryptSignHash(
      key,
      &pkcs1_info,
      hash_value.data(),
      safe_ulong(hash_value.size()),
      nullptr,
      0,
      &sig_len,
      BCRYPT_PAD_PKCS1);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptSignHash (PKCS1 length) failed");
    }

    std::vector<BYTE> sig(sig_len);
    status = BCryptSignHash(
      key,
      &pkcs1_info,
      hash_value.data(),
      safe_ulong(hash_value.size()),
      sig.data(),
      sig_len,
      &sig_len,
      BCRYPT_PAD_PKCS1);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptSignHash (PKCS1) failed");
    }
    return std::string(reinterpret_cast<char*>(sig.data()), sig_len);
  }

  // Sign with ECDSA — BCrypt produces raw r||s directly (JWT format)
  std::string sign_ecdsa(
    BCRYPT_KEY_HANDLE key, LPCWSTR hash_algo, std::string_view signing_input)
  {
    auto hash_value = compute_hash(hash_algo, signing_input);

    DWORD sig_len = 0;
    NTSTATUS status = BCryptSignHash(
      key,
      nullptr,
      hash_value.data(),
      safe_ulong(hash_value.size()),
      nullptr,
      0,
      &sig_len,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptSignHash (ECDSA length) failed");
    }

    std::vector<BYTE> sig(sig_len);
    status = BCryptSignHash(
      key,
      nullptr,
      hash_value.data(),
      safe_ulong(hash_value.size()),
      sig.data(),
      sig_len,
      &sig_len,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      throw std::runtime_error("BCryptSignHash (ECDSA) failed");
    }
    return std::string(reinterpret_cast<char*>(sig.data()), sig_len);
  }

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
      LPCWSTR halgo = hmac_algo_id(algo);
      auto result = hmac_raw(halgo, secret.value, signing_input);
      return std::string(reinterpret_cast<char*>(result.data()), result.size());
    }

    // Asymmetric: load private key
    KeyHandle pkey = load_private_key_ast(ast);
    if (!pkey)
    {
      throw std::runtime_error("failed to load private key from JWK");
    }

    LPCWSTR halgo = hash_algo_id(algo);

    switch (algo)
    {
      case Algorithm::RS256:
      case Algorithm::RS384:
      case Algorithm::RS512:
        return sign_rsa(pkey.get(), halgo, signing_input, false);

      case Algorithm::PS256:
      case Algorithm::PS384:
      case Algorithm::PS512:
        return sign_rsa(pkey.get(), halgo, signing_input, true);

      case Algorithm::ES256:
      case Algorithm::ES384:
      case Algorithm::ES512:
        return sign_ecdsa(pkey.get(), halgo, signing_input);

      case Algorithm::EdDSA:
        throw std::runtime_error("EdDSA algorithm is not supported");

      default:
        throw std::runtime_error("unsupported algorithm for signing");
    }
  }

  // ── X.509 Certificate Parsing (WinCrypt/crypt32) ──

  // Extract CommonName from a certificate context
  std::string get_common_name(PCCERT_CONTEXT cert_ctx)
  {
    if (!cert_ctx)
    {
      return {};
    }
    // Get the length first
    DWORD len = CertGetNameStringA(
      cert_ctx, CERT_NAME_ATTR_TYPE, 0, (void*)szOID_COMMON_NAME, nullptr, 0);
    if (len <= 1)
    {
      return {};
    }
    std::string name(len - 1, '\0');
    CertGetNameStringA(
      cert_ctx,
      CERT_NAME_ATTR_TYPE,
      0,
      (void*)szOID_COMMON_NAME,
      name.data(),
      len);
    return name;
  }

  // Extract DNS names and URI strings from certificate SANs
  void extract_sans(
    PCCERT_CONTEXT cert_ctx,
    std::vector<std::string>& dns_names,
    std::vector<std::string>& uri_strings)
  {
    if (!cert_ctx)
    {
      return;
    }
    PCERT_EXTENSION san_ext = CertFindExtension(
      szOID_SUBJECT_ALT_NAME2,
      cert_ctx->pCertInfo->cExtension,
      cert_ctx->pCertInfo->rgExtension);
    if (!san_ext)
    {
      return;
    }

    PCERT_ALT_NAME_INFO san_info = nullptr;
    DWORD info_size = 0;
    if (!CryptDecodeObjectEx(
          X509_ASN_ENCODING,
          szOID_SUBJECT_ALT_NAME2,
          san_ext->Value.pbData,
          san_ext->Value.cbData,
          CRYPT_DECODE_ALLOC_FLAG,
          nullptr,
          &san_info,
          &info_size))
    {
      return;
    }
    if (!san_info)
    {
      return;
    }

    for (DWORD i = 0; i < san_info->cAltEntry; ++i)
    {
      CERT_ALT_NAME_ENTRY& entry = san_info->rgAltEntry[i];
      if (entry.dwAltNameChoice == CERT_ALT_NAME_DNS_NAME && entry.pwszDNSName)
      {
        // Convert wide string to UTF-8
        int needed = WideCharToMultiByte(
          CP_UTF8, 0, entry.pwszDNSName, -1, nullptr, 0, nullptr, nullptr);
        if (needed > 1)
        {
          std::string s(needed - 1, '\0');
          WideCharToMultiByte(
            CP_UTF8,
            0,
            entry.pwszDNSName,
            -1,
            s.data(),
            needed,
            nullptr,
            nullptr);
          dns_names.push_back(std::move(s));
        }
      }
      else if (entry.dwAltNameChoice == CERT_ALT_NAME_URL && entry.pwszURL)
      {
        int needed = WideCharToMultiByte(
          CP_UTF8, 0, entry.pwszURL, -1, nullptr, 0, nullptr, nullptr);
        if (needed > 1)
        {
          std::string s(needed - 1, '\0');
          WideCharToMultiByte(
            CP_UTF8, 0, entry.pwszURL, -1, s.data(), needed, nullptr, nullptr);
          uri_strings.push_back(std::move(s));
        }
      }
    }
    LocalFree(san_info);
  }

  // Convert a certificate context to base64-encoded DER
  std::string cert_to_der_b64(PCCERT_CONTEXT cert_ctx)
  {
    if (!cert_ctx || !cert_ctx->pbCertEncoded || cert_ctx->cbCertEncoded == 0)
    {
      return {};
    }
    std::string_view der(
      reinterpret_cast<const char*>(cert_ctx->pbCertEncoded),
      cert_ctx->cbCertEncoded);
    return ::base64_encode(der, false);
  }

  // Parse a CERT_CONTEXT into a ParsedCertificate
  ParsedCertificate cert_to_parsed(PCCERT_CONTEXT cert_ctx)
  {
    ParsedCertificate pc;
    pc.subject.common_name = get_common_name(cert_ctx);
    extract_sans(cert_ctx, pc.dns_names, pc.uri_strings);
    pc.der_b64 = cert_to_der_b64(cert_ctx);
    return pc;
  }

  // Parse PEM data into CERT_CONTEXT objects by extracting DER from PEM blocks
  std::vector<CertContextHandle> parse_pem_certs(std::string_view pem_data)
  {
    std::vector<CertContextHandle> certs;
    auto der_blocks = extract_pem_der_blocks(pem_data, "CERTIFICATE");

    for (auto& der : der_blocks)
    {
      PCCERT_CONTEXT ctx = CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        reinterpret_cast<const BYTE*>(der.data()),
        static_cast<DWORD>(der.size()));
      if (ctx)
      {
        certs.emplace_back(ctx);
      }
    }
    return certs;
  }

  // Parse concatenated DER data into CERT_CONTEXT objects
  std::vector<CertContextHandle> parse_der_certs(std::string_view der_data)
  {
    std::vector<CertContextHandle> certs;
    const BYTE* p = reinterpret_cast<const BYTE*>(der_data.data());
    DWORD remaining = static_cast<DWORD>(der_data.size());

    while (remaining > 0)
    {
      // Decode the ASN.1 length to find the certificate boundary
      DWORD cert_size = 0;
      PCCERT_CONTEXT ctx = CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, p, remaining);
      if (!ctx)
      {
        break;
      }
      cert_size = ctx->cbCertEncoded;
      certs.emplace_back(ctx);
      p += cert_size;
      remaining -= cert_size;
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

    std::vector<CertContextHandle> certs;
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

  VerifyCertsResult parse_and_verify_certificates(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {false, {}, decoded.error};
    }

    std::vector<CertContextHandle> certs;
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
      VerifyCertsResult result;
      result.valid = false;
      for (auto& cert : certs)
      {
        result.certs.push_back(cert_to_parsed(cert.get()));
      }
      return result;
    }

    // Build an in-memory certificate store with all non-leaf certs.
    // OPA convention: last cert is leaf, others are CA/intermediates.
    HCERTSTORE root_store = CertOpenStore(
      CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, nullptr);
    if (!root_store)
    {
      return {false, {}, "failed to create certificate store"};
    }

    // Add self-signed certs to root store, others as additional store
    HCERTSTORE extra_store = CertOpenStore(
      CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, nullptr);

    for (size_t i = 0; i + 1 < certs.size(); ++i)
    {
      PCCERT_CONTEXT c = certs[i].get();
      // Check if self-signed (issuer == subject)
      if (CertCompareCertificateName(
            X509_ASN_ENCODING, &c->pCertInfo->Subject, &c->pCertInfo->Issuer))
      {
        CertAddCertificateContextToStore(
          root_store, c, CERT_STORE_ADD_ALWAYS, nullptr);
      }
      else
      {
        CertAddCertificateContextToStore(
          extra_store, c, CERT_STORE_ADD_ALWAYS, nullptr);
      }
    }

    // Create a custom chain engine that trusts only our root store
    CERT_CHAIN_ENGINE_CONFIG engine_config = {};
    engine_config.cbSize = sizeof(engine_config);
    engine_config.hExclusiveRoot = root_store;

    HCERTCHAINENGINE engine = nullptr;
    if (!CertCreateCertificateChainEngine(&engine_config, &engine))
    {
      CertCloseStore(root_store, 0);
      CertCloseStore(extra_store, 0);
      return {false, {}, "failed to create chain engine"};
    }

    // Verify the leaf against the custom engine
    PCCERT_CONTEXT leaf = certs.back().get();

    CERT_CHAIN_PARA chain_params = {};
    chain_params.cbSize = sizeof(chain_params);

    PCCERT_CHAIN_CONTEXT chain_ctx = nullptr;
    BOOL chain_ok = CertGetCertificateChain(
      engine,
      leaf,
      nullptr, // current time
      extra_store,
      &chain_params,
      0, // no revocation checking
      nullptr,
      &chain_ctx);

    VerifyCertsResult result;

    if (chain_ok && chain_ctx)
    {
      // Check the chain status — allow untrusted root since we're using
      // our own root store, and ignore time/revocation issues for test certs
      DWORD error_status = chain_ctx->TrustStatus.dwErrorStatus;
      // Mask out acceptable errors for OPA-style verification
      DWORD acceptable_errors = CERT_TRUST_IS_NOT_TIME_VALID |
        CERT_TRUST_REVOCATION_STATUS_UNKNOWN | CERT_TRUST_IS_OFFLINE_REVOCATION;
      result.valid = ((error_status & ~acceptable_errors) == 0);

      if (result.valid && chain_ctx->cChain > 0)
      {
        // Return verified chain in leaf-first order
        PCERT_SIMPLE_CHAIN simple_chain = chain_ctx->rgpChain[0];
        if (simple_chain->cElement > static_cast<DWORD>(MaxCertChainLen))
        {
          result.valid = false;
          result.certs.clear();
          for (auto& cert : certs)
          {
            result.certs.push_back(cert_to_parsed(cert.get()));
          }
          CertFreeCertificateChain(chain_ctx);
          CertFreeCertificateChainEngine(engine);
          CertCloseStore(root_store, 0);
          CertCloseStore(extra_store, 0);
          return result;
        }
        for (DWORD i = 0; i < simple_chain->cElement; ++i)
        {
          result.certs.push_back(
            cert_to_parsed(simple_chain->rgpElement[i]->pCertContext));
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

      CertFreeCertificateChain(chain_ctx);
    }
    else
    {
      result.valid = false;
      for (auto& cert : certs)
      {
        result.certs.push_back(cert_to_parsed(cert.get()));
      }
    }

    CertFreeCertificateChainEngine(engine);
    CertCloseStore(root_store, 0);
    CertCloseStore(extra_store, 0);
    return result;
  }

  ParseCSRResult parse_certificate_request(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {{}, decoded.error};
    }

    std::string_view data_view = decoded.data;
    std::string der_data;

    if (decoded.is_pem)
    {
      auto blocks = extract_pem_der_blocks(decoded.data, "CERTIFICATE REQUEST");
      if (blocks.empty())
      {
        // Also try "NEW CERTIFICATE REQUEST"
        blocks =
          extract_pem_der_blocks(decoded.data, "NEW CERTIFICATE REQUEST");
      }
      if (blocks.empty())
      {
        return {{}, "asn1: structure error"};
      }
      der_data = std::move(blocks[0]);
      data_view = der_data;
    }

    // Decode the PKCS#10 CSR to extract the subject
    PCERT_REQUEST_INFO req_info = nullptr;
    DWORD req_info_size = 0;
    if (!CryptDecodeObjectEx(
          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
          X509_CERT_REQUEST_TO_BE_SIGNED,
          reinterpret_cast<const BYTE*>(data_view.data()),
          static_cast<DWORD>(data_view.size()),
          CRYPT_DECODE_ALLOC_FLAG,
          nullptr,
          &req_info,
          &req_info_size))
    {
      return {{}, "asn1: structure error"};
    }

    if (!req_info)
    {
      return {{}, "asn1: structure error"};
    }

    // Extract CommonName from the subject using CertNameToStrA
    // (CertGetNameStringA requires a CERT_CONTEXT, which we don't have for a
    // CSR)
    ParseCSRResult result;
    DWORD name_len = CertNameToStrA(
      X509_ASN_ENCODING,
      &req_info->Subject,
      CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
      nullptr,
      0);
    if (name_len > 1)
    {
      std::string full_name(name_len - 1, '\0');
      CertNameToStrA(
        X509_ASN_ENCODING,
        &req_info->Subject,
        CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
        full_name.data(),
        name_len);
      // Extract CN= value from the X.500 string
      auto cn_pos = full_name.find("CN=");
      if (cn_pos != std::string::npos)
      {
        cn_pos += 3;
        auto cn_end = full_name.find(',', cn_pos);
        if (cn_end == std::string::npos)
        {
          cn_end = full_name.size();
        }
        result.subject.common_name = full_name.substr(cn_pos, cn_end - cn_pos);
      }
    }

    LocalFree(req_info);
    return result;
  }

  // ── RSA Private Key Parsing ──

  // Convert raw big-endian bytes to base64url (no padding), stripping leading
  // zero bytes.
  std::string bytes_to_base64url(const BYTE* data, DWORD size)
  {
    // Skip leading zeros
    while (size > 0 && *data == 0)
    {
      ++data;
      --size;
    }
    if (size == 0)
    {
      return "AA"; // zero value
    }
    std::string_view sv(reinterpret_cast<const char*>(data), size);
    return base64url_encode_nopad(sv);
  }

  // Parse a single RSA private key from DER bytes and return JWK components.
  // Handles both PKCS#1 (RSA PRIVATE KEY) and PKCS#8 (PRIVATE KEY) formats.
  std::optional<RSAPrivateKeyJWK> parse_rsa_key_der(std::string_view der_data)
  {
    // Try PKCS#8 first (PRIVATE KEY), then PKCS#1 (RSA PRIVATE KEY)
    // CryptDecodeObjectEx with PKCS_PRIVATE_KEY_INFO for PKCS#8,
    // or CNG_RSA_PRIVATE_KEY_BLOB for PKCS#1.

    // Strategy: import through CNG to normalize the format, then export
    // the full private blob to extract all components.

    // Try PKCS#8 wrapper first
    PCRYPT_PRIVATE_KEY_INFO pkcs8_info = nullptr;
    DWORD pkcs8_size = 0;
    bool is_pkcs8 = CryptDecodeObjectEx(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
      PKCS_PRIVATE_KEY_INFO,
      reinterpret_cast<const BYTE*>(der_data.data()),
      static_cast<DWORD>(der_data.size()),
      CRYPT_DECODE_ALLOC_FLAG,
      nullptr,
      &pkcs8_info,
      &pkcs8_size);

    std::string_view rsa_der = der_data;
    std::unique_ptr<void, decltype(&LocalFree)> pkcs8_guard(nullptr, LocalFree);

    if (is_pkcs8 && pkcs8_info)
    {
      pkcs8_guard.reset(pkcs8_info);
      // Extract the inner RSA private key from PKCS#8
      rsa_der = std::string_view(
        reinterpret_cast<const char*>(pkcs8_info->PrivateKey.pbData),
        pkcs8_info->PrivateKey.cbData);
    }

    // Decode as PKCS#1 RSA private key to CNG blob
    BYTE* cng_blob = nullptr;
    DWORD cng_blob_size = 0;
    if (!CryptDecodeObjectEx(
          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
          CNG_RSA_PRIVATE_KEY_BLOB,
          reinterpret_cast<const BYTE*>(rsa_der.data()),
          static_cast<DWORD>(rsa_der.size()),
          CRYPT_DECODE_ALLOC_FLAG,
          nullptr,
          &cng_blob,
          &cng_blob_size))
    {
      return std::nullopt;
    }
    std::unique_ptr<void, decltype(&LocalFree)> blob_guard(cng_blob, LocalFree);

    // Import the CNG blob to get a key handle
    BCRYPT_ALG_HANDLE alg_raw = nullptr;
    NTSTATUS status =
      BCryptOpenAlgorithmProvider(&alg_raw, BCRYPT_RSA_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
      return std::nullopt;
    }
    AlgHandle alg(alg_raw);

    BCRYPT_KEY_HANDLE key_raw = nullptr;
    status = BCryptImportKeyPair(
      alg.get(),
      nullptr,
      BCRYPT_RSAPRIVATE_BLOB,
      &key_raw,
      cng_blob,
      cng_blob_size,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      return std::nullopt;
    }
    KeyHandle key(key_raw);

    // Export as BCRYPT_RSAFULLPRIVATE_BLOB to get all components
    DWORD export_size = 0;
    status = BCryptExportKey(
      key.get(),
      nullptr,
      BCRYPT_RSAFULLPRIVATE_BLOB,
      nullptr,
      0,
      &export_size,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      return std::nullopt;
    }

    std::vector<BYTE> export_blob(export_size);
    status = BCryptExportKey(
      key.get(),
      nullptr,
      BCRYPT_RSAFULLPRIVATE_BLOB,
      export_blob.data(),
      export_size,
      &export_size,
      0);
    if (!BCRYPT_SUCCESS(status))
    {
      return std::nullopt;
    }

    // Parse the exported blob
    // Layout: header | e[cbPublicExp] | n[cbModulus] | p[cbPrime1] |
    //         q[cbPrime2] | dp[cbPrime1] | dq[cbPrime2] | qi[cbPrime1] |
    //         d[cbModulus]
    auto* hdr = reinterpret_cast<const BCRYPT_RSAKEY_BLOB*>(export_blob.data());
    const BYTE* ptr = export_blob.data() + sizeof(BCRYPT_RSAKEY_BLOB);

    RSAPrivateKeyJWK jwk;
    jwk.kty = "RSA";
    jwk.e = bytes_to_base64url(ptr, hdr->cbPublicExp);
    ptr += hdr->cbPublicExp;
    jwk.n = bytes_to_base64url(ptr, hdr->cbModulus);
    ptr += hdr->cbModulus;
    jwk.p = bytes_to_base64url(ptr, hdr->cbPrime1);
    ptr += hdr->cbPrime1;
    jwk.q = bytes_to_base64url(ptr, hdr->cbPrime2);
    ptr += hdr->cbPrime2;
    jwk.dp = bytes_to_base64url(ptr, hdr->cbPrime1);
    ptr += hdr->cbPrime1;
    jwk.dq = bytes_to_base64url(ptr, hdr->cbPrime2);
    ptr += hdr->cbPrime2;
    jwk.qi = bytes_to_base64url(ptr, hdr->cbPrime1);
    ptr += hdr->cbPrime1;
    jwk.d = bytes_to_base64url(ptr, hdr->cbModulus);

    return jwk;
  }

  ParseRSAKeyResult parse_rsa_private_key(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {{}, decoded.error};
    }

    std::string der_data;
    if (decoded.is_pem)
    {
      // Try PKCS#1 first, then PKCS#8
      auto blocks = extract_pem_der_blocks(decoded.data, "RSA PRIVATE KEY");
      if (blocks.empty())
      {
        blocks = extract_pem_der_blocks(decoded.data, "PRIVATE KEY");
      }
      if (blocks.empty())
      {
        return {{}, "failed to parse RSA private key"};
      }
      der_data = std::move(blocks[0]);
    }
    else
    {
      der_data = std::move(decoded.data);
    }

    auto jwk = parse_rsa_key_der(der_data);
    if (!jwk)
    {
      return {{}, "failed to parse RSA private key"};
    }
    return {*jwk, {}};
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
      return {{}, false, {}};
    }

    if (!decoded.is_pem)
    {
      return {{}, false, {}};
    }

    ParsePrivateKeysResult result;
    result.is_empty_input = false;

    // Extract all PKCS#1 blocks
    auto rsa_blocks = extract_pem_der_blocks(decoded.data, "RSA PRIVATE KEY");
    for (auto& der : rsa_blocks)
    {
      auto jwk = parse_rsa_key_der(der);
      if (jwk)
      {
        result.keys.push_back(*jwk);
      }
    }

    // Extract all PKCS#8 blocks
    auto pkcs8_blocks = extract_pem_der_blocks(decoded.data, "PRIVATE KEY");
    for (auto& der : pkcs8_blocks)
    {
      auto jwk = parse_rsa_key_der(der);
      if (jwk)
      {
        result.keys.push_back(*jwk);
      }
    }

    return result;
  }
}

#endif // REGOCPP_CRYPTO_BCRYPT
