// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef REGOCPP_CRYPTO_MBEDTLS

#include "base64/base64.h"
#include "crypto_core.hh"
#include "crypto_utils.hh"

#include <algorithm>
#include <cstring>
#include <mbedtls/asn1.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/bignum.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <memory>
#include <stdexcept>
#include <trieste/json.h>
#include <vector>

namespace
{
  using rego::crypto_core::to_hex;

  // ── RAII wrappers ──

  struct MdCtx
  {
    mbedtls_md_context_t ctx;
    MdCtx()
    {
      mbedtls_md_init(&ctx);
    }
    ~MdCtx()
    {
      mbedtls_md_free(&ctx);
    }
    MdCtx(const MdCtx&) = delete;
    MdCtx& operator=(const MdCtx&) = delete;
  };

  struct PkCtx
  {
    mbedtls_pk_context ctx;
    PkCtx()
    {
      mbedtls_pk_init(&ctx);
    }
    ~PkCtx()
    {
      mbedtls_pk_free(&ctx);
    }
    PkCtx(const PkCtx&) = delete;
    PkCtx& operator=(const PkCtx&) = delete;
  };

  struct Mpi
  {
    mbedtls_mpi val;
    Mpi()
    {
      mbedtls_mpi_init(&val);
    }
    ~Mpi()
    {
      mbedtls_mpi_free(&val);
    }
    Mpi(const Mpi&) = delete;
    Mpi& operator=(const Mpi&) = delete;
  };

  struct EntropyCtx
  {
    mbedtls_entropy_context ctx;
    EntropyCtx()
    {
      mbedtls_entropy_init(&ctx);
    }
    ~EntropyCtx()
    {
      mbedtls_entropy_free(&ctx);
    }
    EntropyCtx(const EntropyCtx&) = delete;
    EntropyCtx& operator=(const EntropyCtx&) = delete;
  };

  struct CtrDrbgCtx
  {
    mbedtls_ctr_drbg_context ctx;
    CtrDrbgCtx()
    {
      mbedtls_ctr_drbg_init(&ctx);
    }
    ~CtrDrbgCtx()
    {
      mbedtls_ctr_drbg_free(&ctx);
    }
    CtrDrbgCtx(const CtrDrbgCtx&) = delete;
    CtrDrbgCtx& operator=(const CtrDrbgCtx&) = delete;
  };

  struct X509Crt
  {
    mbedtls_x509_crt crt;
    X509Crt()
    {
      mbedtls_x509_crt_init(&crt);
    }
    ~X509Crt()
    {
      mbedtls_x509_crt_free(&crt);
    }
    X509Crt(const X509Crt&) = delete;
    X509Crt& operator=(const X509Crt&) = delete;
  };

  struct X509Csr
  {
    mbedtls_x509_csr csr;
    X509Csr()
    {
      mbedtls_x509_csr_init(&csr);
    }
    ~X509Csr()
    {
      mbedtls_x509_csr_free(&csr);
    }
    X509Csr(const X509Csr&) = delete;
    X509Csr& operator=(const X509Csr&) = delete;
  };

  // ── RNG setup (shared) ──

  struct Rng
  {
    EntropyCtx entropy;
    CtrDrbgCtx drbg;

    Rng()
    {
      int ret = mbedtls_ctr_drbg_seed(
        &drbg.ctx, mbedtls_entropy_func, &entropy.ctx, nullptr, 0);
      if (ret != 0)
      {
        throw std::runtime_error("mbedtls_ctr_drbg_seed failed");
      }
    }
  };

  Rng& get_rng()
  {
    static Rng rng;
    return rng;
  }

  // ── Hash helpers ──

  const mbedtls_md_info_t* md_info_from_type(mbedtls_md_type_t type)
  {
    return mbedtls_md_info_from_type(type);
  }

