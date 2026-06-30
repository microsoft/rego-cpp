#include "builtins.hh"
#include "rego.hh"
#include "trieste/json.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  // Subset of Go's net/url.URL; parsing mirrors net/url.Parse for OPA parity.
  struct ParsedURI
  {
    std::string scheme;
    std::string hostname; // authority host, IPv6 brackets stripped
    std::string port; // authority port (digits only)
    std::string path; // percent-decoded path
    std::string raw_path; // original encoded path when non-canonical
    std::string raw_query;
    std::string fragment; // percent-decoded fragment
  };

  bool is_hex(char c)
  {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
      (c >= 'A' && c <= 'F');
  }

  int hex_value(char c)
  {
    if (c >= '0' && c <= '9')
    {
      return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
      return c - 'a' + 10;
    }
    return c - 'A' + 10;
  }

  // Percent-decodes %XX; nullopt on a bad escape (matches Go's unescape).
  // Intentional duplicate of encoding.cc's maybe_urlquery_decode.
  std::optional<std::string> unescape(const std::string& s)
  {
    std::string result;
    result.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
    {
      char c = s[i];
      if (c == '%')
      {
        if (i + 2 >= s.size() || !is_hex(s[i + 1]) || !is_hex(s[i + 2]))
        {
          return std::nullopt;
        }
        result.push_back(
          static_cast<char>((hex_value(s[i + 1]) << 4) | hex_value(s[i + 2])));
        i += 2;
      }
      else
      {
        result.push_back(c);
      }
    }
    return result;
  }

  // True if a byte must be %-encoded in a path (Go's shouldEscape/encodePath).
  bool should_escape_path(unsigned char c)
  {
    if (
      (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9'))
    {
      return false;
    }
    switch (c)
    {
      case '-':
      case '_':
      case '.':
      case '~':
      case '$':
      case '&':
      case '+':
      case ',':
      case '/':
      case ':':
      case ';':
      case '=':
      case '@':
        return false;
      default:
        return true;
    }
  }

  // Re-encodes a decoded path using Go's encodePath rules.
  std::string escape_path(const std::string& path)
  {
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(path.size());
    for (char ch : path)
    {
      unsigned char c = static_cast<unsigned char>(ch);
      if (should_escape_path(c))
      {
        result.push_back('%');
        result.push_back(hex[c >> 4]);
        result.push_back(hex[c & 0xf]);
      }
      else
      {
        result.push_back(ch);
      }
    }
    return result;
  }

  // Splits authority "host[:port]"; false if malformed (Go's parseHost).
  bool parse_authority(
    const std::string& authority, std::string& hostname, std::string& port)
  {
    // Groups: 1=IPv6 inner, 2=reg-name/IPv4, 3=port; FullMatch anchors ends.
    static const TRegex host_re(
      R"((?:\[([^\[\]]*)\]|([^\[\]:]*))(?::([0-9]*))?)");

    std::string ipv6, reg_name;
    if (!TRegex::FullMatch(authority, host_re, &ipv6, &reg_name, &port))
    {
      return false; // unclosed IPv6 bracket or non-numeric port
    }

    if (!ipv6.empty())
    {
      // IPv6 literals aren't %-decoded except the RFC 6874 zone after "%25".
      std::size_t zone = ipv6.find("%25");
      if (zone == std::string::npos)
      {
        hostname = ipv6;
        return true;
      }
      auto decoded_zone = unescape(ipv6.substr(zone + 3));
      if (!decoded_zone.has_value())
      {
        return false;
      }
      hostname = ipv6.substr(0, zone) + "%" + decoded_zone.value();
      return true;
    }

    auto decoded = unescape(reg_name);
    if (!decoded.has_value())
    {
      return false;
    }
    hostname = decoded.value();
    return true;
  }

  // Parses a URI per Go's net/url.Parse (RFC 3986); nullopt on error.
  std::optional<ParsedURI> url_parse(const std::string& raw_in)
  {
    // Go's net/url rejects any ASCII control character in the raw URL.
    for (unsigned char c : raw_in)
    {
      if (c < 0x20 || c == 0x7f)
      {
        return std::nullopt;
      }
    }

    static const TRegex uri_re(
      R"((?:([a-zA-Z][a-zA-Z0-9+.-]*):)?(//[^/?#]*)?([^?#]*)(?:\?([^#]*))?(?:#(.*))?)");

    std::string scheme, authority, rest, raw_query, frag;
    if (!TRegex::FullMatch(
          raw_in, uri_re, &scheme, &authority, &rest, &raw_query, &frag))
    {
      return std::nullopt;
    }

    ParsedURI u;
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](char c) {
      return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    u.scheme = scheme;
    u.raw_query = raw_query;

    const bool has_authority = !authority.empty(); // captured as "//..."
    const bool rest_is_rooted = !rest.empty() && rest.front() == '/';

    if (!has_authority && !rest_is_rooted)
    {
      if (!u.scheme.empty())
      {
        // Opaque URI (e.g. "mailto:..."): OPA exposes only the scheme.
        rest.clear();
      }
      else
      {
        // Relative reference: the first path segment cannot contain a colon.
        std::string segment = rest.substr(0, rest.find('/'));
        if (segment.find(':') != std::string::npos)
        {
          return std::nullopt;
        }
      }
    }

    if (has_authority)
    {
      std::string host_part = authority.substr(2); // drop leading "//"
      std::size_t at = host_part.rfind('@');
      if (at != std::string::npos)
      {
        host_part = host_part.substr(at + 1);
      }
      if (!parse_authority(host_part, u.hostname, u.port))
      {
        return std::nullopt;
      }
    }

    // setPath: decode the path and record the raw form when non-canonical.
    auto decoded_path = unescape(rest);
    if (!decoded_path.has_value())
    {
      return std::nullopt;
    }
    u.path = decoded_path.value();
    if (escape_path(u.path) != rest)
    {
      u.raw_path = rest;
    }

    auto decoded_frag = unescape(frag);
    if (!decoded_frag.has_value())
    {
      return std::nullopt;
    }
    u.fragment = decoded_frag.value();

    return u;
  }

  Node parse(const Nodes& args)
  {
    Node x_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("uri.parse"));
    if (x_node->type() == Error)
    {
      return x_node;
    }

    std::string raw = json::unescape(get_string(x_node));
    auto parsed = url_parse(raw);
    if (!parsed.has_value())
    {
      return err(args[0], "uri.parse: invalid URI", EvalBuiltInError);
    }

    const ParsedURI& u = parsed.value();
    Node result = NodeDef::create(Object);
    if (!u.scheme.empty())
    {
      result << object_item(rego::string("scheme"), rego::string(u.scheme));
    }

    if (!u.hostname.empty())
    {
      result << object_item(rego::string("hostname"), rego::string(u.hostname));
    }
    if (!u.port.empty())
    {
      result << object_item(rego::string("port"), rego::string(u.port));
    }
    if (!u.path.empty())
    {
      result << object_item(rego::string("path"), rego::string(u.path));
      std::string raw_path = u.raw_path.empty() ? u.path : u.raw_path;
      result << object_item(rego::string("raw_path"), rego::string(raw_path));
    }
    if (!u.raw_query.empty())
    {
      result << object_item(
        rego::string("raw_query"), rego::string(u.raw_query));
    }
    if (!u.fragment.empty())
    {
      result << object_item(rego::string("fragment"), rego::string(u.fragment));
    }

    return result;
  }

  BuiltIn parse_factory()
  {
    const Node parse_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "uri")
                               << (bi::Description ^ "the URI string to parse")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^ "object containing URI components")
                   << (bi::Type
                       << (bi::DynamicObject << (bi::Type << bi::String)
                                             << (bi::Type << bi::String))));
    return BuiltInDef::create({"uri.parse"}, parse_decl, parse);
  }

  Node is_valid(const Nodes& args)
  {
    Node x_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("uri.is_valid"));
    if (x_node->type() == Error)
    {
      return Resolver::scalar(false);
    }

    std::string raw = json::unescape(get_string(x_node));
    if (raw.empty())
    {
      return Resolver::scalar(false);
    }

    return Resolver::scalar(url_parse(raw).has_value());
  }

  BuiltIn is_valid_factory()
  {
    const Node is_valid_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "uri")
                       << (bi::Description ^ "the URI string to validate")
                       << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "result")
                   << (bi::Description ^
                       "true if `uri` is a valid URI, false otherwise")
                   << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"uri.is_valid"}, is_valid_decl, is_valid);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn uri(const Location& name)
    {
      assert(name.view().starts_with("uri."));
      std::string_view view = name.view().substr(4); // skip "uri."
      if (view == "parse")
      {
        return parse_factory();
      }
      if (view == "is_valid")
      {
        return is_valid_factory();
      }

      return nullptr;
    }
  }
}
