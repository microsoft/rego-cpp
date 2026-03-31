// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Platform-independent crypto utility functions shared between backends.

#pragma once

#ifdef REGOCPP_HAS_CRYPTO

#include "base64/base64.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <trieste/json.h>
#include <vector>

namespace rego::crypto_core
{
  // Maximum base64url-encoded size (in bytes) accepted for a single JWK key
  // component (n, e, d, p, q, dp, dq, qi, x, y). This bounds memory
  // allocation during key parsing to prevent resource exhaustion from
  // adversarial JWK inputs. 16 KB of base64url decodes to ~12 KB, which is
  // far beyond any practical RSA modulus (8192-bit = 1024 bytes).
  constexpr size_t MaxJWKComponentB64Len = 16384;

  // Per-algorithm JWK component size bounds (base64url-encoded bytes).
  // These are tighter than MaxJWKComponentB64Len for algorithms where the
  // key component sizes are well-known.
  constexpr size_t MaxRSAComponentB64Len = 2048; // RSA-8192: ~1366 bytes
  constexpr size_t MaxECComponentB64Len = 128; // P-521: ~88 bytes
  constexpr size_t MaxOKPComponentB64Len = 128; // Ed25519: ~44 bytes

  // Maximum number of certificates allowed in a parsed chain. Bounds
  // memory allocation and loop iterations during PEM/DER cert parsing
  // and after chain verification in each backend.
  constexpr size_t MaxCertChainLen = 256;

  // Maximum decoded size (in bytes) of a single PEM block. Prevents
  // unbounded memory allocation from adversarial base64 payloads.
  constexpr size_t MaxPEMBlockSize = 10 * 1024 * 1024; // 10 MB

  // ── Hex encoding ──