  std::string digest_hex(mbedtls_md_type_t type, std::string_view data)
  {
    const mbedtls_md_info_t* info = md_info_from_type(type);
    if (!info)
    {
      throw std::runtime_error("unsupported digest type");
    }

    unsigned char buf[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md(
      info,
      reinterpret_cast<const unsigned char*>(data.data()),
      data.size(),
      buf);
    if (ret != 0)
    {
      throw std::runtime_error("mbedtls_md failed");
    }

    return to_hex(buf, mbedtls_md_get_size(info));
  }

  std::string hmac_hex(
    mbedtls_md_type_t type, std::string_view key, std::string_view data)
  {
    const mbedtls_md_info_t* info = md_info_from_type(type);
    if (!info)
    {
      throw std::runtime_error("unsupported digest type for HMAC");
    }

    unsigned char buf[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md_hmac(
      info,
      reinterpret_cast<const unsigned char*>(key.data()),
      key.size(),
      reinterpret_cast<const unsigned char*>(data.data()),
      data.size(),
      buf);
    if (ret != 0)
    {
      throw std::runtime_error("mbedtls_md_hmac failed");
    }

    return to_hex(buf, mbedtls_md_get_size(info));
  }

  // ── Algorithm mapping ──

  using rego::crypto_core::Algorithm;

  mbedtls_md_type_t md_type_for_algo(Algorithm algo)
  {
    switch (algo)
    {
      case Algorithm::HS256:
      case Algorithm::RS256:
      case Algorithm::PS256:
      case Algorithm::ES256:
        return MBEDTLS_MD_SHA256;
      case Algorithm::HS384:
      case Algorithm::RS384:
      case Algorithm::PS384:
      case Algorithm::ES384:
        return MBEDTLS_MD_SHA384;
      case Algorithm::HS512:
      case Algorithm::RS512:
      case Algorithm::PS512:
      case Algorithm::ES512:
        return MBEDTLS_MD_SHA512;
      case Algorithm::EdDSA:
        return MBEDTLS_MD_NONE;
    }
    return MBEDTLS_MD_NONE;
  }

  using rego::crypto_core::extract_jwks_keys;
  using rego::crypto_core::json_select_string;
  using rego::crypto_core::parse_json;

  // ── Key loading: PEM public keys and certificates ──

  bool pk_from_pem_pubkey(PkCtx& pk, std::string_view pem)
  {
    int ret = mbedtls_pk_parse_public_key(
      &pk.ctx,
      reinterpret_cast<const unsigned char*>(pem.data()),
      pem.size() + 1); // mbedtls requires null-terminated PEM
    return ret == 0;
  }

  bool pk_from_certificate(PkCtx& pk, std::string_view cert_pem)
  {
    X509Crt crt;
    int ret = mbedtls_x509_crt_parse(
      &crt.crt,
      reinterpret_cast<const unsigned char*>(cert_pem.data()),
      cert_pem.size() + 1);
    if (ret != 0)
    {
      return false;
    }

    // Copy the public key from the certificate.
    // We need to export and re-import the key.
    // 8192 bytes is sufficient for RSA keys up to 16384 bits.
    unsigned char buf[8192];
    int len = mbedtls_pk_write_pubkey_der(&crt.crt.pk, buf, sizeof(buf));
    if (len < 0)
    {
      return false;
    }

    // mbedtls_pk_write_pubkey_der writes from end of buffer
    ret = mbedtls_pk_parse_public_key(
      &pk.ctx, buf + sizeof(buf) - len, static_cast<size_t>(len));
    return ret == 0;
  }

  // ── Key loading: JWK ──

  // Load MPI from base64url-encoded big integer
  bool mpi_from_base64url(Mpi& mpi, std::string_view b64)
  {
    rego::crypto_core::SecureString raw(::base64_decode(b64));
    return mbedtls_mpi_read_binary(
             &mpi.val,
             reinterpret_cast<const unsigned char*>(raw.data()),
             raw.size()) == 0;
  }

  // Parse a JWK RSA key into a PK context (public key only)
  bool pk_from_jwk_rsa(
    PkCtx& pk, std::string_view n_b64, std::string_view e_b64)
  {
    Mpi n, e;
    if (!mpi_from_base64url(n, n_b64) || !mpi_from_base64url(e, e_b64))
    {
      return false;
    }

    int ret =
      mbedtls_pk_setup(&pk.ctx, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret != 0)
    {
      return false;
    }

    mbedtls_rsa_context* rsa = mbedtls_pk_rsa(pk.ctx);
    ret = mbedtls_rsa_import(rsa, &n.val, nullptr, nullptr, nullptr, &e.val);
    if (ret != 0)
    {
      return false;
    }
    return mbedtls_rsa_complete(rsa) == 0;
  }

  // Map JWK curve name to mbedtls group ID
  mbedtls_ecp_group_id ec_group_from_crv(std::string_view crv)
  {
    if (crv == "P-256")
      return MBEDTLS_ECP_DP_SECP256R1;
    if (crv == "P-384")
      return MBEDTLS_ECP_DP_SECP384R1;
    if (crv == "P-521")
      return MBEDTLS_ECP_DP_SECP521R1;
    return MBEDTLS_ECP_DP_NONE;
  }

  // Parse a JWK EC key into a PK context (public key only)
  bool pk_from_jwk_ec(
    PkCtx& pk,
    std::string_view crv,
    std::string_view x_b64,
    std::string_view y_b64)
  {
    mbedtls_ecp_group_id grp_id = ec_group_from_crv(crv);
    if (grp_id == MBEDTLS_ECP_DP_NONE)
    {
      return false;
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

    int ret =
      mbedtls_pk_setup(&pk.ctx, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if (ret != 0)
    {
      return false;
    }

    // Mbed TLS 3.6: use opaque API for EC keypair
    mbedtls_ecp_keypair* ec = mbedtls_pk_ec(pk.ctx);
    mbedtls_ecp_group grp;
    mbedtls_ecp_group_init(&grp);
    ret = mbedtls_ecp_group_load(&grp, grp_id);
    if (ret != 0)
    {
      mbedtls_ecp_group_free(&grp);
      return false;
    }

    mbedtls_ecp_point Q;
    mbedtls_ecp_point_init(&Q);
    ret = mbedtls_ecp_point_read_binary(&grp, &Q, point.data(), point.size());
    mbedtls_ecp_group_free(&grp);
    if (ret != 0)
    {
      mbedtls_ecp_point_free(&Q);
      return false;
    }

    ret = mbedtls_ecp_set_public_key(grp_id, ec, &Q);
    mbedtls_ecp_point_free(&Q);
    return ret == 0;
  }

  // Dispatch JWK AST to appropriate key loader
  bool pk_from_jwk_ast(PkCtx& pk, const trieste::Node& ast)
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
        return false;
      }
      return pk_from_jwk_rsa(pk, n, e);
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
        return false;
      }
      return pk_from_jwk_ec(pk, crv, x, y);
    }
    // OKP (Ed25519) not supported in mbedtls backend
    return false;
  }

  // Auto-detect key format and load PK context.
  bool load_public_key(
    PkCtx& pk, std::string_view key_or_cert, std::string_view kid = {})
  {
    // PEM certificate
    if (
      key_or_cert.find("-----BEGIN CERTIFICATE-----") != std::string_view::npos)
    {
      return pk_from_certificate(pk, key_or_cert);
    }

    // PEM public key
    if (
      key_or_cert.find("-----BEGIN PUBLIC KEY-----") != std::string_view::npos)
    {
      return pk_from_pem_pubkey(pk, key_or_cert);
    }

    // Try JSON (JWK or JWKS)
    auto ast = parse_json(key_or_cert);
    if (!ast)
    {
      return false;
    }

    // Try JWKS first
    auto keys = extract_jwks_keys(ast);
    if (!keys.empty())
    {
      for (auto& key_ast : keys)
      {
        if (kid.empty() || json_select_string(key_ast, "/kid") == kid)
        {
          return pk_from_jwk_ast(pk, key_ast);
        }
      }
      return false;
    }

    // Single JWK
    std::string_view kty = json_select_string(ast, "/kty");
    if (!kty.empty())
    {
      return pk_from_jwk_ast(pk, ast);
    }

    return false;
  }

  // ── Signature verification helpers ──

