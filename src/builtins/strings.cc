#include "errors.h"
#include "register.h"
#include "resolver.h"

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
    Node haystack = Resolver::unwrap(args[0], JSONString, "contains: operand 1 ",
                                     EvalTypeError);
    if(haystack->type() == Error){
      return haystack;
    }

    Node needle = Resolver::unwrap(args[1], JSONString, "contains: operand 2 ",
                                   EvalTypeError);
    if(needle->type() == Error){
      return needle;
    }

    std::string haystack_str = Resolver::get_string(haystack);
    std::string needle_str = Resolver::get_string(needle);
    return Resolver::scalar(haystack_str.find(needle_str) != std::string::npos);
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
      };
    }
  }
}