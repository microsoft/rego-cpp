#include "builtins.h"

namespace
{
  using namespace rego;

  Node count(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Object, Set, JSONString}));
    if (collection->type() == Error)
    {
      return collection;
    }

    if (collection->type() == JSONString)
    {
      std::string collection_str = get_string(collection);
      runestring collection_runes = utf8_to_runestring(collection_str);
      return Resolver::scalar(BigInt(collection_runes.size()));
    }

    return Resolver::scalar(BigInt(collection->size()));
  }

  Node max(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Set}).func("max"));
    if (collection->type() == Error)
    {
      return collection;
    }

    if (collection->size() == 0)
    {
      return Undefined ^ "undefined";
    }

    auto it = std::max_element(
      collection->begin(), collection->end(), [](const Node& a, const Node& b) {
        return to_json(a) < to_json(b);
      });

    return *it;
  }

  Node min(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Set}).func("min"));
    if (collection->type() == Error)
    {
      return collection;
    }

    if (collection->size() == 0)
    {
      return Undefined ^ "undefined";
    }

    auto it = std::min_element(
      collection->begin(), collection->end(), [](const Node& a, const Node& b) {
        return to_json(a) < to_json(b);
      });

    return *it;
  }

  Node sort(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Set}).func("sort"));
    if (collection->type() == Error)
    {
      return collection;
    }

    Node sorted = collection->clone();
    std::sort(sorted->begin(), sorted->end(), [](const Node& a, const Node& b) {
      return to_json(a) < to_json(b);
    });

    return sorted;
  }

  Node sum(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Set}).func("sum"));
    if (collection->type() == Error)
    {
      return collection;
    }

    return std::accumulate(
      collection->begin(),
      collection->end(),
      Int ^ "0",
      [](const Node& a, const Node& b) {
        return Resolver::arithinfix(Add ^ "+", a, b);
      });
  }

  Node product(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Set}).func("product"));
    if (collection->type() == Error)
    {
      return collection;
    }

    Node sum = std::accumulate(
      collection->begin(),
      collection->end(),
      Int ^ "1",
      [](const Node& a, const Node& b) {
        return Resolver::arithinfix(Multiply ^ "*", a, b);
      });

    return Term << (Scalar << sum);
  }

  Node any(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).func("any").types({Array, Set}));
    if (collection->type() == Error)
    {
      return collection;
    }

    for (const Node& item : *collection)
    {
      auto maybe_boolean = unwrap(item, {True, False});
      if (maybe_boolean.success)
      {
        if (maybe_boolean.node->type() == True)
        {
          return True ^ "true";
        }
      }
    }

    return False ^ "false";
  }

  Node all(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).func("all").types({Array, Set}));
    if (collection->type() == Error)
    {
      return collection;
    }

    for (const Node& item : *collection)
    {
      auto maybe_boolean = unwrap(item, {True, False});
      if (maybe_boolean.success)
      {
        if (maybe_boolean.node->type() == False)
        {
          return False ^ "false";
        }
      }
      else
      {
        return False ^ "false";
      }
    }

    return True ^ "true";
  }

}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> aggregates()
    {
      return std::vector<BuiltIn>{
        BuiltInDef::create(Location("all"), 1, all),
        BuiltInDef::create(Location("any"), 1, any),
        BuiltInDef::create(Location("count"), 1, count),
        BuiltInDef::create(Location("max"), 1, max),
        BuiltInDef::create(Location("min"), 1, min),
        BuiltInDef::create(Location("product"), 1, product),
        BuiltInDef::create(Location("sort"), 1, sort),
        BuiltInDef::create(Location("sum"), 1, sum),
      };
    }
  }
}