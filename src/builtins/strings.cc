#include "builtins.h"

#include <bitset>

namespace
{
  using namespace rego;

  Node unwrap_strings(const Node& collection, std::vector<std::string>& items)
  {
    for (auto term : *collection)
    {
      auto maybe_item = unwrap(term, {JSONString});
      if (!maybe_item.success)
      {
        return maybe_item.node;
      }

      items.push_back(get_string(maybe_item.node));
    }

    return {};
  }

  Node concat(const Nodes& args)
  {
    Node delimiter =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("concat"));
    if (delimiter->type() == Error)
    {
      return delimiter;
    }

    Node collection =
      unwrap_arg(args, UnwrapOpt(1).types({Array, Set}).func("concat"));
    if (collection->type() == Error)
    {
      return collection;
    }

    auto delim_str = get_string(delimiter);
    std::vector<std::string> items;
    Node bad_term = unwrap_strings(collection, items);
    if (bad_term)
    {
      return err(
        bad_term,
        "concat: operand 2 must be array of strings but got array containing " +
          type_name(bad_term),
        EvalTypeError);
    }

    std::ostringstream result_str;
    for (auto it = items.begin(); it != items.end(); ++it)
    {
      if (it != items.begin())
      {
        result_str << delim_str;
      }
      result_str << *it;
    }

