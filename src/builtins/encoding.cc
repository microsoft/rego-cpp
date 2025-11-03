#include "base64/base64.h"
#include "builtins.hh"
#include "rego.hh"
#include "trieste/json.h"
#include "trieste/wf.h"
#include "trieste/yaml.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  namespace base64
  {
    Node encode(const Nodes& args)
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

    BuiltIn encode_factory()
    {
      const Node encode_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string to encode")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^ "base64 serialization of `x`")
                     << (bi::Type << bi::String));
      return BuiltInDef::create({"base64.encode"}, encode_decl, encode);
    }

    Node decode(const Nodes& args)
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

    BuiltIn decode_factory()
    {
      const Node decode_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string to decode")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^ "base64 deserialization of `x`")
                     << (bi::Type << bi::String));
      return BuiltInDef::create({"base64.decode"}, decode_decl, decode);
    }

    Node is_valid(const Nodes& args)
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

    BuiltIn is_valid_factory()
    {
      const Node base64_is_valid_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string to check")
                                 << (bi::Type << bi::String)))
                 << (bi::Result << (bi::Name ^ "y")
                                << (bi::Description ^
                                    "`true` if `x` is valid base64 encoded "
                                    "value, `false` otherwise")
                                << (bi::Type << bi::Boolean));
      return BuiltInDef::create(
        {"base64.is_valid"}, base64_is_valid_decl, is_valid);
    }
  } // namespace base64

  namespace base64url
  {
    Node encode(const Nodes& args)
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

    BuiltIn encode_factory()
    {
      const Node encode_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string to encode")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^ "base64url serialization of `x`")
                     << (bi::Type << bi::String));
      return BuiltInDef::create({"base64url.encode"}, encode_decl, encode);
    }

    Node encode_no_pad(const Nodes& args)
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

    BuiltIn encode_no_pad_factory()
    {
      const Node encode_no_pad_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string to encode")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^ "base64url serialization of `x`")
                     << (bi::Type << bi::String));
      return BuiltInDef::create(
        {"base64url.encode_no_pad"}, encode_no_pad_decl, encode_no_pad);
    }

    Node decode(const Nodes& args)
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

    BuiltIn decode_factory()
    {
      const Node decode_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string to decode")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^ "base64url deserialization of `x`")
                     << (bi::Type << bi::String));
      return BuiltInDef::create({"base64url.decode"}, decode_decl, decode);
    }
  } // namespace base64url

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

  namespace hex
  {
    Node encode(const Nodes& args)
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

    BuiltIn encode_factory()
    {
      const Node encode_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string to encode")
                                 << (bi::Type << bi::String)))
                 << (bi::Result << (bi::Name ^ "y")
                                << (bi::Description ^
                                    "serialization of `x` using hex-encoding")
                                << (bi::Type << bi::String));
      return BuiltInDef::create({"hex.encode"}, encode_decl, encode);
    }

    Node decode(const Nodes& args)
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
    BuiltIn decode_factory()
    {
      const Node decode_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "a hex-encoded string")
                                 << (bi::Type << bi::String)))
                 << (bi::Result << (bi::Name ^ "y")
                                << (bi::Description ^ "deserialized from `x`")
                                << (bi::Type << bi::String));
      return BuiltInDef::create({"hex.decode"}, decode_decl, decode);
    }
  } // namespace hex

  namespace yaml_
  {
    Node marshal(const Nodes& args)
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

    BuiltIn marshal_factory()
    {
      const Node marshal_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the term to serialize")
                                 << (bi::Type << bi::Any)))
                 << (bi::Result << (bi::Name ^ "y")
                                << (bi::Description ^
                                    "the YAML string representation of `x`")
                                << (bi::Type << bi::String));
      return BuiltInDef::create({"yaml.marshal"}, marshal_decl, marshal);
    }

    Node unmarshal(const Nodes& args)
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

    BuiltIn unmarshal_factory()
    {
      const Node unmarshal_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "a YAML string")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^ "the term deserialized from `x`")
                     << (bi::Type << bi::Any));
      return BuiltInDef::create({"yaml.unmarshal"}, unmarshal_decl, unmarshal);
    }

    Node is_valid(const Nodes& args)
    {
      auto maybe_x = unwrap(args[0], JSONString);
      if (!maybe_x.success)
      {
        return False ^ "false";
      }

      std::string x_json = get_string(maybe_x.node);
      std::string x_raw = json::unescape(strip_quotes(x_json));
      auto result =
        yaml::reader().synthetic(x_raw).wf_check_enabled(true).read();
      return result.ok ? True ^ "true" : False ^ "false";
    }

    BuiltIn is_valid_factory()
    {
      const Node is_valid_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "a YAML string")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^
                         "`true` if `x` is valid YAML, `false` otherwise")
                     << (bi::Type << bi::Boolean));
      return BuiltInDef::create({"yaml.is_valid"}, is_valid_decl, is_valid);
    }
  } // namespace yaml_

  namespace urlquery
  {
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

    Node encode(const Nodes& args)
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

    BuiltIn encode_factory()
    {
      const Node encode_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the string to encode")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^ "URL-encoding serialization of `x`")
                     << (bi::Type << bi::String));
      return BuiltInDef::create({"urlquery.encode"}, encode_decl, encode);
    }

    Node encode_object(const Nodes& args)
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

        auto maybe_value =
          unwrap(object->at(i) / Val, {JSONString, Array, Set});
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

    BuiltIn encode_object_factory()
    {
      const Node encode_object_decl =
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
                                                 << (bi::Type
                                                     << bi::String)))))))))
                 << (bi::Result << (bi::Name ^ "y")
                                << (bi::Description ^
                                    "URL-encoding serialization of `object`")
                                << (bi::Type << bi::String));
      return BuiltInDef::create(
        {"urlquery.encode_object"}, encode_object_decl, encode_object);
    }

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

    Node decode(const Nodes& args)
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

    BuiltIn decode_factory()
    {
      const Node decode_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the URL-encoded string")
                                 << (bi::Type << bi::String)))
                 << (bi::Result << (bi::Name ^ "y")
                                << (bi::Description ^
                                    "URL-encoding deserialization of `x`")
                                << (bi::Type << bi::String));
      return BuiltInDef::create({"urlquery.decode"}, decode_decl, decode);
    }

    Node decode_object(const Nodes& args)
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
          auto maybe_value =
            maybe_urlquery_decode(item_value.substr(equal + 1));

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

    BuiltIn decode_object_factory()
    {
      const Node decode_object_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the URL-encoded string")
                                 << (bi::Type << bi::String)))
                 << (bi::Result
                     << (bi::Name ^ "y")
                     << (bi::Description ^ "the resulting object")
                     << (bi::Type
                         << (bi::DynamicObject
                             << (bi::Type << bi::String)
                             << (bi::Type
                                 << (bi::DynamicArray
                                     << (bi::Type << bi::String))))));
      return BuiltInDef::create(
        {"urlquery.decode_object"}, decode_object_decl, decode_object);
    }
  } // namespace urlquery
} // namespace