  bool verify_hmac(
    Algorithm algo,
    std::string_view signing_input,
    std::string_view sig_bytes,
    std::string_view secret)
  {
    mbedtls_md_type_t type = md_type_for_algo(algo);
    const mbedtls_md_info_t* info = md_info_from_type(type);
    if (!info)
    {
      return false;
    }

    unsigned char buf[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md_hmac(
      info,
      reinterpret_cast<const unsigned char*>(secret.data()),
      secret.size(),
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      buf);
    if (ret != 0)
    {
      return false;
    }

    size_t md_size = mbedtls_md_get_size(info);
    if (md_size != sig_bytes.size())
    {
      return false;
    }

    // Constant-time comparison
    volatile unsigned char result = 0;
    for (size_t i = 0; i < md_size; ++i)
    {
      result |= static_cast<unsigned char>(buf[i]) ^
        static_cast<unsigned char>(sig_bytes[i]);
    }
    return result == 0;
  }

  bool verify_rsa_pkcs1(
    mbedtls_md_type_t md_type,
    PkCtx& pk,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    const mbedtls_md_info_t* info = md_info_from_type(md_type);
    if (!info)
    {
      return false;
    }

    // Hash the signing input
    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md(
      info,
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      hash);
    if (ret != 0)
    {
      return false;
    }

    size_t hash_len = mbedtls_md_get_size(info);

    // Ensure PKCS#1 v1.5 padding is set. The cached PkCtx may have been
    // previously configured for PSS (PKCS_V21) by verify_rsa_pss.
    mbedtls_rsa_context* rsa = mbedtls_pk_rsa(pk.ctx);
    if (rsa)
    {
      mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15, md_type);
    }

    // Verify signature
    ret = mbedtls_pk_verify(
      &pk.ctx,
      md_type,
      hash,
      hash_len,
      reinterpret_cast<const unsigned char*>(sig_bytes.data()),
      sig_bytes.size());
    return ret == 0;
  }

  bool verify_rsa_pss(
    mbedtls_md_type_t md_type,
    PkCtx& pk,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    const mbedtls_md_info_t* info = md_info_from_type(md_type);
    if (!info)
    {
      return false;
    }

    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md(
      info,
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      hash);
    if (ret != 0)
    {
      return false;
    }

    size_t hash_len = mbedtls_md_get_size(info);

    // Use RSA-PSS verification
    mbedtls_rsa_context* rsa = mbedtls_pk_rsa(pk.ctx);
    if (!rsa)
    {
      return false;
    }

    ret = mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V21, md_type);
    if (ret != 0)
    {
      return false;
    }

    // mbedtls_rsa_rsassa_pss_verify reads exactly rsa->len bytes from the
    // signature buffer (the RSA modulus size). JWT signatures decoded from
    // base64url may be shorter if leading zero bytes were stripped. Pad the
    // signature to the expected length to avoid a heap-buffer-overflow.
    size_t key_len = mbedtls_rsa_get_len(rsa);
    std::vector<unsigned char> sig_padded(key_len, 0);
    if (sig_bytes.size() > key_len)
    {
      return false;
    }
    std::memcpy(
      sig_padded.data() + (key_len - sig_bytes.size()),
      sig_bytes.data(),
      sig_bytes.size());

