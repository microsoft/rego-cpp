#include "errors.h"
#include "helpers.h"
#include "register.h"
#include "resolver.h"
#include "utf8.h"

namespace
{
  using namespace rego;

  Node count(const Nodes& args)
  {
    Node collection = unwrap_arg(
      args, UnwrapOpt(0).types({Array, Object, Set, JSONString}));
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
    Node collection = args[0];
    if (collection->type() == Term)
    {
      collection = collection->front();
    }

    if (collection->type() == Array || collection->type() == Set)
    {
      auto it = std::max_element(
        collection->begin(),
        collection->end(),
        [](const Node& a, const Node& b) { return to_json(a) < to_json(b); });

      return *it;
    }

    return err(args[0], "max: expected collection");
  }

  Node min(const Nodes& args)
  {
    Node collection = args[0];
    if (collection->type() == Term)
    {
      collection = collection->front();
    }

    if (collection->type() == Array || collection->type() == Set)
    {
      auto it = std::min_element(
        collection->begin(),
        collection->end(),
        [](const Node& a, const Node& b) { return to_json(a) < to_json(b); });

      return *it;
    }

    return err(args[0], "max: expected collection");
  }

  Node sort(const Nodes& args)
  {
    Node collection = args[0];
    if (collection->type() == Term)
    {
      collection = collection->front();
    }

    if (collection->type() == Array || collection->type() == Set)
    {
      Node sorted = collection->clone();
      std::sort(
        sorted->begin(), sorted->end(), [](const Node& a, const Node& b) {
          return to_json(a) < to_json(b);
        });

      return sorted;
    }

    return err(args[0], "max: expected collection");
  }

  Node sum(const Nodes& args)
  {
    Node collection = args[0];
    if (collection->type() == Term)
    {
      collection = collection->front();
    }

    if (collection->type() == Array || collection->type() == Set)
    {
      Node sum = std::accumulate(
        collection->begin(),
        collection->end(),
        JSONInt ^ "0",
        [](const Node& a, const Node& b) {
          return Resolver::arithinfix(Add ^ "+", a, b);
        });

      return Term << (Scalar << sum);
    }

    return err(args[0], "sum: expected collection");
  }

  Node product(const Nodes& args)
  {
    Node collection = args[0];
    if (collection->type() == Term)
    {
      collection = collection->front();
    }

    if (collection->type() == Array || collection->type() == Set)
    {
      Node sum = std::accumulate(
        collection->begin(),
        collection->end(),
        JSONInt ^ "1",
        [](const Node& a, const Node& b) {
          return Resolver::arithinfix(Multiply ^ "*", a, b);
        });

      return Term << (Scalar << sum);
    }

    return err(args[0], "sum: expected collection");
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> aggregates()
    {
      return std::vector<BuiltIn>{
        BuiltInDef::create(Location("count"), 1, count),
        BuiltInDef::create(Location("max"), 1, max),
        BuiltInDef::create(Location("min"), 1, min),
        BuiltInDef::create(Location("sort"), 1, sort),
        BuiltInDef::create(Location("sum"), 1, sum),
        BuiltInDef::create(Location("product"), 1, product)};
    }
  }
}