namespace rego
{
  namespace builtins
  {
    BuiltIn base64(const Location& name)
    {
      assert(name.view().starts_with("base64."));
      std::string_view view = name.view().substr(7); // skip "base64."
      if (view == "encode")
      {
        return base64::encode_factory();
      }
      if (view == "decode")
      {
        return base64::decode_factory();
      }
      if (view == "is_valid")
      {
        return base64::is_valid_factory();
      }

      return nullptr;
    }

    BuiltIn base64url(const Location& name)
    {
      assert(name.view().starts_with("base64url."));
      std::string_view view = name.view().substr(10); // skip "base64url."
      if (view == "encode")
      {
        return base64url::encode_factory();
      }
      if (view == "encode_no_pad")
      {
        return base64url::encode_no_pad_factory();
      }
      if (view == "decode")
      {
        return base64url::decode_factory();
      }

      return nullptr;
    }

    BuiltIn hex(const Location& name)
    {
      assert(name.view().starts_with("hex."));
      std::string_view view = name.view().substr(4); // skip "hex."
      if (view == "encode")
      {
        return hex::encode_factory();
      }
      if (view == "decode")
      {
        return hex::decode_factory();
      }

      return nullptr;
    }

    BuiltIn yaml(const Location& name)
    {
      assert(name.view().starts_with("yaml."));
      std::string_view view = name.view().substr(5); // skip "yaml."
      if (view == "marshal")
      {
        return yaml_::marshal_factory();
      }
      if (view == "unmarshal")
      {
        return yaml_::unmarshal_factory();
      }
      if (view == "is_valid")
      {
        return yaml_::is_valid_factory();
      }

      return nullptr;
    }

    BuiltIn urlquery(const Location& name)
    {
      assert(name.view().starts_with("urlquery."));
      std::string_view view = name.view().substr(9); // skip "urlquery."
      if (view == "encode")
      {
        return urlquery::encode_factory();
      }
      if (view == "encode_object")
      {
        return urlquery::encode_object_factory();
      }
      if (view == "decode")
      {
        return urlquery::decode_factory();
      }
      if (view == "decode_object")
      {
        return urlquery::decode_object_factory();
      }

      return nullptr;
    }
  }
}