    ret = mbedtls_rsa_rsassa_pss_verify(
      rsa,
      md_type,
      static_cast<unsigned int>(hash_len),
      hash,
      sig_padded.data());
    return ret == 0;
  }

  bool verify_ecdsa(
    mbedtls_md_type_t md_type,
    PkCtx& pk,
    std::string_view signing_input,
    std::string_view sig_bytes)
  {
    // Validate signature length against the expected curve size.
    // ES256 (SHA-256, P-256): 64 bytes, ES384 (SHA-384, P-384): 96 bytes,
    // ES512 (SHA-512, P-521): 132 bytes.
    size_t expected_sig_len = 0;
    switch (md_type)
    {
      case MBEDTLS_MD_SHA256:
        expected_sig_len = 64;
        break;
      case MBEDTLS_MD_SHA384:
        expected_sig_len = 96;
        break;
      case MBEDTLS_MD_SHA512:
        expected_sig_len = 132;
        break;
      default:
        return false;
    }
    if (sig_bytes.empty() || sig_bytes.size() != expected_sig_len)
    {
      return false;
    }

    const mbedtls_md_info_t* info = md_info_from_type(md_type);
    if (!info)
    {
      return false;
    }

    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md(
      info,
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      hash);
    if (ret != 0)
    {
      return false;
    }

    size_t hash_len = mbedtls_md_get_size(info);

    // JWT ECDSA signatures are raw R||S; mbedtls_pk_verify expects DER.
    // Convert raw to DER ASN.1 SEQUENCE { INTEGER r, INTEGER s }
    size_t half = sig_bytes.size() / 2;
    Mpi r, s;
    mbedtls_mpi_read_binary(
      &r.val, reinterpret_cast<const unsigned char*>(sig_bytes.data()), half);
    mbedtls_mpi_read_binary(
      &s.val,
      reinterpret_cast<const unsigned char*>(sig_bytes.data()) + half,
      half);

    // Encode as DER: SEQUENCE { INTEGER r, INTEGER s }
    unsigned char der_buf[256];
    unsigned char* p = der_buf + sizeof(der_buf);
    size_t len = 0;

    // Write s
    ret = mbedtls_asn1_write_mpi(&p, der_buf, &s.val);
    if (ret < 0)
    {
      return false;
    }
    len += static_cast<size_t>(ret);

    // Write r
    ret = mbedtls_asn1_write_mpi(&p, der_buf, &r.val);
    if (ret < 0)
    {
      return false;
    }
    len += static_cast<size_t>(ret);

    // Write SEQUENCE tag + length
    ret = mbedtls_asn1_write_len(&p, der_buf, len);
    if (ret < 0)
    {
      return false;
    }
    len += static_cast<size_t>(ret);

    ret = mbedtls_asn1_write_tag(
      &p, der_buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    if (ret < 0)
    {
      return false;
    }
    len += static_cast<size_t>(ret);

    ret = mbedtls_pk_verify(&pk.ctx, md_type, hash, hash_len, p, len);
    return ret == 0;
  }

  // ── PEM validation ──

  // ── Signing: private key loading ──

  bool pk_from_jwk_rsa_private(
    PkCtx& pk,
    std::string_view n_b64,
    std::string_view e_b64,
    std::string_view d_b64,
    std::string_view p_b64,
    std::string_view q_b64,
    std::string_view dp_b64,
    std::string_view dq_b64,
    std::string_view qi_b64)
  {
    Mpi n, e, d, p, q;

    if (
      !mpi_from_base64url(n, n_b64) || !mpi_from_base64url(e, e_b64) ||
      !mpi_from_base64url(d, d_b64))
    {
      return false;
    }

    // p, q, dp, dq, qi are optional for mbedtls_rsa_complete
    bool have_pq = !p_b64.empty() && !q_b64.empty() &&
      mpi_from_base64url(p, p_b64) && mpi_from_base64url(q, q_b64);

    int ret =
      mbedtls_pk_setup(&pk.ctx, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret != 0)
    {
      return false;
    }

    mbedtls_rsa_context* rsa = mbedtls_pk_rsa(pk.ctx);
    ret = mbedtls_rsa_import(
      rsa,
      &n.val,
      have_pq ? &p.val : nullptr,
      have_pq ? &q.val : nullptr,
      &d.val,
      &e.val);
    if (ret != 0)
    {
      return false;
    }

    // mbedtls_rsa_complete will derive dp, dq, qi from d, p, q
    return mbedtls_rsa_complete(rsa) == 0;
  }

  bool pk_from_jwk_ec_private(
    PkCtx& pk,
    std::string_view crv,
    std::string_view x_b64,
    std::string_view y_b64,
    std::string_view d_b64)
  {
    mbedtls_ecp_group_id grp_id = ec_group_from_crv(crv);
    if (grp_id == MBEDTLS_ECP_DP_NONE)
    {
      return false;
    }

    std::string x_raw = ::base64_decode(x_b64);
    std::string y_raw = ::base64_decode(y_b64);
    rego::crypto_core::SecureString d_raw(::base64_decode(d_b64));

    // Build uncompressed point
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

    int ret =
      mbedtls_pk_setup(&pk.ctx, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if (ret != 0)
    {
      return false;
    }

    // Set private key
    mbedtls_ecp_keypair* ec = mbedtls_pk_ec(pk.ctx);
    ret = mbedtls_ecp_read_key(
      grp_id,
      ec,
      reinterpret_cast<const unsigned char*>(d_raw.data()),
      d_raw.size());
    if (ret != 0)
    {
      return false;
    }

    // Set public key
    mbedtls_ecp_group grp;
    mbedtls_ecp_group_init(&grp);
    ret = mbedtls_ecp_group_load(&grp, grp_id);
    if (ret != 0)
    {
      mbedtls_ecp_group_free(&grp);
      return false;
    }

    mbedtls_ecp_point Q;
    mbedtls_ecp_point_init(&Q);
    ret = mbedtls_ecp_point_read_binary(&grp, &Q, point.data(), point.size());
    mbedtls_ecp_group_free(&grp);
    if (ret != 0)
    {
      mbedtls_ecp_point_free(&Q);
      return false;
    }

    ret = mbedtls_ecp_set_public_key(grp_id, ec, &Q);
    mbedtls_ecp_point_free(&Q);
    return ret == 0;
  }

  bool load_private_key_ast(
    PkCtx& pk, std::string& ed25519_raw, const trieste::Node& ast)
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
        return false;
      }
      for (auto sv : {n, e, d, p, q, dp, dq, qi})
      {
        if (sv.size() > MaxRSAComponentB64Len)
        {
          return false;
        }
      }
      return pk_from_jwk_rsa_private(pk, n, e, d, p, q, dp, dq, qi);
    }
    if (kty == "EC")
    {
      std::string_view crv = json_select_string(ast, "/crv");
      std::string_view x = json_select_string(ast, "/x");
      std::string_view y = json_select_string(ast, "/y");
      std::string_view d = json_select_string(ast, "/d");
      if (crv.empty() || x.empty() || y.empty() || d.empty())
      {
        return false;
      }
      if (
        x.size() > MaxECComponentB64Len || y.size() > MaxECComponentB64Len ||
        d.size() > MaxECComponentB64Len)
      {
        return false;
      }
      return pk_from_jwk_ec_private(pk, crv, x, y, d);
    }
    if (kty == "OKP")
    {
      std::string_view d = json_select_string(ast, "/d");
      if (d.empty() || d.size() > MaxOKPComponentB64Len)
      {
        return false;
      }
      ed25519_raw = ::base64_decode(d);
      return true;
    }
    return false;
  }

  // ── Signing helpers ──

  std::string sign_hmac(
    Algorithm algo, std::string_view signing_input, std::string_view secret)
  {
    mbedtls_md_type_t type = md_type_for_algo(algo);
    const mbedtls_md_info_t* info = md_info_from_type(type);
    if (!info)
    {
      throw std::runtime_error("unsupported digest type for HMAC signing");
    }

    unsigned char buf[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md_hmac(
      info,
      reinterpret_cast<const unsigned char*>(secret.data()),
      secret.size(),
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      buf);
    if (ret != 0)
    {
      throw std::runtime_error("HMAC signing failed");
    }

    size_t md_size = mbedtls_md_get_size(info);
    return std::string(reinterpret_cast<char*>(buf), md_size);
  }

  std::string sign_rsa(
    mbedtls_md_type_t md_type,
    PkCtx& pk,
    std::string_view signing_input,
    bool use_pss)
  {
    const mbedtls_md_info_t* info = md_info_from_type(md_type);
    if (!info)
    {
      throw std::runtime_error("unsupported digest type for RSA signing");
    }

    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md(
      info,
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      hash);
    if (ret != 0)
    {
      throw std::runtime_error("hash for RSA signing failed");
    }

    size_t hash_len = mbedtls_md_get_size(info);
    mbedtls_rsa_context* rsa = mbedtls_pk_rsa(pk.ctx);

    if (use_pss)
    {
      mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V21, md_type);
    }
    else
    {
      mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15, md_type);
    }

    size_t sig_len = mbedtls_rsa_get_len(rsa);
    std::vector<unsigned char> sig(sig_len);

    auto& rng = get_rng();
    if (use_pss)
    {
      ret = mbedtls_rsa_rsassa_pss_sign(
        rsa,
        mbedtls_ctr_drbg_random,
        &rng.drbg.ctx,
        md_type,
        static_cast<unsigned int>(hash_len),
        hash,
        sig.data());
    }
    else
    {
      ret = mbedtls_rsa_rsassa_pkcs1_v15_sign(
        rsa,
        mbedtls_ctr_drbg_random,
        &rng.drbg.ctx,
        md_type,
        static_cast<unsigned int>(hash_len),
        hash,
        sig.data());
    }

    if (ret != 0)
    {
      throw std::runtime_error("RSA signing failed");
    }

    return std::string(reinterpret_cast<char*>(sig.data()), sig.size());
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

  std::string sign_ecdsa(
    mbedtls_md_type_t md_type,
    PkCtx& pk,
    std::string_view signing_input,
    Algorithm algo)
  {
    const mbedtls_md_info_t* info = md_info_from_type(md_type);
    if (!info)
    {
      throw std::runtime_error("unsupported digest type for ECDSA signing");
    }

    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
    int ret = mbedtls_md(
      info,
      reinterpret_cast<const unsigned char*>(signing_input.data()),
      signing_input.size(),
      hash);
    if (ret != 0)
    {
      throw std::runtime_error("hash for ECDSA signing failed");
    }

    size_t hash_len = mbedtls_md_get_size(info);

    // Sign — produces DER-encoded signature
    unsigned char der_sig[256];
    size_t der_len = 0;
    auto& rng = get_rng();

    ret = mbedtls_pk_sign(
      &pk.ctx,
      md_type,
      hash,
      hash_len,
      der_sig,
      sizeof(der_sig),
      &der_len,
      mbedtls_ctr_drbg_random,
      &rng.drbg.ctx);
    if (ret != 0)
    {
      throw std::runtime_error("ECDSA signing failed");
    }

    // Convert DER to raw R||S
    // Parse the DER SEQUENCE { INTEGER r, INTEGER s }
    unsigned char* p = der_sig;
    unsigned char* end = der_sig + der_len;
    size_t seq_len;
    ret = mbedtls_asn1_get_tag(
      &p, end, &seq_len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    if (ret != 0)
    {
      throw std::runtime_error("failed to parse ECDSA DER signature");
    }

    Mpi r, s;
    ret = mbedtls_asn1_get_mpi(&p, end, &r.val);
    if (ret != 0)
    {
      throw std::runtime_error("failed to parse ECDSA DER r");
    }
    ret = mbedtls_asn1_get_mpi(&p, end, &s.val);
    if (ret != 0)
    {
      throw std::runtime_error("failed to parse ECDSA DER s");
    }

    size_t comp_size = ecdsa_component_size(algo);
    std::vector<unsigned char> raw(comp_size * 2, 0);
    mbedtls_mpi_write_binary(&r.val, raw.data(), comp_size);
    mbedtls_mpi_write_binary(&s.val, raw.data() + comp_size, comp_size);

    return std::string(reinterpret_cast<char*>(raw.data()), raw.size());
  }

  // ── X.509 helpers ──

  std::string get_common_name(const mbedtls_x509_name* name)
  {
    if (!name)
    {
      return {};
    }

    // Walk the name list looking for OID 2.5.4.3 (CommonName)
    const mbedtls_x509_name* cur = name;
    while (cur)
    {
      // OID for CN: 0x55 0x04 0x03
      if (
        cur->oid.len == 3 && cur->oid.p[0] == 0x55 && cur->oid.p[1] == 0x04 &&
        cur->oid.p[2] == 0x03)
      {
        return std::string(
          reinterpret_cast<const char*>(cur->val.p), cur->val.len);
      }
      cur = cur->next;
    }
    return {};
  }

  void extract_sans(
    const mbedtls_x509_crt& crt,
    std::vector<std::string>& dns_names,
    std::vector<std::string>& uri_strings)
  {
    const mbedtls_x509_sequence* cur = &crt.subject_alt_names;
    while (cur)
    {
      if (cur->buf.len == 0)
      {
        cur = cur->next;
        continue;
      }

      // The tag encodes the SAN type:
      //   tag & 0x1F == 2 → dNSName
      //   tag & 0x1F == 6 → uniformResourceIdentifier
      unsigned char tag = cur->buf.tag;
      int san_type = tag & 0x1F;

      if (san_type == 2) // dNSName
      {
        // Need to parse the ASN.1 value from the raw buffer
        // mbedtls stores the raw ASN.1 data; we need to extract the string
        mbedtls_x509_subject_alternative_name san;
        int ret = mbedtls_x509_parse_subject_alt_name(&cur->buf, &san);
        if (ret == 0 && san.type == MBEDTLS_X509_SAN_DNS_NAME)
        {
          dns_names.emplace_back(
            reinterpret_cast<const char*>(san.san.unstructured_name.p),
            san.san.unstructured_name.len);
        }
        mbedtls_x509_free_subject_alt_name(&san);
      }
      else if (san_type == 6) // URI
      {
        mbedtls_x509_subject_alternative_name san;
        int ret = mbedtls_x509_parse_subject_alt_name(&cur->buf, &san);
        if (
          ret == 0 && san.type == MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER)
        {
          uri_strings.emplace_back(
            reinterpret_cast<const char*>(san.san.unstructured_name.p),
            san.san.unstructured_name.len);
        }
        mbedtls_x509_free_subject_alt_name(&san);
      }

      cur = cur->next;
    }
  }

  std::string cert_to_der_b64(const mbedtls_x509_crt& crt)
  {
    std::string_view der(reinterpret_cast<const char*>(crt.raw.p), crt.raw.len);
    return ::base64_encode(der, false);
  }

  using rego::crypto_core::ParsedCertificate;

  ParsedCertificate cert_to_parsed(const mbedtls_x509_crt& crt)
  {
    ParsedCertificate pc;
    pc.subject.common_name = get_common_name(&crt.subject);
    extract_sans(crt, pc.dns_names, pc.uri_strings);
    pc.der_b64 = cert_to_der_b64(crt);
    return pc;
  }

  // MPI to base64url (for JWK export of RSA private keys)
  std::string mpi_to_base64url(const mbedtls_mpi& mpi)
  {
    size_t len = mbedtls_mpi_size(&mpi);
    std::vector<unsigned char> buf(len);
    mbedtls_mpi_write_binary(&mpi, buf.data(), len);
    std::string_view sv(reinterpret_cast<char*>(buf.data()), buf.size());
    return rego::crypto_core::base64url_encode_nopad_impl(sv);
  }
}

