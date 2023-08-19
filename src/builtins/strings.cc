#include "errors.h"
#include "register.h"
#include "resolver.h"
#include "utf8.h"

#include <bitset>

namespace
{
  using namespace rego;

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
    std::ostringstream result_str;
    std::string sep = "";
    for (auto node : *collection)
    {
      auto maybe_item = Resolver::maybe_unwrap_string(node);
      if (!maybe_item.has_value())
      {
        return err(node, "concat: invalid argument(s)", RegoTypeError);
      }

      Node item = *maybe_item;
      std::string item_str = Resolver::get_string(*maybe_item);
      result_str << sep << item_str;
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
      return err(args[0], "startswith: expected string arguments");
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
      return err(args[0], "endswith: expected string arguments");
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
      return err(args[0], "format_int: expected number argument");
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

    runestring haystack_runes = utf8_to_runes(Resolver::get_string(haystack));
    runestring needle_runes = utf8_to_runes(Resolver::get_string(needle));
    auto pos = haystack_runes.find(needle_runes);
    if(pos == haystack_runes.npos)
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

    runestring haystack_runes = utf8_to_runes(Resolver::get_string(haystack));
    runestring needle_runes = utf8_to_runes(Resolver::get_string(needle));
    Node array = NodeDef::create(Array);
    auto pos = haystack_runes.find(needle_runes);
    while(pos != haystack_runes.npos)
    {
      array->push_back(JSONInt ^ std::to_string(pos));
      pos = haystack_runes.find(needle_runes, pos + 1);
    }

    return array;
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
      };
    }
  }
}