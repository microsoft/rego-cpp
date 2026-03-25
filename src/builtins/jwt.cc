#include "builtins.hh"

#ifdef REGOCPP_HAS_CRYPTO
#include "crypto_core.hh"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <trieste/json.h>
#endif

namespace
{
  using namespace rego;
  using namespace trieste;
  namespace bi = builtins;

  const char* Message = "JSON Web Tokens are not supported";

#ifdef REGOCPP_HAS_CRYPTO

  // ── JWT token structure helpers ──

  struct JWTParts
  {
    std::string_view header_b64;
    std::string_view payload_b64;
    std::string_view sig_b64;
    std::string_view signing_input; // "header.payload"
  };

  // Split a JWT token into its three base64url sections.
  // Returns false if the token doesn't have exactly 3 sections.
  bool split_jwt(std::string_view token, JWTParts& parts)
  {
    auto dot1 = token.find('.');
    if (dot1 == std::string_view::npos)
    {
      return false;
    }
    auto dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string_view::npos)
    {
      return false;
    }
    if (token.find('.', dot2 + 1) != std::string_view::npos)
    {
      return false;
    }

    parts.header_b64 = token.substr(0, dot1);
    parts.payload_b64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    parts.sig_b64 = token.substr(dot2 + 1);
    parts.signing_input = token.substr(0, dot2);
    return true;
  }

  std::size_t count_dots(std::string_view s)
  {
    std::size_t count = 0;
    for (char c : s)
    {
      if (c == '.')
        ++count;
    }
    return count;
  }

  // Validate a base64url string. Returns the 0-based byte position of the
  // first invalid character, or -1 if the input is valid.
  int validate_base64url(std::string_view s)
  {
    for (int i = 0; i < static_cast<int>(s.size()); ++i)
    {
      char c = s[i];
      bool valid = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '=';
      if (!valid)
      {
        return i;
      }
    }
    return -1;
  }

  // Parse a JSON string into a JSON AST (json:: namespace types).
  // Returns nullptr on parse failure.
  Node parse_json(const std::string& json_str)
  {
    auto result = ::json::reader().synthetic(json_str).read();
    if (!result.ok)
    {
      return nullptr;
    }
    return result.ast->front();
  }

  // Parse a JSON string into a Rego Term node (rego:: namespace types).
  Node parse_json_to_term(const std::string& json_str)
  {
    auto result = ::json::reader().synthetic(json_str).wf_check_enabled(true) >>
      json_to_rego(true);
    if (!result.ok)
    {
      return nullptr;
    }
    return result.ast->front();
  }

  // Convert an existing json:: AST node to a Rego Term without re-parsing.
  Node json_ast_to_term(const Node& json_node)
  {
    auto result =
      json_to_rego(true).wf_check_enabled(true).rewrite(Top << json_node);
    if (!result.ok)
    {
      return nullptr;
    }
    return result.ast->front();
  }

  // Convert raw bytes to hex string
  std::string bytes_to_hex(std::string_view data)
  {
    static constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(data.size() * 2);
    for (unsigned char c : data)
    {
      result.push_back(hex[c >> 4]);
      result.push_back(hex[c & 0x0f]);
    }
    return result;
  }

  // Check if an "aud" array node contains a specific audience string
  bool aud_array_contains(const Node& aud_node, std::string_view target)
  {
    for (auto& elem : *aud_node)
    {
      auto s = ::json::get_string(elem);
      if (s.has_value() && s->view() == target)
      {
        return true;
      }
    }
    return false;
  }

  // Maximum nesting depth for nested JWT (cty: "JWT") to prevent stack
  // overflow. Matches OPA's limit.
  constexpr int MaxJWTNesting = 10;

  // ── io.jwt.decode implementation ──

  Node decode_impl(const Nodes& args, int depth = 0)
  {
    Node jwt_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("io.jwt.decode"));
    if (jwt_node->type() == Error)
    {
      return jwt_node;
    }

    if (depth >= MaxJWTNesting)
    {
      return err(jwt_node, "nested JWT depth exceeded", EvalBuiltInError);
    }

    std::string jwt_str = ::json::unescape(get_string(jwt_node));

    // Check for no period separators
    if (jwt_str.find('.') == std::string::npos)
    {
      return err(
        jwt_node, "encoded JWT had no period separators", EvalBuiltInError);
    }

    // Validate section count
    std::size_t dots = count_dots(jwt_str);
    if (dots != 2)
    {
      std::ostringstream msg;
      msg << "encoded JWT must have 3 sections, found " << (dots + 1);
      return err(jwt_node, msg.str(), EvalBuiltInError);
    }

    JWTParts parts;
    split_jwt(jwt_str, parts);

    // Validate base64url encoding of each section
    int bad_byte = validate_base64url(parts.header_b64);
    if (bad_byte >= 0)
    {
      std::ostringstream msg;
      msg << "JWT header had invalid encoding: illegal base64 data at input "
             "byte "
          << bad_byte;
      return err(jwt_node, msg.str(), EvalBuiltInError);
    }
    bad_byte = validate_base64url(parts.payload_b64);
    if (bad_byte >= 0)
    {
      std::ostringstream msg;
      msg << "JWT payload had invalid encoding: illegal base64 data at input "
             "byte "
          << bad_byte;
      return err(jwt_node, msg.str(), EvalBuiltInError);
    }
    bad_byte = validate_base64url(parts.sig_b64);
    if (bad_byte >= 0)
    {
      std::ostringstream msg;
      msg << "JWT signature had invalid encoding: illegal base64 data at "
             "input byte "
          << bad_byte;
      return err(jwt_node, msg.str(), EvalBuiltInError);
    }

    // Decode and parse header
    std::string header_json = crypto_core::base64url_decode(parts.header_b64);
    Node header_ast = parse_json(header_json);
    if (!header_ast)
    {
      return err(jwt_node, "failed to decode JWT header", EvalBuiltInError);
    }

    // Check for JWE (encrypted JWT)
    Node enc_node = ::json::select(header_ast, {"/enc"});
    if (enc_node->type() != Error)
    {
      return err(
        jwt_node,
        "JWT is a JWE object, which is not supported",
        EvalBuiltInError);
    }

    // Decode payload — handle nested JWT (cty: "JWT")
    std::string payload_decoded =
      crypto_core::base64url_decode(parts.payload_b64);

    auto cty = ::json::select_string(header_ast, {"/cty"});
    if (cty.has_value() && cty->view() == "JWT")
    {
      // The payload is an inner JWT token string. Recursively decode.
      std::string inner = payload_decoded;
      if (inner.size() >= 2 && inner.front() == '"' && inner.back() == '"')
      {
        inner = inner.substr(1, inner.size() - 2);
      }
      Node inner_token = JSONString ^ inner;
      Nodes inner_args = {inner_token};
      return decode_impl(inner_args, depth + 1);
    }

    // Convert already-parsed JSON ASTs to Rego terms for the return value
    Node header = json_ast_to_term(header_ast);
    if (!header)
    {
      return err(jwt_node, "failed to decode JWT header", EvalBuiltInError);
    }

    Node payload_ast2 = parse_json(payload_decoded);
    if (!payload_ast2)
    {
      return err(jwt_node, "failed to decode JWT payload", EvalBuiltInError);
    }
    Node payload = json_ast_to_term(payload_ast2);
    if (!payload)
    {
      return err(jwt_node, "failed to decode JWT payload", EvalBuiltInError);
    }

    // Decode signature to hex
    std::string sig_raw = crypto_core::base64url_decode(parts.sig_b64);
    std::string sig_hex = bytes_to_hex(sig_raw);

    return array({header, payload, rego::string(sig_hex)});
  }

  // ── io.jwt.verify_* implementation ──

  crypto_core::Algorithm algo_from_verify_name(std::string_view name)
  {
    if (name == "verify_hs256")
      return crypto_core::Algorithm::HS256;
    if (name == "verify_hs384")
      return crypto_core::Algorithm::HS384;
    if (name == "verify_hs512")
      return crypto_core::Algorithm::HS512;
    if (name == "verify_rs256")
      return crypto_core::Algorithm::RS256;
    if (name == "verify_rs384")
      return crypto_core::Algorithm::RS384;
    if (name == "verify_rs512")
      return crypto_core::Algorithm::RS512;
    if (name == "verify_ps256")
      return crypto_core::Algorithm::PS256;
    if (name == "verify_ps384")
      return crypto_core::Algorithm::PS384;
    if (name == "verify_ps512")
      return crypto_core::Algorithm::PS512;
    if (name == "verify_es256")
      return crypto_core::Algorithm::ES256;
    if (name == "verify_es384")
      return crypto_core::Algorithm::ES384;
    if (name == "verify_es512")
      return crypto_core::Algorithm::ES512;
    if (name == "verify_eddsa")
      return crypto_core::Algorithm::EdDSA;
    throw std::invalid_argument("unknown verify function");
  }

  Node verify_impl(std::string_view func_suffix, const Nodes& args)
  {
    std::string func_name = "io.jwt." + std::string(func_suffix);

    Node jwt_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func(func_name));
    if (jwt_node->type() == Error)
    {
      return jwt_node;
    }
    Node key_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func(func_name));
    if (key_node->type() == Error)
    {
      return key_node;
    }

    std::string jwt_str = ::json::unescape(get_string(jwt_node));
    std::string key_str = ::json::unescape(get_string(key_node));
    if (jwt_str.find('.') == std::string::npos)
    {
      return err(
        jwt_node, "encoded JWT had no period separators", EvalBuiltInError);
    }

    std::size_t dots = count_dots(jwt_str);
    if (dots != 2)
    {
      std::ostringstream msg;
      msg << "encoded JWT must have 3 sections, found " << (dots + 1);
      return err(jwt_node, msg.str(), EvalBuiltInError);
    }

    JWTParts parts;
    split_jwt(jwt_str, parts);

    crypto_core::Algorithm expected_algo = algo_from_verify_name(func_suffix);

    // Parse header to check algorithm
    std::string header_json = crypto_core::base64url_decode(parts.header_b64);
    Node header_ast = parse_json(header_json);
    if (!header_ast)
    {
      return boolean(false);
    }

    crypto_core::Algorithm token_algo;
    try
    {
      auto alg = ::json::select_string(header_ast, {"/alg"});
      if (!alg.has_value())
      {
        return boolean(false);
      }
      token_algo = crypto_core::parse_algorithm(alg->view());
    }
    catch (...)
    {
      return boolean(false);
    }

    std::string sig_raw = crypto_core::base64url_decode(parts.sig_b64);

    // Always perform signature verification even on algorithm mismatch
    // to avoid leaking the expected algorithm through response timing.
    auto result = crypto_core::verify_signature(
      expected_algo, parts.signing_input, sig_raw, key_str);

    if (!result.error.empty())
    {
      return err(jwt_node, result.error, EvalBuiltInError);
    }

    return boolean(result.valid && token_algo == expected_algo);
  }

  // ── Claim validation helpers ──

  // Check a simple string claim (iss, sub). Returns true if passes.
  bool check_string_claim(
    const Node& payload_ast,
    const std::string& json_path,
    const std::optional<std::string>& constraint)
  {
    if (!constraint.has_value())
    {
      return true;
    }
    auto value = ::json::select_string(payload_ast, {json_path});
    return value.has_value() && value->view() == constraint.value();
  }

  // Check the "aud" claim. Returns true if passes.
  bool check_aud_claim(
    const Node& payload_ast, const std::optional<std::string>& constraint)
  {
    Node payload_aud = ::json::select(payload_ast, {"/aud"});
    bool payload_has_aud = payload_aud->type() != Error;

    if (constraint.has_value())
    {
      if (!payload_has_aud)
      {
        return false;
      }
      if (payload_aud->type() == ::json::Array)
      {
        return aud_array_contains(payload_aud, constraint.value());
      }
      auto aud_str = ::json::get_string(payload_aud);
      return aud_str.has_value() && aud_str->view() == constraint.value();
    }

    // Token has aud but constraints don't specify aud — failure
    return !payload_has_aud;
  }

  // ── io.jwt.decode_verify implementation ──

  // Find a string value for a key in a Rego Object node (constraints)
  std::optional<std::string> get_object_string(
    const Node& obj, const std::string& key)
  {
    for (auto& item : *obj)
    {
      if (item->type() != ObjectItem)
        continue;
      Node k = item->front();
      auto key_str = try_get_string(k);
      if (key_str.has_value() && key_str.value() == key)
      {
        Node v = item->back();
        auto val_str = try_get_string(v);
        if (val_str.has_value())
        {
          return val_str;
        }
      }
    }
    return std::nullopt;
  }

  // Find a numeric value for a key in a Rego Object node (constraints)
  std::optional<double> get_object_number(
    const Node& obj, const std::string& key)
  {
    for (auto& item : *obj)
    {
      if (item->type() != ObjectItem)
        continue;
      Node k = item->front();
      auto key_str = try_get_string(k);
      if (key_str.has_value() && key_str.value() == key)
      {
        Node v = item->back();
        auto val = try_get_double(v);
        if (val.has_value())
        {
          return val;
        }
      }
    }
    return std::nullopt;
  }

  Node decode_verify_impl(const Nodes& args, int depth = 0)
  {
    Node jwt_node = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("io.jwt.decode_verify"));
    if (jwt_node->type() == Error)
    {
      return jwt_node;
    }

    if (depth >= MaxJWTNesting)
    {
      return err(jwt_node, "nested JWT depth exceeded", EvalBuiltInError);
    }

    Node constraints =
      unwrap_arg(args, UnwrapOpt(1).type(Object).func("io.jwt.decode_verify"));
    if (constraints->type() == Error)
    {
      return constraints;
    }

    auto make_failure = []() {
      return array({boolean(false), object({}), object({})});
    };

    std::string jwt_str = ::json::unescape(get_string(jwt_node));

    if (jwt_str.find('.') == std::string::npos)
    {
      return err(
        jwt_node, "encoded JWT had no period separators", EvalBuiltInError);
    }

    std::size_t dots = count_dots(jwt_str);
    if (dots != 2)
    {
      std::ostringstream msg;
      msg << "encoded JWT must have 3 sections, found " << (dots + 1);
      return err(jwt_node, msg.str(), EvalBuiltInError);
    }

    JWTParts parts;
    split_jwt(jwt_str, parts);

    // Decode and parse header into JSON AST
    std::string header_json = crypto_core::base64url_decode(parts.header_b64);
    Node header_ast = parse_json(header_json);
    if (!header_ast)
    {
      return err(jwt_node, "failed to decode JWT header", EvalBuiltInError);
    }

    // Check for JWE
    Node enc_node = ::json::select(header_ast, {"/enc"});
    if (enc_node->type() != Error)
    {
      return err(
        jwt_node,
        "JWT is a JWE object, which is not supported",
        EvalBuiltInError);
    }

    // Check for critical extensions — we don't support any
    Node crit_node = ::json::select(header_ast, {"/crit"});
    if (crit_node->type() != Error)
    {
      return make_failure();
    }

    // Extract the algorithm from the header
    crypto_core::Algorithm token_algo;
    std::string alg_str;
    try
    {
      auto alg = ::json::select_string(header_ast, {"/alg"});
      if (!alg.has_value())
      {
        return make_failure();
      }
      alg_str = std::string(alg->view());
      token_algo = crypto_core::parse_algorithm(alg_str);
    }
    catch (...)
    {
      return make_failure();
    }

    // Check "alg" constraint — must match header algorithm
    auto alg_constraint = get_object_string(constraints, "alg");
    if (alg_constraint.has_value() && alg_constraint.value() != alg_str)
    {
      return make_failure();
    }

    // Get the verification key from constraints
    auto cert_str = get_object_string(constraints, "cert");
    auto secret_str = get_object_string(constraints, "secret");

    // Signature verification
    std::string sig_raw = crypto_core::base64url_decode(parts.sig_b64);
    crypto_core::VerifyResult vresult;

    if (cert_str.has_value())
    {
      std::string key = ::json::unescape(cert_str.value());
      vresult = crypto_core::verify_signature_any_key(
        token_algo, parts.signing_input, sig_raw, key);
    }
    else if (secret_str.has_value())
    {
      std::string key = ::json::unescape(secret_str.value());
      vresult = crypto_core::verify_signature(
        token_algo, parts.signing_input, sig_raw, key);
    }
    else
    {
      return make_failure();
    }

    if (!vresult.error.empty())
    {
      return err(jwt_node, vresult.error, EvalBuiltInError);
    }

    if (!vresult.valid)
    {
      return make_failure();
    }

    // Handle nested JWT (cty: "JWT")
    auto cty = ::json::select_string(header_ast, {"/cty"});
    if (cty.has_value() && cty->view() == "JWT")
    {
      std::string payload_decoded =
        crypto_core::base64url_decode(parts.payload_b64);
      // Strip surrounding quotes if present
      if (
        payload_decoded.size() >= 2 && payload_decoded.front() == '"' &&
        payload_decoded.back() == '"')
      {
        payload_decoded = payload_decoded.substr(1, payload_decoded.size() - 2);
      }
      Node inner_token = JSONString ^ payload_decoded;
      Nodes inner_args = {inner_token, args[1]};
      return decode_verify_impl(inner_args, depth + 1);
    }

    // Decode and parse payload into JSON AST for claim validation
    std::string payload_json = crypto_core::base64url_decode(parts.payload_b64);
    Node payload_ast = parse_json(payload_json);
    if (!payload_ast)
    {
      return err(jwt_node, "failed to decode JWT payload", EvalBuiltInError);
    }

    // ── Claim validation ──

    // Determine the "current time" in seconds.
    // Constraints "time" is in nanoseconds; otherwise use wall clock.
    double now_seconds;
    auto time_constraint = get_object_number(constraints, "time");
    if (time_constraint.has_value())
    {
      now_seconds = time_constraint.value() / 1e9;
    }
    else
    {
      auto now = std::chrono::system_clock::now();
      now_seconds =
        std::chrono::duration<double>(now.time_since_epoch()).count();
    }

    // Check "iss" and "sub" string claims
    if (
      !check_string_claim(
        payload_ast, "/iss", get_object_string(constraints, "iss")) ||
      !check_string_claim(
        payload_ast, "/sub", get_object_string(constraints, "sub")))
    {
      return make_failure();
    }

    // Check "aud" claim
    if (!check_aud_claim(payload_ast, get_object_string(constraints, "aud")))
    {
      return make_failure();
    }

    // Check "exp" — payload exp must be in the future
    Node exp_node = ::json::select(payload_ast, {"/exp"});
    if (exp_node->type() != Error)
    {
      auto exp_val = ::json::get_number(exp_node);
      if (!exp_val.has_value())
      {
        return err(jwt_node, "exp value must be a number", EvalBuiltInError);
      }
      if (exp_val.value() <= now_seconds)
      {
        return make_failure();
      }
    }

    // Check "nbf" — payload nbf must be in the past
    Node nbf_node = ::json::select(payload_ast, {"/nbf"});
    if (nbf_node->type() != Error)
    {
      auto nbf_val = ::json::get_number(nbf_node);
      if (!nbf_val.has_value())
      {
        return err(jwt_node, "nbf value must be a number", EvalBuiltInError);
      }
      if (nbf_val.value() > now_seconds)
      {
        return make_failure();
      }
    }

    // All checks passed — convert already-parsed JSON ASTs to Rego terms
    Node header = json_ast_to_term(header_ast);
    if (!header)
    {
      return err(jwt_node, "failed to decode JWT header", EvalBuiltInError);
    }

    Node payload = json_ast_to_term(payload_ast);
    if (!payload)
    {
      return err(jwt_node, "failed to decode JWT payload", EvalBuiltInError);
    }

    return array({boolean(true), header, payload});
  }

  // ── Shared encode/sign helpers ──

  // Serialize a Rego Object node to a compact JSON string with sorted keys.
  std::string rego_node_to_json(const Node& node)
  {
    Node term = Resolver::to_term(node);
    if (term->type() == Error)
    {
      return {};
    }
    auto result = rego_to_json().wf_check_enabled(true).rewrite(Top << term);
    if (!result.ok)
    {
      return {};
    }
    return json::to_string(result.ast);
  }

  // Determine if the JWT "typ" implies the payload should be valid JSON.
  // OPA treats "JWT" or empty/missing typ as requiring JSON payload.
  bool is_jwt_typ(std::string_view typ)
  {
    return typ.empty() || typ == "JWT";
  }

  // ── io.jwt.encode_sign_raw implementation ──

  Node encode_sign_raw_impl(const Nodes& args)
  {
    Node header_node = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("io.jwt.encode_sign_raw"));
    if (header_node->type() == Error)
      return header_node;

    Node payload_node = unwrap_arg(
      args, UnwrapOpt(1).type(JSONString).func("io.jwt.encode_sign_raw"));
    if (payload_node->type() == Error)
      return payload_node;

    Node key_node = unwrap_arg(
      args, UnwrapOpt(2).type(JSONString).func("io.jwt.encode_sign_raw"));
    if (key_node->type() == Error)
      return key_node;

    std::string header_str = ::json::unescape(get_string(header_node));
    std::string payload_str = ::json::unescape(get_string(payload_node));
    std::string key_str = ::json::unescape(get_string(key_node));

    // Parse header to extract and validate "alg"
    Node header_ast = parse_json(header_str);
    if (!header_ast)
    {
      if (header_str.empty())
      {
        return err(
          header_node,
          "missing or invalid 'alg' header: cannot parse JSON: "
          "cannot parse empty string",
          EvalBuiltInError);
      }
      // If the input looks like it intended to be a JSON object, report parse
      // error. Otherwise treat it as a value with no "alg" field.
      if (header_str.find('{') != std::string::npos)
      {
        return err(
          header_node,
          "missing or invalid 'alg' header: cannot parse JSON",
          EvalBuiltInError);
      }
      return err(
        header_node,
        "missing or invalid 'alg' header: jwsbb: header \"alg\" not found",
        EvalBuiltInError);
    }

    auto alg_opt = ::json::select_string(header_ast, {"/alg"});
    if (!alg_opt.has_value())
    {
      return err(
        header_node,
        "missing or invalid 'alg' header: jwsbb: header \"alg\" not found",
        EvalBuiltInError);
    }

    std::string_view alg_str = alg_opt->view();
    crypto_core::Algorithm algo;
    try
    {
      algo = crypto_core::parse_algorithm(alg_str);
    }
    catch (const std::invalid_argument&)
    {
      return err(
        header_node,
        "unknown JWS algorithm: " + std::string(alg_str),
        EvalBuiltInError);
    }

    // Check if typ implies JWT; if so, payload must be valid JSON
    auto typ_opt = ::json::select_string(header_ast, {"/typ"});
    std::string_view typ = typ_opt.has_value() ? typ_opt->view() : "";
    if (is_jwt_typ(typ))
    {
      if (payload_str.empty())
      {
        return err(
          payload_node,
          "type is JWT but payload is not JSON",
          EvalBuiltInError);
      }
      Node payload_check = parse_json(payload_str);
      if (!payload_check)
      {
        return err(
          payload_node,
          "type is JWT but payload is not JSON",
          EvalBuiltInError);
      }
    }

    // Base64url-encode header and payload as raw strings
    std::string header_b64 = crypto_core::base64url_encode_nopad(header_str);
    std::string payload_b64 = crypto_core::base64url_encode_nopad(payload_str);

    // Build the signing input
    std::string signing_input = header_b64 + "." + payload_b64;

    // Sign
    std::string sig_bytes;
    try
    {
      sig_bytes = crypto_core::sign(algo, signing_input, key_str);
    }
    catch (const std::exception& e)
    {
      return err(header_node, e.what(), EvalBuiltInError);
    }

    std::string sig_b64 = crypto_core::base64url_encode_nopad(sig_bytes);
    std::string token = signing_input + "." + sig_b64;
    return JSONString ^ token;
  }

  // ── io.jwt.encode_sign implementation ──

  Node encode_sign_impl(const Nodes& args)
  {
    Node header_node =
      unwrap_arg(args, UnwrapOpt(0).type(Object).func("io.jwt.encode_sign"));
    if (header_node->type() == Error)
      return header_node;

    Node payload_node = unwrap_arg(
      args, UnwrapOpt(1).types({Object, Set}).func("io.jwt.encode_sign"));
    if (payload_node->type() == Error)
      return payload_node;

    Node key_node =
      unwrap_arg(args, UnwrapOpt(2).type(Object).func("io.jwt.encode_sign"));
    if (key_node->type() == Error)
      return key_node;

    // Serialize the Rego objects to compact JSON with sorted keys
    std::string header_json = rego_node_to_json(header_node);
    std::string payload_json = rego_node_to_json(payload_node);
    std::string key_json = rego_node_to_json(key_node);

    if (header_json.empty() || key_json.empty())
    {
      return err(
        header_node, "failed to serialize arguments", EvalBuiltInError);
    }

    // Delegate to the raw implementation using the serialized JSON strings
    Node h = JSONString ^ header_json;
    Node p = JSONString ^ payload_json;
    Node k = JSONString ^ key_json;
    Nodes raw_args = {h, p, k};
    return encode_sign_raw_impl(raw_args);
  }