    return Resolver::scalar(result_str.str());
  }

  Node startswith(const Nodes& args)
  {
    Node search_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("startswith"));
    if (search_node->type() == Error)
    {
      return search_node;
    }

    Node base_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("startswith"));
    if (base_node->type() == Error)
    {
      return base_node;
    }

    std::string search = get_string(search_node);
    std::string base = get_string(base_node);
    return Resolver::scalar(starts_with(search, base));
  }

  Node endswith(const Nodes& args)
  {
    Node search_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("startswith"));
    if (search_node->type() == Error)
    {
      return search_node;
    }

    Node base_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("startswith"));
    if (base_node->type() == Error)
    {
      return base_node;
    }

    std::string search = get_string(search_node);
    std::string base = get_string(base_node);

    return Resolver::scalar(ends_with(search, base));
  }

  Node contains(const Nodes& args)
  {
    Node haystack =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("contains"));
    if (haystack->type() == Error)
    {
      return haystack;
    }

    Node needle =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("contains"));
    if (needle->type() == Error)
    {
      return needle;
    }

    std::string haystack_str = get_string(haystack);
    std::string needle_str = get_string(needle);
    return Resolver::scalar(haystack_str.find(needle_str) != std::string::npos);
  }

  Node format_int(const Nodes& args)
  {
    Node number =
      unwrap_arg(args, UnwrapOpt(0).types({Int, Float}).func("format_int"));
    if (number->type() == Error)
    {
      return number;
    }

    Node base = unwrap_arg(args, UnwrapOpt(1).type(Int).func("format_int"));
    if (base->type() == Error)
    {
      return base;
    }

    std::int64_t value;
    if (number->type() == Float)
    {
      value = static_cast<std::int64_t>(std::floor(get_double(number)));
    }
    else
    {
      value = get_int(number).to_int();
    }

    std::ostringstream result;
    std::size_t base_size = get_int(base).to_size();
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
    Node haystack =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("indexof"));
    if (haystack->type() == Error)
    {
      return haystack;
    }

    Node needle =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("indexof"));
    if (needle->type() == Error)
    {
      return needle;
    }

    runestring haystack_runes = utf8_to_runestring(get_string(haystack));
    runestring needle_runes = utf8_to_runestring(get_string(needle));
    auto pos = haystack_runes.find(needle_runes);
    if (pos == haystack_runes.npos)
    {
      return Int ^ "-1";
    }

    return Int ^ std::to_string(pos);
  }

  Node indexof_n(const Nodes& args)
  {
    Node haystack =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("indexof_n"));
    if (haystack->type() == Error)
    {
      return haystack;
    }

    Node needle =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("indexof_n"));
    if (needle->type() == Error)
    {
      return needle;
    }

    runestring haystack_runes = utf8_to_runestring(get_string(haystack));
    runestring needle_runes = utf8_to_runestring(get_string(needle));
    Node array = NodeDef::create(Array);
    auto pos = haystack_runes.find(needle_runes);
    while (pos != haystack_runes.npos)
    {
      array->push_back(Int ^ std::to_string(pos));
      pos = haystack_runes.find(needle_runes, pos + 1);
    }

    return array;
  }

  Node lower(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("lower"));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = get_string(x);
    std::transform(
      x_str.begin(), x_str.end(), x_str.begin(), [](const char& c) {
        return static_cast<char>(std::tolower(c));
      });
    return Resolver::scalar(x_str);
  }

  Node upper(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("upper"));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = get_string(x);
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
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("replace"));
    if (x->type() == Error)
    {
      return x;
    }

    Node old = unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("replace"));
    if (old->type() == Error)
    {
      return old;
    }

    Node new_ = unwrap_arg(args, UnwrapOpt(2).type(JSONString).func("replace"));
    if (new_->type() == Error)
    {
      return new_;
    }

    std::string x_str = get_string(x);
    std::string old_str = get_string(old);
    std::string new_str = get_string(new_);
    replace(x_str, old_str, new_str);

    return Resolver::scalar(x_str);
  }

  Node split(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("split"));
    if (x->type() == Error)
    {
      return x;
    }

    Node delimiter =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("split"));
    if (delimiter->type() == Error)
    {
      return delimiter;
    }

    std::string x_str = get_string(x);
    std::string delimiter_str = get_string(delimiter);
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
    Node format =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("sprintf"));
    if (format->type() == Error)
    {
      return format;
    }

    Node values =
      unwrap_arg(args, UnwrapOpt(1).types({Array, Set}).func("sprintf"));
    if (values->type() == Error)
    {
      return values;
    }

    std::string format_str = get_string(format);
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
          result << std::boolalpha << is_truthy(node);
          break;

        case PrintVerbType::Binary:
          result << std::bitset<8>(get_int(values).to_int());
          break;

        case PrintVerbType::Integer:
          node = unwrap_arg({node}, UnwrapOpt(0).type(Int).func("sprintf"));
          if (node->type() == Error)
          {
            return node;
          }
          std::snprintf(
            buf, buf_size, verb.format.c_str(), get_int(node).to_int());
          result << buf;
          break;

        case PrintVerbType::Double:
          node = unwrap_arg(
            {node}, UnwrapOpt(0).types({Int, Float}).func("sprintf"));
          if (node->type() == Error)
          {
            return node;
          }
          std::snprintf(buf, buf_size, verb.format.c_str(), get_double(node));
          result << buf;
          break;

        case PrintVerbType::String:
          result << get_string(node);
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
    Node search = unwrap_arg(
      args,
      UnwrapOpt(0).types({JSONString, Set, Array}).func("any_prefix_match"));
    if (search->type() == Error)
    {
      return search;
    }

    Node base = unwrap_arg(
      args,
      UnwrapOpt(1).types({JSONString, Set, Array}).func("any_prefix_match"));
    if (base->type() == Error)
    {
      return base;
    }

    std::vector<std::string> search_strings;
    if (search->type() == JSONString)
    {
      search_strings.push_back(get_string(search));
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
            type_name(bad_term),
          EvalTypeError);
      }
    }

    std::vector<std::string> base_strings;
    if (base->type() == JSONString)
    {
      base_strings.push_back(get_string(base));
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
            type_name(bad_term),
          EvalTypeError);
      }
    }

    for (auto& search_str : search_strings)
    {
      for (auto& base_str : base_strings)
      {
        if (starts_with(search_str, base_str))
        {
          return True ^ "true";
        }
      }
    }

    return False ^ "false";
  }

  Node any_suffix_match(const Nodes& args)
  {
    Node search = unwrap_arg(
      args,
      UnwrapOpt(0).types({JSONString, Set, Array}).func("any_suffix_match"));
    if (search->type() == Error)
    {
      return search;
    }

    Node base = unwrap_arg(
      args,
      UnwrapOpt(1).types({JSONString, Set, Array}).func("any_suffix_match"));
    if (base->type() == Error)
    {
      return base;
    }

    std::vector<std::string> search_strings;
    if (search->type() == JSONString)
    {
      search_strings.push_back(get_string(search));
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
            type_name(bad_term),
          EvalTypeError);
      }
    }

    std::vector<std::string> base_strings;
    if (base->type() == JSONString)
    {
      base_strings.push_back(get_string(base));
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
            type_name(bad_term),
          EvalTypeError);
      }
    }

    for (auto& search_str : search_strings)
    {
      for (auto& base_str : base_strings)
      {
        if (ends_with(search_str, base_str))
        {
          return True ^ "true";
        }
      }
    }

    return False ^ "false";
  }

  Node replace_n(const Nodes& args)
  {
    Node patterns =
      unwrap_arg(args, UnwrapOpt(0).type(Object).func("strings.replace_n"));
    if (patterns->type() == Error)
    {
      return patterns;
    }

    Node value =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("strings.replace_n"));
    if (value->type() == Error)
    {
      return value;
    }

    std::string value_str = get_string(value);
    for (auto& item : *patterns)
    {
      Node old_node = unwrap_arg(
        {item / Key},
        UnwrapOpt(0)
          .type(JSONString)
          .func("strings.replace_n")
          .message("operand 1 non-string key found in pattern object"));
      if (old_node->type() == Error)
      {
        return old_node;
      }

      Node new_node = unwrap_arg(
        {item / Val},
        UnwrapOpt(0)
          .type(JSONString)
          .func("strings.replace_n")
          .message("operand 1 non-string value found in pattern object"));
      if (new_node->type() == Error)
      {
        return new_node;
      }

      std::string old_str = get_string(old_node);
      std::string new_str = get_string(new_node);
      replace(value_str, old_str, new_str);
    }

    return JSONString ^ value_str;
  }

  Node reverse(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("reverse"));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = get_string(x);
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
    Node value =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("substring"));
    if (value->type() == Error)
    {
      return value;
    }
    Node offset = unwrap_arg(args, UnwrapOpt(1).type(Int).func("substring"));
    if (offset->type() == Error)
    {
      return offset;
    }
    Node length = unwrap_arg(args, UnwrapOpt(2).type(Int).func("substring"));
    if (length->type() == Error)
    {
      return length;
    }

    std::string value_str = get_string(value);
    std::vector<rune> value_runes = utf8_to_runes(value_str);
    std::int64_t offset_int = get_int(offset).to_int();
    if (offset_int < 0)
    {
      return err(args[1], "negative offset", EvalBuiltInError);
    }

    std::size_t offset_size = static_cast<std::size_t>(offset_int);
    if (offset_size >= value_runes.size())
    {
      return JSONString ^ "";
    }

    std::int64_t length_int = get_int(length).to_int();
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
    Node value = unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("trim"));
    if (value->type() == Error)
    {
      return value;
    }
    Node cutset = unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("trim"));
    if (cutset->type() == Error)
    {
      return cutset;
    }

    return JSONString ^ trim(get_string(value), get_string(cutset), true, true);
  }

  Node trim_left(const Nodes& args)
  {
    Node value =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("trim_left"));
    if (value->type() == Error)
    {
      return value;
    }
    Node cutset =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("trim_left"));
    if (cutset->type() == Error)
    {
      return cutset;
    }

    return JSONString ^
      trim(get_string(value), get_string(cutset), true, false);
  }

  Node trim_right(const Nodes& args)
  {
    Node value =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("trim_right"));
    if (value->type() == Error)
    {
      return value;
    }
    Node cutset =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("trim_right"));
    if (cutset->type() == Error)
    {
      return cutset;
    }

    return JSONString ^
      trim(get_string(value), get_string(cutset), false, true);
  }

  Node trim_space(const Nodes& args)
  {
    Node value =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("trim_space"));
    if (value->type() == Error)
    {
      return value;
    }

    return JSONString ^ trim(get_string(value), " \t\n\r\v\f", true, true);
  }

  Node trim_prefix(const Nodes& args)
  {
    Node value =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("trim_prefix"));
    if (value->type() == Error)
    {
      return value;
    }
    Node prefix =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("trim_prefix"));
    if (prefix->type() == Error)
    {
      return prefix;
    }

    std::string value_str = get_string(value);
    std::string prefix_str = get_string(prefix);
    if (starts_with(value_str, prefix_str))
    {
      return JSONString ^ value_str.substr(prefix_str.size());
    }

    return value;
  }

  Node trim_suffix(const Nodes& args)
  {
    Node value =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("trim_suffix"));
    if (value->type() == Error)
    {
      return value;
    }
    Node suffix =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("trim_suffix"));
    if (suffix->type() == Error)
    {
      return suffix;
    }

    std::string value_str = get_string(value);
    std::string suffix_str = get_string(suffix);
    if (ends_with(value_str, suffix_str))
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
        BuiltInDef::create(Location("contains"), 2, ::contains),
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