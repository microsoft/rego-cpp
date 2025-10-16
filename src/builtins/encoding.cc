#include "base64/base64.h"
#include "builtins.h"
#include "rego.hh"
#include "trieste/json.h"
#include "trieste/wf.h"
#include "trieste/yaml.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node base64_encode_(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
    std::string encoded = ::base64_encode(x_string);
    return JSONString ^ encoded;
  }

  const Node base64_encode_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "string to encode")
                             << (bi::Type << bi::String)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^ "base64 serialization of `x`")
                            << (bi::Type << bi::String));

  Node base64_decode_(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
    std::string decoded = ::base64_decode(x_string);
    return JSONString ^ decoded;
  }

  const Node base64_decode_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "string to decode")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "base64 deserialization of `x`")
                 << (bi::Type << bi::String));

  Node base64_is_valid(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return False ^ "false";
    }

    std::string x_string = get_string(x_string_node);
    for (auto c : x_string)
    {
      if (!(std::isalnum(c) || c == '+' || c == '/' || c == '='))
        return False ^ "false";
    }

    return True ^ "true";
  }

  const Node base64_is_valid_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg << (bi::Name ^ "x") << (bi::Description ^ "string to check")
                    << (bi::Type << bi::String)))
    << (bi::Result
        << (bi::Name ^ "y")
        << (bi::Description ^
            "`true` if `x` is valid base64 encoded value, `false` otherwise")
        << (bi::Type << bi::Boolean));

  Node base64url_encode(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
    std::string encoded = ::base64_encode(x_string, true);
    return JSONString ^ encoded;
  }

  const Node base64url_encode_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "string to encode")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "base64url serialization of `x`")
                 << (bi::Type << bi::String));

  Node base64url_encode_no_pad(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
    std::string encoded = ::base64_encode(x_string, true);
    while (encoded.back() == '=')
    {
      encoded.pop_back();
    }
    return JSONString ^ encoded;
  }

  const Node base64url_encode_no_pad_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "string to encode")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "base64url serialization of `x`")
                 << (bi::Type << bi::String));

  Node base64url_decode(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
    std::string decoded = ::base64_decode(x_string);
    return JSONString ^ decoded;
  }

  const Node base64url_decode_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "string to decode")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "base64url deserialization of `x`")
                 << (bi::Type << bi::String));

  Node hex_encode(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
    std::ostringstream encoded;
    for (auto c : x_string)
    {
      encoded << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return JSONString ^ encoded.str();
  }

  const Node hex_encode_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "string to encode")
                             << (bi::Type << bi::String)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^
                                "serialization of `x` using hex-encoding")
                            << (bi::Type << bi::String));

  std::optional<int> hexdigit(char hex)
  {
    if (hex >= '0' && hex <= '9')
    {
      return (hex - '0');
    }

    if (hex >= 'A' && hex <= 'F')
    {
      return (hex - 'A' + 10);
    }

    if (hex >= 'a' && hex <= 'f')
    {
      return (hex - 'a' + 10);
    }

    return std::nullopt;
  }

  Node hex_decode(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
    std::ostringstream decoded;
    for (std::size_t i = 0; i < x_string.size(); i += 2)
    {
      int test = 0;
      for (std::size_t j = i; j < i + 2; ++j)
      {
        char hex = x_string[j];
        auto maybe_digit = hexdigit(hex);
        if (maybe_digit.has_value())
        {
          test = (test << 4) | maybe_digit.value();
          continue;
        }

        std::ostringstream error;
        error << "invalid byte: U+" << std::hex << std::setw(4)
              << std::setfill('0') << (int)hex << " '" << hex << "'";
        return err(x_string_node, error.str(), EvalBuiltInError);
      }

      decoded << (char)test;
    }
    return JSONString ^ decoded.str();
  }

  const Node hex_decode_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "a hex-encoded string")
                             << (bi::Type << bi::String)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^ "deserialized from `x`")
                            << (bi::Type << bi::String));

  Node json_marshal(const Nodes& args)
  {
    Node x = Resolver::to_term(args[0]);
    if (x->type() == Error)
    {
      return x;
    }

    auto result = rego_to_json().wf_check_enabled(true).rewrite(Top << x);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(args[0], "failed to marshal JSON", EvalBuiltInError);
    }

    std::string json = json::to_string(result.ast);
    return JSONString ^ add_quotes(json::escape(json));
  }

  const Node json_marshal_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the term to serialize")
                             << (bi::Type << bi::Any)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "the JSON string representation of `x`")
                 << (bi::Type << bi::String));

  Node json_marshal_with_options(const Nodes& args)
  {
    Node x = Resolver::to_term(args[0]);
    if (x->type() == Error)
    {
      return x;
    }

    Node opts = unwrap_arg(args, UnwrapOpt(1).type(Object));
    if (opts == Error)
    {
      return opts;
    }

    std::optional<bool> maybe_pretty = std::nullopt;
    std::string indent = "\t";
    std::string prefix = "";

    for (auto& item : *opts)
    {
      auto maybe_key = unwrap(item / Key, JSONString);
      if (!maybe_key.success)
      {
        return err(item, "key must be a string", EvalBuiltInError);
      }

      std::string key = get_string(maybe_key.node);
      if (key == "pretty")
      {
        auto maybe_bool = unwrap(item / Val, {True, False});
        if (!maybe_bool.success)
        {
          return err(item, "value must be a boolean", EvalBuiltInError);
        }
        maybe_pretty = get_bool(maybe_bool.node);
      }
      else if (key == "indent")
      {
        indent = get_string(item / Val);
        if (!maybe_pretty.has_value())
        {
          maybe_pretty = true;
        }
      }
      else if (key == "prefix")
      {
        prefix = get_string(item / Val);
        if (!maybe_pretty.has_value())
        {
          maybe_pretty = true;
        }
      }
      else
      {
        std::string message = "object contained unknown key \"" + key + '"';
        return err(item, message, EvalTypeError);
      }
    }

    bool pretty = maybe_pretty.value_or(false);

    auto result = rego_to_json().wf_check_enabled(true).rewrite(Top << x);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(args[0], "failed to marshal JSON", EvalBuiltInError);
    }

    std::string json = json::to_string(result.ast, pretty, false, indent);

    if (pretty && !prefix.empty())
    {
      std::ostringstream prefixed;
      prefixed << prefix;
      size_t start = 0;
      size_t end = json.find('\n');
      while (end != std::string::npos)
      {
        prefixed << json.substr(start, end - start) << '\n' << prefix;
        start = end + 1;
        end = json.find('\n', start);
      }
      prefixed << json.substr(start);
      json = prefixed.str();
    }

    return JSONString ^ add_quotes(json::escape(json));
  }

  const Node json_marshal_with_options_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the term to serialize")
                             << (bi::Type << bi::Any))
                 << (bi::Arg
                     << (bi::Name ^ "opts")
                     << (bi::Description ^ "encoding options")
                     << (bi::Type
                         << (bi::HybridObject
                             << (bi::DynamicObject << (bi::Type << bi::String)
                                                   << (bi::Type << bi::Any))
                             << (bi::StaticObject
                                 << (bi::ObjectItem << (bi::Name ^ "indent")
                                                    << (bi::Type << bi::String))
                                 << (bi::ObjectItem << (bi::Name ^ "prefix")
                                                    << (bi::Type << bi::String))
                                 << (bi::ObjectItem
                                     << (bi::Name ^ "pretty")
                                     << (bi::Type << bi::Boolean)))))))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^
                     "the JSON string representation of `x`, with configured "
                     "prefix/indent string(s) as appropriate")
                 << (bi::Type << bi::String));

  Node json_unmarshal(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_json = get_string(x);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result = json::reader().synthetic(x_raw).wf_check_enabled(true) >>
      json_to_rego(true);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(x, "failed to parse JSON", EvalBuiltInError);
    }

    return result.ast->front();
  }

  const Node json_unmarshal_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "a JSON string")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "the term deserialized from `x`")
                 << (bi::Type << bi::Any));

  Node json_is_valid(const Nodes& args)
  {
    auto maybe_x = unwrap(args[0], JSONString);
    if (!maybe_x.success)
    {
      return False ^ "false";
    }

    std::string x_json = get_string(maybe_x.node);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result = json::reader().synthetic(x_raw).wf_check_enabled(true).read();
    return result.ok ? True ^ "true" : False ^ "false";
  }

  const Node json_is_valid_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "a JSON string")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^
                     "`true` if `x` is valid JSON, `false` otherwise")
                 << (bi::Type << bi::Boolean));

  Node yaml_marshal(const Nodes& args)
  {
    Node term = args[0];
    if (term != Term)
    {
      if (term->in({Int, Float, True, False, Null, JSONString}))
      {
        term = Term << (Scalar << term);
      }
      else if (term->in({Array, Object, Set, Scalar}))
      {
        term = Term << term;
      }
      else
      {
        return err(args[0], "failed to marshal YAML", EvalBuiltInError);
      }
    }

    auto result = rego_to_yaml().wf_check_enabled(true).rewrite(Top << term);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(args[0], "failed to marshal YAML", EvalBuiltInError);
    }

    std::string yaml = yaml::to_string(result.ast);
    return JSONString ^ add_quotes(json::escape(yaml));
  }

  const Node yaml_marshal_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the term to serialize")
                             << (bi::Type << bi::Any)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "the YAML string representation of `x`")
                 << (bi::Type << bi::String));

  Node yaml_unmarshal(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_json = get_string(x);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result = yaml::reader().synthetic(x_raw).wf_check_enabled(true) >>
      yaml::to_json() >> json_to_rego(true);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(x, "failed to parse YAML", EvalBuiltInError);
    }

    return result.ast->front();
  }

  const Node yaml_unmarshal_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "a YAML string")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "the term deserialized from `x`")
                 << (bi::Type << bi::Any));

  Node yaml_is_valid(const Nodes& args)
  {
    auto maybe_x = unwrap(args[0], JSONString);
    if (!maybe_x.success)
    {
      return False ^ "false";
    }

    std::string x_json = get_string(maybe_x.node);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result = yaml::reader().synthetic(x_raw).wf_check_enabled(true).read();
    return result.ok ? True ^ "true" : False ^ "false";
  }

  const Node yaml_is_valid_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "a YAML string")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^
                     "`true` if `x` is valid YAML, `false` otherwise")
                 << (bi::Type << bi::Boolean));

  void do_urlquery_encode(std::ostringstream& encoded, Node x)
  {
    std::string x_json = get_string(x);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    for (auto c : x_raw)
    {
      if (std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~')
      {
        encoded << c;
      }
      else
      {
        encoded << '%' << std::uppercase << std::hex << std::setw(2)
                << std::setfill('0') << (int)(unsigned char)c;
      }
    }
  }

  Node urlquery_encode(const Nodes& args)
  {
    auto x = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x->type() == Error)
    {
      return x;
    }

    std::ostringstream encoded;
    do_urlquery_encode(encoded, x);

    return JSONString ^ add_quotes(json::escape(encoded.str()));
  }

  const Node urlquery_encode_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the string to encode")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "URL-encoding serialization of `x`")
                 << (bi::Type << bi::String));

  Node urlquery_encode_object(const Nodes& args)
  {
    auto object = unwrap_arg(args, UnwrapOpt(0).type(Object));
    if (object->type() == Error)
    {
      return object;
    }

    std::ostringstream encoded;
    for (std::size_t i = 0; i < object->size(); ++i)
    {
      auto maybe_key = unwrap(object->at(i) / Key, JSONString);
      if (!maybe_key.success)
      {
        return err(maybe_key.node, "key must be a string", EvalBuiltInError);
      }

      std::ostringstream key_buf;
      do_urlquery_encode(key_buf, maybe_key.node);
      std::string key_str = key_buf.str();

      auto maybe_value = unwrap(object->at(i) / Val, {JSONString, Array, Set});
      if (!maybe_value.success)
      {
        return err(
          maybe_value.node,
          "value must be a string or array of strings",
          EvalBuiltInError);
      }

      if (i > 0)
      {
        encoded << '&';
      }

      if (maybe_value.node == JSONString)
      {
        encoded << key_str << '=';
        do_urlquery_encode(encoded, maybe_value.node);
      }
      else if (maybe_value.node == Array || maybe_value.node == Set)
      {
        Node array = maybe_value.node;
        for (std::size_t j = 0; j < array->size(); ++j)
        {
          maybe_value = unwrap(array->at(j), JSONString);
          if (!maybe_value.success)
          {
            return err(
              maybe_value.node, "value must be a string", EvalBuiltInError);
          }

          if (j > 0)
          {
            encoded << '&';
          }
          encoded << key_str << '=';
          do_urlquery_encode(encoded, maybe_value.node);
        }
      }
    }

    return JSONString ^ add_quotes(json::escape(encoded.str()));
  }

  const Node urlquery_encode_object_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "object")
                     << (bi::Description ^ "the string to encode")
                     << (bi::Type
                         << (bi::DynamicObject
                             << (bi::Type << bi::String)
                             << (bi::Type
                                 << (bi::TypeSeq
                                     << (bi::Type << bi::String)
                                     << (bi::Type
                                         << (bi::DynamicArray
                                             << (bi::Type << bi::String)))
                                     << (bi::Type
                                         << (bi::Set
                                             << (bi::Type << bi::String)))))))))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "URL-encoding serialization of `object`")
                 << (bi::Type << bi::String));

  std::optional<std::string> maybe_urlquery_decode(const std::string& encoded)
  {
    std::ostringstream decoded;
    for (auto it = encoded.begin(); it != encoded.end(); ++it)
    {
      if (*it == '%')
      {
        if (std::distance(it, encoded.end()) < 3)
        {
          return nullptr;
        }

        auto hex0 = hexdigit(it[1]);
        auto hex1 = hexdigit(it[2]);
        if (!hex0.has_value() || !hex1.has_value())
        {
          return nullptr;
        }

        int value = (hex0.value() << 4) | hex1.value();
        decoded << static_cast<char>(value);
        it += 2;
      }
      else
      {
        decoded << *it;
      }
    }

    return decoded.str();
  }

  Node urlquery_decode(const Nodes& args)
  {
    auto x = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_json = get_string(x);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto maybe_result = maybe_urlquery_decode(x_raw);
    if (maybe_result.has_value())
    {
      return JSONString ^ add_quotes(json::escape(maybe_result.value()));
    }

    return err(x, "invalid URL query string", EvalBuiltInError);
  }

  const Node urlquery_decode_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the URL-encoded string")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "URL-encoding deserialization of `x`")
                 << (bi::Type << bi::String));

  Node urlquery_decode_object(const Nodes& args)
  {
    auto x = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_json = get_string(x);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    std::map<std::string, Nodes> items;
    size_t start = 0;
    size_t end = x_raw.find('&');
    while (start < x_raw.size())
    {
      std::string item_value = x_raw.substr(start, end - start);
      size_t equal = item_value.find('=');
      auto maybe_key = maybe_urlquery_decode(item_value.substr(0, equal));
      if (!maybe_key.has_value())
      {
        return err(x, "invalid URL query string", EvalBuiltInError);
      }

      std::string key = maybe_key.value();
      if (!items.contains(key))
      {
        items[key] = Nodes();
      }

      if (equal != std::string::npos)
      {
        auto maybe_value = maybe_urlquery_decode(item_value.substr(equal + 1));

        if (!maybe_value.has_value())
        {
          return err(x, "invalid URL query string", EvalBuiltInError);
        }

        items[key].push_back(
          JSONString ^ add_quotes(json::escape(maybe_value.value())));
      }
      else
      {
        items[key].push_back(JSONString ^ "\"\"");
      }

      start = end + 1;
      end = x_raw.find('&', start);
      if (end == std::string::npos)
      {
        end = x_raw.size();
      }
    }

    Node argseq = NodeDef::create(Seq);
    for (auto& [key, value] : items)
    {
      argseq->push_back(JSONString ^ add_quotes(json::escape(key)));
      argseq->push_back(Resolver::array(Seq << value));
    }

    return Resolver::object(argseq);
  }

  const Node urlquery_decode_object_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the URL-encoded string")
                             << (bi::Type << bi::String)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^ "the resulting object")
                            << (bi::Type
                                << (bi::DynamicObject
                                    << (bi::Type << bi::String)
                                    << (bi::Type
                                        << (bi::DynamicArray
                                            << (bi::Type << bi::String))))));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> encoding()
    {
      return {
        BuiltInDef::create(
          Location("base64.encode"), base64_encode_decl, base64_encode_),
        BuiltInDef::create(
          Location("base64.decode"), base64_decode_decl, base64_decode_),
        BuiltInDef::create(
          Location("base64.is_valid"), base64_is_valid_decl, base64_is_valid),
        BuiltInDef::create(
          Location("base64url.encode"),
          base64url_encode_decl,
          base64url_encode),
        BuiltInDef::create(
          Location("base64url.encode_no_pad"),
          base64url_encode_no_pad_decl,
          base64url_encode_no_pad),
        BuiltInDef::create(
          Location("base64url.decode"),
          base64url_decode_decl,
          base64url_decode),
        BuiltInDef::create(Location("hex.decode"), hex_decode_decl, hex_decode),
        BuiltInDef::create(Location("hex.encode"), hex_encode_decl, hex_encode),
        BuiltInDef::create(
          Location("json.marshal"), json_marshal_decl, json_marshal),
        BuiltInDef::create(
          Location("json.marshal_with_options"),
          json_marshal_with_options_decl,
          json_marshal_with_options),
        BuiltInDef::create(
          Location("json.unmarshal"), json_unmarshal_decl, json_unmarshal),
        BuiltInDef::create(
          Location("json.is_valid"), json_is_valid_decl, json_is_valid),
        BuiltInDef::create(
          Location("urlquery.decode"), urlquery_decode_decl, urlquery_decode),
        BuiltInDef::create(
          Location("urlquery.decode_object"),
          urlquery_decode_object_decl,
          urlquery_decode_object),
        BuiltInDef::create(
          Location("urlquery.encode"), urlquery_encode_decl, urlquery_encode),
        BuiltInDef::create(
          Location("urlquery.encode_object"),
          urlquery_encode_object_decl,
          urlquery_encode_object),
        BuiltInDef::create(
          Location("yaml.marshal"), yaml_marshal_decl, yaml_marshal),
        BuiltInDef::create(
          Location("yaml.unmarshal"), yaml_unmarshal_decl, yaml_unmarshal),
        BuiltInDef::create(
          Location("yaml.is_valid"), yaml_is_valid_decl, yaml_is_valid),
      };
    }
  }
}