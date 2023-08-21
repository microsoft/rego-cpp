#include "errors.h"
#include "register.h"
#include "resolver.h"
#include "utf8.h"

#include <bitset>

namespace
{
  using namespace rego;

  Node unwrap_strings(const Node& collection, std::vector<std::string>& items)
  {
    for (auto term : *collection)
    {
      auto maybe_item = Resolver::maybe_unwrap_string(term);
      if (!maybe_item.has_value())
      {
        return term;
      }
      items.push_back(Resolver::get_string(*maybe_item));
    }

    return {};
  }

  Node concat(const Nodes& args)
  {
    Node delimiter = Resolver::unwrap(
      args[0], JSONString, "concat: operand 1 ", EvalTypeError);
    if (delimiter->type() == Error)
    {
      return delimiter;
    }

    auto maybe_collection = Resolver::maybe_unwrap(args[1], {Array, Set});
    if (!maybe_collection.has_value())
    {
      return Undefined;
    }

    Node collection = *maybe_collection;

    auto delim_str = Resolver::get_string(delimiter);
    std::vector<std::string> items;
    Node bad_term = unwrap_strings(collection, items);
    if (bad_term)
    {
      return err(
        bad_term,
        "concat: operand 2 must be array of strings but got array containing " +
          Resolver::type_name(bad_term),
        EvalTypeError);
    }

    std::ostringstream result_str;
    std::string sep = "";
    for (auto& item : items)
    {
      result_str << sep << item;
      sep = delim_str;
    }

    return Resolver::scalar(result_str.str());
  }

  Node startswith(const Nodes& args)
  {
    auto maybe_search = Resolver::maybe_unwrap_string(args[0]);
    auto maybe_base = Resolver::maybe_unwrap_string(args[1]);
    if (!maybe_search.has_value() || !maybe_base.has_value())
    {
      return err(
        args[0], "startswith: expected string arguments", EvalTypeError);
    }

    std::string search = Resolver::get_string(*maybe_search);
    std::string base = Resolver::get_string(*maybe_base);
    return Resolver::scalar(search.starts_with(base));
  }

  Node endswith(const Nodes& args)
  {
    auto maybe_search = Resolver::maybe_unwrap_string(args[0]);
    auto maybe_base = Resolver::maybe_unwrap_string(args[1]);
    if (!maybe_search.has_value() || !maybe_base.has_value())
    {
      return err(args[0], "endswith: expected string arguments", EvalTypeError);
    }

    std::string search = Resolver::get_string(*maybe_search);
    std::string base = Resolver::get_string(*maybe_base);
    return Resolver::scalar(search.ends_with(base));
  }

  Node contains(const Nodes& args)
  {
    Node haystack = Resolver::unwrap(
      args[0], JSONString, "contains: operand 1 ", EvalTypeError);
    if (haystack->type() == Error)
    {
      return haystack;
    }

    Node needle = Resolver::unwrap(
      args[1], JSONString, "contains: operand 2 ", EvalTypeError);
    if (needle->type() == Error)
    {
      return needle;
    }

    std::string haystack_str = Resolver::get_string(haystack);
    std::string needle_str = Resolver::get_string(needle);
    return Resolver::scalar(haystack_str.find(needle_str) != std::string::npos);
  }

  Node format_int(const Nodes& args)
  {
    auto maybe_number = Resolver::maybe_unwrap_number(args[0]);
    if (!maybe_number.has_value())
    {
      return err(
        args[0], "format_int: expected number argument", EvalTypeError);
    }

    auto base = Resolver::unwrap(
      args[1], JSONInt, "format_int: operand 2 ", EvalTypeError);
    if (base->type() == Error)
    {
      return base;
    }

    Node number = *maybe_number;
    std::int64_t value;
    if (number->type() == JSONFloat)
    {
      value =
        static_cast<std::int64_t>(std::floor(Resolver::get_double(number)));
    }
    else
    {
      value = Resolver::get_int(number).to_int();
    }

    std::ostringstream result;
    std::size_t base_size = Resolver::get_int(base).to_size();
    switch (base_size)
    {
      case 2:
        result << std::bitset<2>(value);
        break;
      case 8:
        result << std::oct << value;
        break;
      case 10:
        result << value;
        break;
      case 16:
        result << std::hex << value;
        break;
      default:
        return err(
          args[1], "operand 2 must be one of {2, 8, 10, 16}", EvalTypeError);
    }

    return Resolver::scalar(result.str());
  }