  inline std::string to_hex(const unsigned char* data, size_t len)
  {
    static const char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i)
    {
      result += hex_chars[(data[i] >> 4) & 0x0F];
      result += hex_chars[data[i] & 0x0F];
    }
    return result;
  }

  // ── Constant-time HMAC comparison ──

  inline bool hmac_equal_impl(std::string_view mac1, std::string_view mac2)
  {
    if (mac1.size() != mac2.size())
    {
      return false;
    }

    volatile unsigned char result = 0;
    for (size_t i = 0; i < mac1.size(); ++i)
    {
      result |= static_cast<unsigned char>(mac1[i]) ^
        static_cast<unsigned char>(mac2[i]);
    }
    return result == 0;
  }

  // ── PEM validation ──

  // Validates that a PEM string contains a well-formed certificate or public
  // key block. Returns an empty string on success, or an error message on
  // failure. Shared across all crypto backends.
  inline std::string validate_pem(std::string_view pem)
  {
    static const std::string_view cert_begin = "-----BEGIN CERTIFICATE-----";
    static const std::string_view cert_end = "-----END CERTIFICATE-----";
    static const std::string_view key_begin = "-----BEGIN PUBLIC KEY-----";
    static const std::string_view key_end = "-----END PUBLIC KEY-----";

    auto cert_pos = pem.find(cert_begin);
    if (cert_pos != std::string_view::npos)
    {
      auto end_pos = pem.find(cert_end, cert_pos);
      if (end_pos == std::string_view::npos)
      {
        return "failed to parse a PEM certificate";
      }
      auto after = end_pos + cert_end.size();
      auto remainder = pem.substr(after);
      while (!remainder.empty() &&
             (remainder.front() == '\n' || remainder.front() == '\r' ||
              remainder.front() == ' ' || remainder.front() == '\t'))
      {
        remainder.remove_prefix(1);
      }
      if (!remainder.empty())
      {
        return "extra data after a PEM certificate block";
      }
      return {};
    }

    if (
      pem.find("-----BEGIN CERT") != std::string_view::npos &&
      pem.find(cert_begin) == std::string_view::npos)
    {
      return "failed to extract a Key from the PEM certificate";
    }

    auto key_pos = pem.find(key_begin);
    if (key_pos != std::string_view::npos)
    {
      auto end_pos = pem.find(key_end, key_pos);
      if (end_pos == std::string_view::npos)
      {
        return "failed to parse a PEM key";
      }
      return {};
    }

    return {};
  }

  // ── Secure memory erasure ──

  // Zeroes the contents of a string before clearing it. Implemented in
  // crypto.cc so that the platform-specific include (windows.h) does not
  // leak into this header.
  void secure_erase(std::string& s);

  // RAII guard that zeroes a std::string on destruction.
  struct SecureString
  {
    std::string value;

    SecureString() = default;
    explicit SecureString(std::string v) : value(std::move(v)) {}
    ~SecureString()
    {
      secure_erase(value);
    }
    SecureString(const SecureString&) = delete;
    SecureString& operator=(const SecureString&) = delete;
    SecureString(SecureString&&) = default;
    SecureString& operator=(SecureString&&) = default;

    const char* data() const
    {
      return value.data();
    }
    size_t size() const
    {
      return value.size();
    }
    bool empty() const
    {
      return value.empty();
    }
  };

  // ── Base64url ──

  inline std::string base64url_encode_nopad_impl(std::string_view data)
  {
    std::string encoded = ::base64_encode(data, true);
    while (!encoded.empty() && encoded.back() == '=')
    {
      encoded.pop_back();
    }
    return encoded;
  }

  inline std::string base64url_decode_impl(std::string_view data)
  {
    return ::base64_decode(data);
  }

  // ── Algorithm parsing ──

  inline Algorithm parse_algorithm_impl(std::string_view name)
  {
    if (name == "HS256")
      return Algorithm::HS256;
    if (name == "HS384")
      return Algorithm::HS384;
    if (name == "HS512")
      return Algorithm::HS512;
    if (name == "RS256")
      return Algorithm::RS256;
    if (name == "RS384")
      return Algorithm::RS384;
    if (name == "RS512")
      return Algorithm::RS512;
    if (name == "PS256")
      return Algorithm::PS256;
    if (name == "PS384")
      return Algorithm::PS384;
    if (name == "PS512")
      return Algorithm::PS512;
    if (name == "ES256")
      return Algorithm::ES256;
    if (name == "ES384")
      return Algorithm::ES384;
    if (name == "ES512")
      return Algorithm::ES512;
    if (name == "EdDSA")
      return Algorithm::EdDSA;
    throw std::invalid_argument(
      std::string("unknown JWT algorithm: ") + std::string(name));
  }

  // ── PEM / DER input decoding ──

  // Decoded certificate input — result of decode_cert_input.
  struct DecodedInput
  {
    std::string data;
    bool is_pem;
    std::string error;
  };

  // Decode input that may be: PEM string, base64-encoded PEM, or
  // base64-encoded DER. Returns the decoded bytes and format.
  inline DecodedInput decode_cert_input(std::string_view input)
  {
    // Direct PEM string
    if (input.find("-----BEGIN") != std::string_view::npos)
    {
      return {std::string(input), true, {}};
    }

    // Count non-whitespace chars and reject if not a multiple of 4,
    // since ::base64_decode is lenient about length but OPA is not.
    size_t non_ws_count = 0;
    for (char c : input)
    {
      if (c != '\n' && c != '\r' && c != ' ' && c != '\t')
      {
        non_ws_count++;
      }
    }

    if (non_ws_count == 0 || non_ws_count % 4 != 0)
    {
      return {{}, false, "illegal base64"};
    }

    // Try base64 decode — pos_of_char throws on invalid characters
    try
    {
      std::string decoded = ::base64_decode(input);
      if (decoded.empty())
      {
        return {{}, false, "illegal base64"};
      }
      // Check if the decoded data is PEM
      if (decoded.find("-----BEGIN") != std::string::npos)
      {
        return {std::move(decoded), true, {}};
      }
      // Otherwise it's raw DER
      return {std::move(decoded), false, {}};
    }
    catch (const std::exception&)
    {
      return {{}, false, "illegal base64"};
    }
  }

  // Extract DER bytes from PEM blocks manually, like Go's pem.Decode.
  inline std::vector<std::string> extract_pem_der_blocks(
    std::string_view pem_data, std::string_view block_type)
  {
    std::vector<std::string> blocks;
    std::string begin_marker =
      std::string("-----BEGIN ") + std::string(block_type) + "-----";
    std::string end_marker =
      std::string("-----END ") + std::string(block_type) + "-----";

    size_t pos = 0;
    while (pos < pem_data.size())
    {
      if (blocks.size() >= MaxCertChainLen)
      {
        break;
      }

      size_t begin = pem_data.find(begin_marker, pos);
      if (begin == std::string_view::npos)
      {
        break;
      }
      size_t content_start = pem_data.find('\n', begin);
      if (content_start == std::string_view::npos)
      {
        break;
      }
      content_start++;

      size_t end = pem_data.find(end_marker, content_start);
      if (end == std::string_view::npos)
      {
        break;
      }

      std::string b64;
      for (size_t i = content_start; i < end; ++i)
      {
        char c = pem_data[i];
        if (c != '\n' && c != '\r' && c != ' ' && c != '\t')
        {
          b64 += c;
        }
      }

      try
      {
        // Check encoded size before decoding to avoid transient large
        // allocations from adversarial PEM blocks. Base64 expands by ~4/3,
        // so encoded size * 3/4 approximates decoded size.
        if (b64.size() > MaxPEMBlockSize * 4 / 3)
        {
          pos = end + end_marker.size();
          continue;
        }
        auto decoded = ::base64_decode(b64);
        if (decoded.size() > MaxPEMBlockSize)
        {
          // Skip oversized PEM blocks to prevent resource exhaustion
          pos = end + end_marker.size();
          continue;
        }
        blocks.push_back(std::move(decoded));
      }
      catch (const std::exception&)
      {
        // Skip malformed PEM blocks
      }

      pos = end + end_marker.size();
    }
    return blocks;
  }

  // ── JSON helpers (shared across all crypto backends) ──

  inline trieste::Node parse_json(std::string_view json_str)
  {
    // Quick check: if the string doesn't start with '{' or '[' (after
    // whitespace), it cannot be valid JSON. Skip the full parse to avoid
    // generating hundreds of spurious error log messages when probing
    // non-JSON inputs like PEM certificates.
    auto pos = json_str.find_first_not_of(" \t\n\r");
    if (
      pos == std::string_view::npos ||
      (json_str[pos] != '{' && json_str[pos] != '['))
    {
      return nullptr;
    }

    std::string raw(json_str);
    auto result = trieste::json::reader().synthetic(raw).read();
    if (!result.ok)
    {
      return nullptr;
    }
    return result.ast->front();
  }

  inline std::string_view json_select_string(
    const trieste::Node& ast, const char* pointer)
  {
    if (!ast)
    {
      return {};
    }
    auto val = trieste::json::select_string(ast, {pointer});
    if (!val.has_value())
    {
      return {};
    }
    return val->view();
  }

  inline std::vector<trieste::Node> extract_jwks_keys(const trieste::Node& ast)
  {
    std::vector<trieste::Node> result;
    if (!ast)
    {
      return result;
    }
    auto keys_node = trieste::json::select(ast, {"/keys"});
    if (
      keys_node->type() == trieste::Error ||
      keys_node->type() != trieste::json::Array)
    {
      return result;
    }
    for (auto& elem : *keys_node)
    {
      if (elem->type() == trieste::json::Object)
      {
        result.push_back(elem);
      }
    }
    return result;
  }
}

#endif // REGOCPP_HAS_CRYPTO