#endif // REGOCPP_HAS_CRYPTO

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
#ifndef REGOCPP_HAS_CRYPTO
    return BuiltInDef::placeholder({"io.jwt.decode"}, decode_decl, Message);
#else
    return BuiltInDef::create(
      {"io.jwt.decode"}, decode_decl, [](const Nodes& args) {
        return decode_impl(args);
      });
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
    return BuiltInDef::placeholder(
      {"io.jwt.decode_verify"}, decode_verify_decl, Message);
#else
    return BuiltInDef::create(
      {"io.jwt.decode_verify"}, decode_verify_decl, [](const Nodes& args) {
        return decode_verify_impl(args);
      });
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
    return BuiltInDef::placeholder(
      {"io.jwt.encode_sign"}, encode_sign_decl, Message);
#else
    return BuiltInDef::create(
      {"io.jwt.encode_sign"}, encode_sign_decl, encode_sign_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
    return BuiltInDef::placeholder(
      {"io.jwt.encode_sign_raw"}, encode_sign_raw_decl, Message);
#else
    return BuiltInDef::create(
      {"io.jwt.encode_sign_raw"}, encode_sign_raw_decl, encode_sign_raw_impl);
#endif
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
#ifndef REGOCPP_HAS_CRYPTO
        return BuiltInDef::placeholder(name, verify_decl, Message);
#else
        return BuiltInDef::create(
          name, verify_decl, [view = std::string(view)](const Nodes& args) {
            return verify_impl(view, args);
          });
#endif
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