  Node indexof(const Nodes& args)
  {
    Node haystack = Resolver::unwrap(
      args[0], JSONString, "indexof: operand 1 ", EvalTypeError);
    if (haystack->type() == Error)
    {
      return haystack;
    }

    Node needle = Resolver::unwrap(
      args[1], JSONString, "indexof: operand 2 ", EvalTypeError);
    if (needle->type() == Error)
    {
      return needle;
    }

    runestring haystack_runes =
      utf8_to_runestring(Resolver::get_string(haystack));
    runestring needle_runes = utf8_to_runestring(Resolver::get_string(needle));
    auto pos = haystack_runes.find(needle_runes);
    if (pos == haystack_runes.npos)
    {
      return JSONInt ^ "-1";
    }

    return JSONInt ^ std::to_string(pos);
  }

  Node indexof_n(const Nodes& args)
  {
    Node haystack = Resolver::unwrap(
      args[0], JSONString, "indexof: operand 1 ", EvalTypeError);
    if (haystack->type() == Error)
    {
      return haystack;
    }

    Node needle = Resolver::unwrap(
      args[1], JSONString, "indexof: operand 2 ", EvalTypeError);
    if (needle->type() == Error)
    {
      return needle;
    }

    runestring haystack_runes =
      utf8_to_runestring(Resolver::get_string(haystack));
    runestring needle_runes = utf8_to_runestring(Resolver::get_string(needle));
    Node array = NodeDef::create(Array);
    auto pos = haystack_runes.find(needle_runes);
    while (pos != haystack_runes.npos)
    {
      array->push_back(JSONInt ^ std::to_string(pos));
      pos = haystack_runes.find(needle_runes, pos + 1);
    }

    return array;
  }

  Node lower(const Nodes& args)
  {
    Node x =
      Resolver::unwrap(args[0], JSONString, "lower: operand 1 ", EvalTypeError);
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = Resolver::get_string(x);
    std::transform(
      x_str.begin(), x_str.end(), x_str.begin(), [](const char& c) {
        return static_cast<char>(std::tolower(c));
      });
    return Resolver::scalar(x_str);
  }

  Node upper(const Nodes& args)
  {
    Node x =
      Resolver::unwrap(args[0], JSONString, "upper: operand 1 ", EvalTypeError);
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = Resolver::get_string(x);
    std::transform(
      x_str.begin(), x_str.end(), x_str.begin(), [](const char& c) {
        return static_cast<char>(std::toupper(c));
      });
    return Resolver::scalar(x_str);
  }

  void replace(std::string& x, const std::string& old, const std::string& new_)
  {
    auto pos = x.find(old);
    while (pos != x.npos)
    {
      x.replace(pos, old.size(), new_);
      pos = x.find(old, pos + new_.size());
    }
  }

  Node replace(const Nodes& args)
  {
    Node x = Resolver::unwrap(
      args[0], JSONString, "replace: operand 1 ", EvalTypeError);
    if (x->type() == Error)
    {
      return x;
    }

    Node old = Resolver::unwrap(
      args[1], JSONString, "replace: operand 2 ", EvalTypeError);
    if (old->type() == Error)
    {
      return old;
    }

    Node new_ = Resolver::unwrap(
      args[2], JSONString, "replace: operand 3 ", EvalTypeError);
    if (new_->type() == Error)
    {
      return new_;
    }

    std::string x_str = Resolver::get_string(x);
    std::string old_str = Resolver::get_string(old);
    std::string new_str = Resolver::get_string(new_);
    replace(x_str, old_str, new_str);

    return Resolver::scalar(x_str);
  }

  Node split(const Nodes& args)
  {
    Node x =
      Resolver::unwrap(args[0], JSONString, "split: operand 1 ", EvalTypeError);
    if (x->type() == Error)
    {
      return x;
    }

    Node delimiter =
      Resolver::unwrap(args[1], JSONString, "split: operand 2 ", EvalTypeError);
    if (delimiter->type() == Error)
    {
      return delimiter;
    }

    std::string x_str = Resolver::get_string(x);
    std::string delimiter_str = Resolver::get_string(delimiter);
    Node array = NodeDef::create(Array);
    std::size_t start = 0;
    std::size_t pos = x_str.find(delimiter_str);
    while (pos != x_str.npos)
    {
      array->push_back(JSONString ^ x_str.substr(start, pos - start));
      start = pos + delimiter_str.size();
      pos = x_str.find(delimiter_str, start);
    }

    array->push_back(JSONString ^ x_str.substr(start));
    return array;
  }