// ── Public API implementation ──

namespace rego::crypto_core
{
  std::string md5_hex(std::string_view data)
  {
    return digest_hex(MBEDTLS_MD_MD5, data);
  }

  std::string sha1_hex(std::string_view data)
  {
    return digest_hex(MBEDTLS_MD_SHA1, data);
  }

  std::string sha256_hex(std::string_view data)
  {
    return digest_hex(MBEDTLS_MD_SHA256, data);
  }

  std::string hmac_md5_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(MBEDTLS_MD_MD5, key, data);
  }

  std::string hmac_sha1_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(MBEDTLS_MD_SHA1, key, data);
  }

  std::string hmac_sha256_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(MBEDTLS_MD_SHA256, key, data);
  }

  std::string hmac_sha512_hex(std::string_view key, std::string_view data)
  {
    return hmac_hex(MBEDTLS_MD_SHA512, key, data);
  }

  bool hmac_equal(std::string_view mac1, std::string_view mac2)
  {
    return hmac_equal_impl(mac1, mac2);
  }

  std::string base64url_encode_nopad(std::string_view data)
  {
    return base64url_encode_nopad_impl(data);
  }

  std::string base64url_decode(std::string_view data)
  {
    return base64url_decode_impl(data);
  }

  Algorithm parse_algorithm(std::string_view name)
  {
    return parse_algorithm_impl(name);
  }

  // Single-entry thread-local cache for parsed public keys.
  // Avoids re-parsing the same PEM/JWK key on repeated JWT verifications
  // with the same issuer key.
  struct PkCache
  {
    std::string key_str;
    PkCtx* pk = nullptr;

    ~PkCache()
    {
      delete pk;
    }

    PkCache() = default;
    PkCache(const PkCache&) = delete;
    PkCache& operator=(const PkCache&) = delete;

    // Returns a cached PkCtx if the key matches, otherwise parses
    // the new key, caches it, and returns it. Returns nullptr on failure.
    PkCtx* get(std::string_view key)
    {
      if (pk != nullptr && key_str == key)
      {
        return pk;
      }

      delete pk;
      pk = new PkCtx();
      key_str.assign(key.data(), key.size());

      if (!load_public_key(*pk, key))
      {
        delete pk;
        pk = nullptr;
        key_str.clear();
        return nullptr;
      }

      return pk;
    }
  };

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

    // EdDSA: not supported in mbedtls backend
    if (algo == Algorithm::EdDSA)
    {
      return {false, "EdDSA algorithm is not supported"};
    }

    // Asymmetric (RSA/ECDSA): load public key (cached)
    thread_local PkCache pk_cache;
    PkCtx* pk = pk_cache.get(key_or_cert);
    if (pk == nullptr)
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

    mbedtls_md_type_t md_type = md_type_for_algo(algo);
    bool ok = false;

    switch (algo)
    {
      case Algorithm::RS256:
      case Algorithm::RS384:
      case Algorithm::RS512:
        ok = verify_rsa_pkcs1(md_type, *pk, signing_input, signature_bytes);
        break;

      case Algorithm::PS256:
      case Algorithm::PS384:
      case Algorithm::PS512:
        ok = verify_rsa_pss(md_type, *pk, signing_input, signature_bytes);
        break;

      case Algorithm::ES256:
      case Algorithm::ES384:
      case Algorithm::ES512:
        ok = verify_ecdsa(md_type, *pk, signing_input, signature_bytes);
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
    auto ast = parse_json(key_or_cert);
    if (!ast)
    {
      return verify_signature(
        algo, signing_input, signature_bytes, key_or_cert);
    }

    auto keys = extract_jwks_keys(ast);
    if (keys.size() <= 1)
    {
      return verify_signature(
        algo, signing_input, signature_bytes, key_or_cert);
    }

    for (auto& key_ast : keys)
    {
      // EdDSA not supported in mbedtls backend
      if (algo == Algorithm::EdDSA)
      {
        return {false, "EdDSA algorithm is not supported"};
      }

      PkCtx pk;
      if (!pk_from_jwk_ast(pk, key_ast))
      {
        continue;
      }

      mbedtls_md_type_t md_type = md_type_for_algo(algo);
      bool ok = false;

      switch (algo)
      {
        case Algorithm::RS256:
        case Algorithm::RS384:
        case Algorithm::RS512:
          ok = verify_rsa_pkcs1(md_type, pk, signing_input, signature_bytes);
          break;
        case Algorithm::PS256:
        case Algorithm::PS384:
        case Algorithm::PS512:
          ok = verify_rsa_pss(md_type, pk, signing_input, signature_bytes);
          break;
        case Algorithm::ES256:
        case Algorithm::ES384:
        case Algorithm::ES512:
          ok = verify_ecdsa(md_type, pk, signing_input, signature_bytes);
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

    // HMAC: extract the "k" field
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

    if (algo == Algorithm::EdDSA)
    {
      throw std::runtime_error("EdDSA algorithm is not supported");
    }

    PkCtx pk;
    crypto_core::SecureString ed25519_raw;
    if (!load_private_key_ast(pk, ed25519_raw.value, ast))
    {
      throw std::runtime_error("failed to load private key from JWK");
    }

    mbedtls_md_type_t md_type = md_type_for_algo(algo);

    switch (algo)
    {
      case Algorithm::RS256:
      case Algorithm::RS384:
      case Algorithm::RS512:
        return sign_rsa(md_type, pk, signing_input, false);

      case Algorithm::PS256:
      case Algorithm::PS384:
      case Algorithm::PS512:
        return sign_rsa(md_type, pk, signing_input, true);

      case Algorithm::ES256:
      case Algorithm::ES384:
      case Algorithm::ES512:
        return sign_ecdsa(md_type, pk, signing_input, algo);

      default:
        throw std::runtime_error("unsupported algorithm for signing");
    }
  }

  // ── X.509 Certificate Parsing ──

  // Parse concatenated DER certificates into an X509Crt chain.
  // mbedtls_x509_crt_parse_der only parses one cert, so we must loop.
  int parse_der_chain(X509Crt& chain, const unsigned char* data, size_t len)
  {
    const unsigned char* p = data;
    const unsigned char* end = data + len;
    int total_ret = 0;
    bool any_ok = false;

    while (p < end)
    {
      // Each DER cert starts with SEQUENCE tag (0x30) followed by length
      if (*p != 0x30)
      {
        break;
      }

      // Peek at the length to determine cert size
      const unsigned char* lp = p + 1;
      size_t cert_len = 0;
      if (lp >= end)
      {
        break;
      }

      if (*lp < 0x80)
      {
        cert_len = *lp;
        cert_len += 2; // tag + 1-byte length
      }
      else
      {
        size_t num_bytes = *lp & 0x7F;
        if (num_bytes == 0 || num_bytes > 4 || lp + num_bytes >= end)
        {
          break;
        }
        for (size_t i = 0; i < num_bytes; ++i)
        {
          if (cert_len > (SIZE_MAX >> 8))
          {
            return any_ok ? 0 : -1; // overflow guard
          }
          cert_len = (cert_len << 8) | lp[1 + i];
        }
        cert_len += 2 + num_bytes; // tag + length-of-length + length bytes
      }

      if (p + cert_len > end)
      {
        break;
      }

      int ret = mbedtls_x509_crt_parse_der(&chain.crt, p, cert_len);
      if (ret == 0)
      {
        any_ok = true;
      }
      else
      {
        total_ret = ret;
      }
      p += cert_len;
    }

    return any_ok ? 0 : total_ret;
  }

  ParseCertsResult parse_certificates(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {{}, decoded.error};
    }

    X509Crt chain;
    int ret;
    if (decoded.is_pem)
    {
      // mbedtls requires null-terminated PEM
      std::string pem_str(decoded.data);
      pem_str.push_back('\0');
      ret = mbedtls_x509_crt_parse(
        &chain.crt,
        reinterpret_cast<const unsigned char*>(pem_str.data()),
        pem_str.size());
    }
    else
    {
      ret = parse_der_chain(
        chain,
        reinterpret_cast<const unsigned char*>(decoded.data.data()),
        decoded.data.size());
    }

    if (ret != 0 && chain.crt.raw.len == 0)
    {
      return {{}, "x509: malformed certificate"};
    }

    ParseCertsResult result;
    const mbedtls_x509_crt* cur = &chain.crt;
    while (cur && cur->raw.len > 0)
    {
      result.certs.push_back(cert_to_parsed(*cur));
      cur = cur->next;
    }

    if (result.certs.empty())
    {
      return {{}, "x509: malformed certificate"};
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

    X509Csr csr;
    int ret;
    if (decoded.is_pem)
    {
      std::string pem_str(decoded.data);
      pem_str.push_back('\0');
      ret = mbedtls_x509_csr_parse(
        &csr.csr,
        reinterpret_cast<const unsigned char*>(pem_str.data()),
        pem_str.size());
    }
    else
    {
      ret = mbedtls_x509_csr_parse_der(
        &csr.csr,
        reinterpret_cast<const unsigned char*>(decoded.data.data()),
        decoded.data.size());
    }

    if (ret != 0)
    {
      return {{}, "asn1: structure error"};
    }

    ParseCSRResult result;
    result.subject.common_name = get_common_name(&csr.csr.subject);
    return result;
  }

  VerifyCertsResult parse_and_verify_certificates(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {false, {}, decoded.error};
    }

    X509Crt chain;
    int ret;
    if (decoded.is_pem)
    {
      std::string pem_str(decoded.data);
      pem_str.push_back('\0');
      ret = mbedtls_x509_crt_parse(
        &chain.crt,
        reinterpret_cast<const unsigned char*>(pem_str.data()),
        pem_str.size());
    }
    else
    {
      ret = parse_der_chain(
        chain,
        reinterpret_cast<const unsigned char*>(decoded.data.data()),
        decoded.data.size());
    }

    // Collect all parsed certs and their DER data
    struct CertInfo
    {
      ParsedCertificate parsed;
      const unsigned char* raw_p;
      size_t raw_len;
      bool self_signed;
    };
    std::vector<CertInfo> all_certs;
    const mbedtls_x509_crt* cur = &chain.crt;
    while (cur && cur->raw.len > 0)
    {
      bool ss =
        (cur->issuer_raw.len == cur->subject_raw.len &&
         memcmp(cur->issuer_raw.p, cur->subject_raw.p, cur->issuer_raw.len) ==
           0);
      all_certs.push_back({cert_to_parsed(*cur), cur->raw.p, cur->raw.len, ss});
      cur = cur->next;
    }

    if (all_certs.size() < 2)
    {
      VerifyCertsResult result;
      result.valid = false;
      for (auto& ci : all_certs)
      {
        result.certs.push_back(ci.parsed);
      }
      return result;
    }

    // OPA convention: last cert is the leaf. All others are CA or
    // intermediate. Build a leaf chain (leaf + intermediates) and a separate
    // CA chain (self-signed roots) for mbedtls_x509_crt_verify.
    X509Crt leaf_chain;
    X509Crt ca_chain;

    // Parse leaf (last cert) first into the leaf chain
    size_t leaf_idx = all_certs.size() - 1;
    mbedtls_x509_crt_parse_der(
      &leaf_chain.crt, all_certs[leaf_idx].raw_p, all_certs[leaf_idx].raw_len);

    // All certs except the leaf: self-signed → CA chain, otherwise → append
    // to leaf chain as intermediates
    for (size_t i = 0; i < leaf_idx; ++i)
    {
      if (all_certs[i].self_signed)
      {
        mbedtls_x509_crt_parse_der(
          &ca_chain.crt, all_certs[i].raw_p, all_certs[i].raw_len);
      }
      else
      {
        mbedtls_x509_crt_parse_der(
          &leaf_chain.crt, all_certs[i].raw_p, all_certs[i].raw_len);
      }
    }

    uint32_t flags = 0;
    // NOTE: CRL and OCSP revocation checking is not performed (matching OPA).
    // A revoked certificate will be accepted if otherwise valid.
    ret = mbedtls_x509_crt_verify(
      &leaf_chain.crt,
      &ca_chain.crt,
      nullptr, // no CRL
      nullptr, // no CN to check
      &flags,
      nullptr, // no custom verification callback
      nullptr);

    VerifyCertsResult result;
    result.valid = (ret == 0);

    if (result.valid)
    {
      // Return leaf-first order (matching OPA):
      // [leaf, intermediates..., root]
      result.certs.push_back(all_certs[leaf_idx].parsed);
      for (size_t i = 0; i < leaf_idx; ++i)
      {
        if (!all_certs[i].self_signed)
        {
          result.certs.push_back(all_certs[i].parsed);
        }
      }
      for (size_t i = 0; i < leaf_idx; ++i)
      {
        if (all_certs[i].self_signed)
        {
          result.certs.push_back(all_certs[i].parsed);
        }
      }
    }
    else
    {
      for (auto& ci : all_certs)
      {
        result.certs.push_back(ci.parsed);
      }
    }

    return result;
  }

  ParseRSAKeyResult parse_rsa_private_key(std::string_view input)
  {
    DecodedInput decoded = decode_cert_input(input);
    if (!decoded.error.empty())
    {
      return {{}, decoded.error};
    }

    PkCtx pk;
    int ret;

    auto& rng = get_rng();
    if (decoded.is_pem)
    {
      std::string pem_str(decoded.data);
      pem_str.push_back('\0');
      ret = mbedtls_pk_parse_key(
        &pk.ctx,
        reinterpret_cast<const unsigned char*>(pem_str.data()),
        pem_str.size(),
        nullptr,
        0,
        mbedtls_ctr_drbg_random,
        &rng.drbg.ctx);
    }
    else
    {
      ret = mbedtls_pk_parse_key(
        &pk.ctx,
        reinterpret_cast<const unsigned char*>(decoded.data.data()),
        decoded.data.size(),
        nullptr,
        0,
        mbedtls_ctr_drbg_random,
        &rng.drbg.ctx);
    }

    if (ret != 0 || !mbedtls_pk_can_do(&pk.ctx, MBEDTLS_PK_RSA))
    {
      return {{}, "failed to parse RSA private key"};
    }

    const mbedtls_rsa_context* rsa = mbedtls_pk_rsa(pk.ctx);

    RSAPrivateKeyJWK jwk;
    jwk.kty = "RSA";

    // Export RSA components. mbedtls_rsa_export gives us N, P, Q, D, E
    Mpi n, p, q, d, e;
    mbedtls_rsa_export(rsa, &n.val, &p.val, &q.val, &d.val, &e.val);

    jwk.n = mpi_to_base64url(n.val);
    jwk.e = mpi_to_base64url(e.val);
    jwk.d = mpi_to_base64url(d.val);
    jwk.p = mpi_to_base64url(p.val);
    jwk.q = mpi_to_base64url(q.val);

    // Compute dp, dq, qi from d, p, q
    Mpi dp, dq, qi, one, pm1, qm1;
    mbedtls_mpi_lset(&one.val, 1);

    // dp = d mod (p-1)
    mbedtls_mpi_sub_mpi(&pm1.val, &p.val, &one.val);
    mbedtls_mpi_mod_mpi(&dp.val, &d.val, &pm1.val);

    // dq = d mod (q-1)
    mbedtls_mpi_sub_mpi(&qm1.val, &q.val, &one.val);
    mbedtls_mpi_mod_mpi(&dq.val, &d.val, &qm1.val);

    // qi = q^(-1) mod p
    mbedtls_mpi_inv_mod(&qi.val, &q.val, &p.val);

    jwk.dp = mpi_to_base64url(dp.val);
    jwk.dq = mpi_to_base64url(dq.val);
    jwk.qi = mpi_to_base64url(qi.val);

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
      return {{}, false, {}};
    }

    if (!decoded.is_pem)
    {
      return {{}, false, {}};
    }

    ParsePrivateKeysResult result;
    result.is_empty_input = false;

    // Extract all PEM private key blocks
    auto rsa_blocks = extract_pem_der_blocks(decoded.data, "RSA PRIVATE KEY");
    auto pkcs8_blocks = extract_pem_der_blocks(decoded.data, "PRIVATE KEY");

    auto& rng = get_rng();

    auto try_parse_rsa = [&](const std::string& der) {
      PkCtx pk;
      int ret = mbedtls_pk_parse_key(
        &pk.ctx,
        reinterpret_cast<const unsigned char*>(der.data()),
        der.size(),
        nullptr,
        0,
        mbedtls_ctr_drbg_random,
        &rng.drbg.ctx);

      if (ret != 0 || !mbedtls_pk_can_do(&pk.ctx, MBEDTLS_PK_RSA))
      {
        return;
      }

      const mbedtls_rsa_context* rsa = mbedtls_pk_rsa(pk.ctx);
      Mpi n, p, q, d, e;
      mbedtls_rsa_export(rsa, &n.val, &p.val, &q.val, &d.val, &e.val);

      RSAPrivateKeyJWK jwk;
      jwk.kty = "RSA";
      jwk.n = mpi_to_base64url(n.val);
      jwk.e = mpi_to_base64url(e.val);
      jwk.d = mpi_to_base64url(d.val);
      jwk.p = mpi_to_base64url(p.val);
      jwk.q = mpi_to_base64url(q.val);

      Mpi dp, dq, qi, one, pm1, qm1;
      mbedtls_mpi_lset(&one.val, 1);
      mbedtls_mpi_sub_mpi(&pm1.val, &p.val, &one.val);
      mbedtls_mpi_mod_mpi(&dp.val, &d.val, &pm1.val);
      mbedtls_mpi_sub_mpi(&qm1.val, &q.val, &one.val);
      mbedtls_mpi_mod_mpi(&dq.val, &d.val, &qm1.val);
      mbedtls_mpi_inv_mod(&qi.val, &q.val, &p.val);

      jwk.dp = mpi_to_base64url(dp.val);
      jwk.dq = mpi_to_base64url(dq.val);
      jwk.qi = mpi_to_base64url(qi.val);

      result.keys.push_back(jwk);
    };

    for (auto& der : rsa_blocks)
    {
      try_parse_rsa(der);
    }
    for (auto& der : pkcs8_blocks)
    {
      try_parse_rsa(der);
    }

    return result;
  }
}

#endif // REGOCPP_CRYPTO_MBEDTLS
