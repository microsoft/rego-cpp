#include "base64/base64.h"
#include "builtins.h"
#include "rego.hh"
#include "trieste/json.h"
#include "trieste/wf.h"
#include "trieste/yaml.h"

namespace
{
  using namespace rego;

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

  Node json_marshal(const Nodes& args)
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
        return err(args[0], "failed to marshal JSON", EvalBuiltInError);
      }
    }

    auto result = to_json().wf_check_enabled(true).rewrite(Top << term);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(args[0], "failed to marshal JSON", EvalBuiltInError);
    }

    std::string json = json::to_string(result.ast);
    return JSONString ^ ('"' + json::escape(json) + '"');
  }

  Node json_unmarshal(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_json = get_string(x);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result =
      json::reader().synthetic(x_raw).wf_check_enabled(true) >> from_json(true);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(x, "failed to parse JSON", EvalBuiltInError);
    }

    return result.ast->front();
  }

  Node json_is_valid(const Nodes& args)
  {
    auto maybe_x = unwrap(args[0], {JSONString});
    if (!maybe_x.success)
    {
      return False ^ "false";
    }

    std::string x_json = get_string(maybe_x.node);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result = json::reader().synthetic(x_raw).wf_check_enabled(true).read();
    return result.ok ? True ^ "true" : False ^ "false";
  }

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

    auto result = to_yaml().wf_check_enabled(true).rewrite(Top << term);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(args[0], "failed to marshal YAML", EvalBuiltInError);
    }

    std::string yaml = yaml::to_string(result.ast);
    return JSONString ^ ('"' + json::escape(yaml) + '"');
  }

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
      yaml::to_json() >> from_json(true);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(x, "failed to parse YAML", EvalBuiltInError);
    }

    return result.ast->front();
  }

  Node yaml_is_valid(const Nodes& args)
  {
    auto maybe_x = unwrap(args[0], {JSONString});
    if (!maybe_x.success)
    {
      return False ^ "false";
    }

    std::string x_json = get_string(maybe_x.node);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result = yaml::reader().synthetic(x_raw).wf_check_enabled(true).read();
    return result.ok ? True ^ "true" : False ^ "false";
  }

  void urlquery_encode(std::ostringstream& encoded, Node x)
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
    urlquery_encode(encoded, x);

    return JSONString ^ ('"' + json::escape(encoded.str()) + '"');
  }

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
      auto maybe_key = unwrap(object->at(i) / Key, {JSONString});
      if (!maybe_key.success)
      {
        return err(maybe_key.node, "key must be a string", EvalBuiltInError);
      }

      std::ostringstream key_buf;
      urlquery_encode(key_buf, maybe_key.node);
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
        urlquery_encode(encoded, maybe_value.node);
      }
      else if (maybe_value.node == Array || maybe_value.node == Set)
      {
        Node array = maybe_value.node;
        for (std::size_t j = 0; j < array->size(); ++j)
        {
          maybe_value = unwrap(array->at(j), {JSONString});
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
          urlquery_encode(encoded, maybe_value.node);
        }
      }
    }

    return JSONString ^ ('"' + json::escape(encoded.str()) + '"');
  }

  std::optional<std::string> urlquery_decode(const std::string& encoded)
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
    auto maybe_result = urlquery_decode(x_raw);
    if (maybe_result.has_value())
    {
      return JSONString ^ ('"' + json::escape(maybe_result.value()) + '"');
    }

    return err(x, "invalid URL query string", EvalBuiltInError);
  }

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
      auto maybe_key = urlquery_decode(item_value.substr(0, equal));
      if (!maybe_key.has_value())
      {
        return err(x, "invalid URL query string", EvalBuiltInError);
      }

      std::string key = maybe_key.value();
      if (!contains(items, key))
      {
        items[key] = Nodes();
      }

      if (equal != std::string::npos)
      {
        auto maybe_value = urlquery_decode(item_value.substr(equal + 1));

        if (!maybe_value.has_value())
        {
          return err(x, "invalid URL query string", EvalBuiltInError);
        }

        items[key].push_back(
          JSONString ^ ('"' + json::escape(maybe_value.value()) + '"'));
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

    Node argseq = NodeDef::create(ArgSeq);
    for (auto& [key, value] : items)
    {
      argseq->push_back(JSONString ^ ('"' + json::escape(key) + '"'));
      argseq->push_back(Resolver::array(ArgSeq << value));
    }

    return Resolver::object(argseq, false);
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> encoding()
    {
      return {
        BuiltInDef::create(Location("base64.encode"), 1, base64_encode_),
        BuiltInDef::create(Location("base64.decode"), 1, base64_decode_),
        BuiltInDef::create(Location("base64.is_valid"), 1, base64_is_valid),
        BuiltInDef::create(Location("base64url.encode"), 1, base64url_encode),
        BuiltInDef::create(
          Location("base64url.encode_no_pad"), 1, base64url_encode_no_pad),
        BuiltInDef::create(Location("base64url.decode"), 1, base64url_decode),
        BuiltInDef::create(Location("hex.decode"), 1, hex_decode),
        BuiltInDef::create(Location("hex.encode"), 1, hex_encode),
        BuiltInDef::create(Location("json.marshal"), 1, json_marshal),
        BuiltInDef::create(Location("json.unmarshal"), 1, json_unmarshal),
        BuiltInDef::create(Location("json.is_valid"), 1, json_is_valid),
        BuiltInDef::create(Location("urlquery.decode"), 1, urlquery_decode),
        BuiltInDef::create(
          Location("urlquery.decode_object"), 1, urlquery_decode_object),
        BuiltInDef::create(Location("urlquery.encode"), 1, urlquery_encode),
        BuiltInDef::create(
          Location("urlquery.encode_object"), 1, urlquery_encode_object),
        BuiltInDef::create(Location("yaml.marshal"), 1, yaml_marshal),
        BuiltInDef::create(Location("yaml.unmarshal"), 1, yaml_unmarshal),
        BuiltInDef::create(Location("yaml.is_valid"), 1, yaml_is_valid),
      };
    }
  }
}