  enum class PrintVerbType
  {
    Literal,
    Value,
    Boolean,
    Binary,
    Integer,
    Double,
    String,
  };

  struct PrintVerb
  {
    PrintVerbType type;
    std::string format;
  };

  std::vector<PrintVerb> parse_format(const std::string& format)
  {
    std::vector<PrintVerb> verbs;
    std::size_t start = 0;
    std::size_t pos = format.find('%');
    while (pos < format.size())
    {
      verbs.push_back(
        {PrintVerbType::Literal, format.substr(start, pos - start)});
      std::size_t verb_start = pos;
      bool in_verb = true;
      while (pos < format.size() && in_verb)
      {
        ++pos;
        in_verb = false;
        switch (format[pos])
        {
          case 't':
            verbs.push_back(
              {PrintVerbType::Boolean,
               format.substr(verb_start, pos - verb_start + 1)});
            break;

          case 'b':
            verbs.push_back(
              {PrintVerbType::Binary,
               format.substr(verb_start, pos - verb_start + 1)});
            break;

          case 'd':
          case 'x':
          case 'X':
          case 'o':
          case 'O':
            verbs.push_back(
              {PrintVerbType::Integer,
               format.substr(verb_start, pos - verb_start + 1)});
            break;

          case 'e':
          case 'E':
          case 'f':
          case 'F':
          case 'g':
          case 'G':
            verbs.push_back(
              {PrintVerbType::Double,
               format.substr(verb_start, pos - verb_start + 1)});
            break;

          case 's':
            verbs.push_back(
              {PrintVerbType::String,
               format.substr(verb_start, pos - verb_start + 1)});
            break;

          case '%':
            verbs.push_back({PrintVerbType::Literal, "%"});
            break;

          case 'v':
            verbs.push_back(
              {PrintVerbType::Value,
               format.substr(verb_start, pos - verb_start + 1)});
            break;

          default:
            in_verb = true;
        }
      }
      start = pos + 1;
      pos = format.find('%', start);
    }

    if (start < format.size())
    {
      verbs.push_back({PrintVerbType::Literal, format.substr(start)});
    }

    return verbs;
  }

  Node sprintf(const Nodes& args)
  {
    Node format = Resolver::unwrap(
      args[0], JSONString, "sprintf: operand 1 ", EvalTypeError);
    if (format->type() == Error)
    {
      return format;
    }

    Node values =
      Resolver::unwrap(args[1], Array, "sprintf: operand 2 ", EvalTypeError);
    if (values->type() == Error)
    {
      return values;
    }

    std::string format_str = Resolver::get_string(format);
    std::vector<PrintVerb> verbs = parse_format(format_str);
    std::ostringstream result;
    auto it = values->begin();
    for (auto& verb : verbs)
    {
      if (verb.type == PrintVerbType::Literal)
      {
        result << verb.format;
        continue;
      }

      if (it == values->end())
      {
        return err(args[1], "sprintf: not enough arguments", EvalTypeError);
      }

      const int buf_size = 1024;
      char buf[buf_size];
      Node node = *it;
      Node error;
      ++it;
      switch (verb.type)
      {
        case PrintVerbType::Value:
          result << to_json(node);
          break;

        case PrintVerbType::Boolean:
          result << std::boolalpha << Resolver::is_truthy(node);
          break;

        case PrintVerbType::Binary:
          result << std::bitset<8>(Resolver::get_int(values).to_int());
          break;

        case PrintVerbType::Integer:
          node = Resolver::unwrap(
            node, JSONInt, "sprintf: integer argument ", EvalTypeError);
          if (node->type() == Error)
          {
            return node;
          }
          std::snprintf(
            buf,
            buf_size,
            verb.format.c_str(),
            Resolver::get_int(node).to_int());
          result << buf;
          break;

        case PrintVerbType::Double:
          node = Resolver::unwrap(
            node,
            JSONFloat,
            "sprintf: floating-point argument ",
            EvalTypeError);
          if (node->type() == Error)
          {
            return node;
          }
          std::snprintf(
            buf, buf_size, verb.format.c_str(), Resolver::get_double(node));
          result << buf;
          break;

        case PrintVerbType::String:
          result << Resolver::get_string(node->front());
          break;

        case PrintVerbType::Literal:
          // Should never happen
          break;
      }
    }

    return JSONString ^ result.str();
  }

  Node any_prefix_match(const Nodes& args)
  {
    auto maybe_search =
      Resolver::maybe_unwrap(args[0], {JSONString, Array, Set});
    if (!maybe_search.has_value())
    {
      std::string type_name = Resolver::type_name(args[0]);
      return err(
        args[0],
        "strings.any_prefix_match: operand 1 must be one of {string, set, "
        "array} but got " +
          type_name,
        EvalTypeError);
    }

    auto maybe_base = Resolver::maybe_unwrap(args[1], {JSONString, Array, Set});
    if (!maybe_base.has_value())
    {
      std::string type_name = Resolver::type_name(args[1]);
      return err(
        args[1],
        "strings.any_prefix_match: operand 2 must be one of {string, set, "
        "array} but got " +
          type_name,
        EvalTypeError);
    }

    Node search = *maybe_search;
    std::vector<std::string> search_strings;
    if (search->type() == JSONString)
    {
      search_strings.push_back(Resolver::get_string(search));
    }
    else
    {
      Node bad_term = unwrap_strings(search, search_strings);
      if (bad_term)
      {
        return err(
          bad_term,
          "strings.any_prefix_match: operand 1 must be array of strings but "
          "got array containing " +
            Resolver::type_name(bad_term),
          EvalTypeError);
      }
    }

    Node base = *maybe_base;
    std::vector<std::string> base_strings;
    if (base->type() == JSONString)
    {
      base_strings.push_back(Resolver::get_string(base));
    }
    else
    {
      Node bad_term = unwrap_strings(base, base_strings);
      if (bad_term)
      {
        return err(
          bad_term,
          "strings.any_prefix_match: operand 2 must be array of strings but "
          "got array containing " +
            Resolver::type_name(bad_term),
          EvalTypeError);
      }
    }

    for (auto& search_str : search_strings)
    {
      for (auto& base_str : base_strings)
      {
        if (search_str.starts_with(base_str))
        {
          return JSONTrue ^ "true";
        }
      }
    }

    return JSONFalse ^ "false";
  }

  Node any_suffix_match(const Nodes& args)
  {
    auto maybe_search =
      Resolver::maybe_unwrap(args[0], {JSONString, Array, Set});
    if (!maybe_search.has_value())
    {
      std::string type_name = Resolver::type_name(args[0]);
      return err(
        args[0],
        "strings.any_suffix_match: operand 1 must be one of {string, set, "
        "array} but got " +
          type_name,
        EvalTypeError);
    }

    auto maybe_base = Resolver::maybe_unwrap(args[1], {JSONString, Array, Set});
    if (!maybe_base.has_value())
    {
      std::string type_name = Resolver::type_name(args[1]);
      return err(
        args[1],
        "strings.any_suffix_match: operand 2 must be one of {string, set, "
        "array} but got " +
          type_name,
        EvalTypeError);
    }

    Node search = *maybe_search;
    std::vector<std::string> search_strings;
    if (search->type() == JSONString)
    {
      search_strings.push_back(Resolver::get_string(search));
    }
    else
    {
      Node bad_term = unwrap_strings(search, search_strings);
      if (bad_term)
      {
        return err(
          bad_term,
          "strings.any_suffix_match: operand 1 must be array of strings but "
          "got array containing " +
            Resolver::type_name(bad_term),
          EvalTypeError);
      }
    }

    Node base = *maybe_base;
    std::vector<std::string> base_strings;
    if (base->type() == JSONString)
    {
      base_strings.push_back(Resolver::get_string(base));
    }
    else
    {
      Node bad_term = unwrap_strings(base, base_strings);
      if (bad_term)
      {
        return err(
          bad_term,
          "strings.any_suffix_match: operand 2 must be array of strings but "
          "got array containing " +
            Resolver::type_name(bad_term),
          EvalTypeError);
      }
    }

    for (auto& search_str : search_strings)
    {
      for (auto& base_str : base_strings)
      {
        if (search_str.ends_with(base_str))
        {
          return JSONTrue ^ "true";
        }
      }
    }

    return JSONFalse ^ "false";
  }

  Node replace_n(const Nodes& args)
  {
    Node patterns = Resolver::unwrap(
      args[0], Object, "strings.replace_n: operand 1 ", EvalTypeError);
    if (patterns->type() == Error)
    {
      return patterns;
    }

    Node value = Resolver::unwrap(
      args[1], JSONString, "strings.replace_n: operand 2 ", EvalTypeError);
    if (value->type() == Error)
    {
      return value;
    }

    std::string value_str = Resolver::get_string(value);
    for (auto& item : *patterns)
    {
      auto maybe_old = Resolver::maybe_unwrap_string(item / Key);
      if (!maybe_old.has_value())
      {
        return err(
          item / Key,
          "strings.replace_n: operand 1 non-string key found in pattern object",
          EvalTypeError);
      }

      auto maybe_new = Resolver::maybe_unwrap_string(item / Val);
      if (!maybe_new.has_value())
      {
        return err(
          item / Val,
          "strings.replace_n: operand 1 non-string value found in pattern "
          "object",
          EvalTypeError);
      }

      std::string old_str = Resolver::get_string(*maybe_old);
      std::string new_str = Resolver::get_string(*maybe_new);
      replace(value_str, old_str, new_str);
    }

    return JSONString ^ value_str;
  }

  Node reverse(const Nodes& args)
  {
    Node x = Resolver::unwrap(
      args[0], JSONString, "reverse: operand 1 ", EvalTypeError);
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = Resolver::get_string(x);
    std::vector<rune> x_runes = utf8_to_runes(x_str);
    std::reverse(x_runes.begin(), x_runes.end());
    std::ostringstream y;
    for (auto& rune : x_runes)
    {
      y << rune.source;
    }
    return JSONString ^ y.str();
  }

  Node substring(const Nodes& args)
  {
    Node value = Resolver::unwrap(
      args[0], JSONString, "substring: operand 1 ", EvalTypeError);
    if (value->type() == Error)
    {
      return value;
    }
    Node offset = Resolver::unwrap(
      args[1], JSONInt, "substring: operand 2 ", EvalTypeError);
    if (offset->type() == Error)
    {
      return offset;
    }
    Node length = Resolver::unwrap(
      args[2], JSONInt, "substring: operand 3 ", EvalTypeError);
    if (length->type() == Error)
    {
      return length;
    }

    std::string value_str = Resolver::get_string(value);
    std::vector<rune> value_runes = utf8_to_runes(value_str);
    std::int64_t offset_int = Resolver::get_int(offset).to_int();
    if (offset_int < 0)
    {
      return err(args[1], "negative offset", EvalBuiltInError);
    }

    std::size_t offset_size = static_cast<std::size_t>(offset_int);
    if (offset_size >= value_runes.size())
    {
      return JSONString ^ "";
    }

    std::int64_t length_int = Resolver::get_int(length).to_int();
    std::size_t length_size;
    if (length_int < 0)
    {
      length_size = value_runes.size() - offset_size;
    }
    else
    {
      length_size = static_cast<std::size_t>(length_int);
    }

    if (length_size > value_runes.size() - offset_size)
    {
      length_size = value_runes.size() - offset_size;
    }

    std::vector<rune> output_runes(
      value_runes.begin() + offset_size,
      value_runes.begin() + offset_size + length_size);
    std::ostringstream output;
    for (auto& rune : output_runes)
    {
      output << rune.source;
    }

    return JSONString ^ output.str();
  }

  std::string trim(
    const std::string& value, const std::string& cutset, bool left, bool right)
  {
    runestring value_runes = utf8_to_runestring(value);
    runestring cutset_runes = utf8_to_runestring(cutset);

    std::size_t start, end;
    if (left)
    {
      start = value_runes.find_first_not_of(cutset_runes);
    }
    else
    {
      start = 0;
    }

    if (right)
    {
      end = value_runes.find_last_not_of(cutset_runes);
    }
    else
    {
      end = value_runes.size();
    }

    if (start == value_runes.npos)
    {
      return "";
    }

    runestring output_runes = value_runes.substr(start, end - start + 1);
    return runestring_to_utf8(output_runes);
  }

  Node trim(const Nodes& args)
  {
    Node value =
      Resolver::unwrap(args[0], JSONString, "trim: operand 1 ", EvalTypeError);
    if (value->type() == Error)
    {
      return value;
    }
    Node cutset =
      Resolver::unwrap(args[1], JSONString, "trim: operand 2 ", EvalTypeError);
    if (cutset->type() == Error)
    {
      return cutset;
    }

    return JSONString ^
      trim(Resolver::get_string(value),
           Resolver::get_string(cutset),
           true,
           true);
  }

  Node trim_left(const Nodes& args)
  {
    Node value = Resolver::unwrap(
      args[0], JSONString, "trim_left: operand 1 ", EvalTypeError);
    if (value->type() == Error)
    {
      return value;
    }
    Node cutset = Resolver::unwrap(
      args[1], JSONString, "trim_left: operand 2 ", EvalTypeError);
    if (cutset->type() == Error)
    {
      return cutset;
    }

    return JSONString ^
      trim(Resolver::get_string(value),
           Resolver::get_string(cutset),
           true,
           false);
  }

  Node trim_right(const Nodes& args)
  {
    Node value = Resolver::unwrap(
      args[0], JSONString, "trim_right: operand 1 ", EvalTypeError);
    if (value->type() == Error)
    {
      return value;
    }
    Node cutset = Resolver::unwrap(
      args[1], JSONString, "trim_right: operand 2 ", EvalTypeError);
    if (cutset->type() == Error)
    {
      return cutset;
    }

    return JSONString ^
      trim(Resolver::get_string(value),
           Resolver::get_string(cutset),
           false,
           true);
  }

  Node trim_space(const Nodes& args)
  {
    Node value = Resolver::unwrap(
      args[0], JSONString, "trim_space: operand 1 ", EvalTypeError);
    if (value->type() == Error)
    {
      return value;
    }

    return JSONString ^
      trim(Resolver::get_string(value), " \t\n\r\v\f", true, true);
  }

  Node trim_prefix(const Nodes& args)
  {
    Node value = Resolver::unwrap(
      args[0], JSONString, "trim_prefix: operand 1 ", EvalTypeError);
    if (value->type() == Error)
    {
      return value;
    }
    Node prefix = Resolver::unwrap(
      args[1], JSONString, "trim_prefix: operand 2 ", EvalTypeError);
    if (prefix->type() == Error)
    {
      return prefix;
    }

    std::string value_str = Resolver::get_string(value);
    std::string prefix_str = Resolver::get_string(prefix);
    if (value_str.starts_with(prefix_str))
    {
      return JSONString ^ value_str.substr(prefix_str.size());
    }

    return value;
  }

  Node trim_suffix(const Nodes& args)
  {
    Node value = Resolver::unwrap(
      args[0], JSONString, "trim_suffix: operand 1 ", EvalTypeError);
    if (value->type() == Error)
    {
      return value;
    }
    Node suffix = Resolver::unwrap(
      args[1], JSONString, "trim_suffix: operand 2 ", EvalTypeError);
    if (suffix->type() == Error)
    {
      return suffix;
    }

    std::string value_str = Resolver::get_string(value);
    std::string suffix_str = Resolver::get_string(suffix);
    if (value_str.ends_with(suffix_str))
    {
      return JSONString ^
        value_str.substr(0, value_str.size() - suffix_str.size());
    }

    return value;
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> strings()
    {
      return {
        BuiltInDef::create(Location("concat"), 2, concat),
        BuiltInDef::create(Location("startswith"), 2, startswith),
        BuiltInDef::create(Location("endswith"), 2, endswith),
        BuiltInDef::create(Location("contains"), 2, contains),
        BuiltInDef::create(Location("format_int"), 2, format_int),
        BuiltInDef::create(Location("indexof"), 2, indexof),
        BuiltInDef::create(Location("indexof_n"), 2, indexof_n),
        BuiltInDef::create(Location("lower"), 1, lower),
        BuiltInDef::create(Location("upper"), 1, upper),
        BuiltInDef::create(Location("replace"), 3, replace),
        BuiltInDef::create(Location("split"), 2, split),
        BuiltInDef::create(Location("sprintf"), 2, sprintf),
        BuiltInDef::create(
          Location("strings.any_prefix_match"), 2, any_prefix_match),
        BuiltInDef::create(
          Location("strings.any_suffix_match"), 2, any_suffix_match),
        BuiltInDef::create(Location("strings.replace_n"), 2, replace_n),
        BuiltInDef::create(Location("strings.reverse"), 1, reverse),
        BuiltInDef::create(Location("substring"), 3, substring),
        BuiltInDef::create(Location("trim"), 2, trim),
        BuiltInDef::create(Location("trim_left"), 2, trim_left),
        BuiltInDef::create(Location("trim_right"), 2, trim_right),
        BuiltInDef::create(Location("trim_space"), 1, trim_space),
        BuiltInDef::create(Location("trim_prefix"), 2, trim_prefix),
        BuiltInDef::create(Location("trim_suffix"), 2, trim_suffix),
      };
    }
  }
}