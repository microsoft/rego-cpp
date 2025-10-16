#include "builtins.h"
#include "trieste/utf8.h"

namespace
{
  using namespace rego;
  using namespace trieste::utf8;
  namespace bi = rego::builtins;

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

  const Node count_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg
            << (bi::Name ^ "collection")
            << (bi::Description ^ "the set/array/object/string to be counted")
            << (bi::Type
                << (bi::TypeSeq
                    << (bi::Type << bi::String)
                    << (bi::Type << (bi::DynamicArray << (bi::Type << bi::Any)))
                    << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))))
    << (bi::Result << (bi::Name ^ "n")
                   << (bi::Description ^
                       "the count of elements, key/val pairs, or characters, "
                       "respectively.")
                   << (bi::Type << bi::Number));

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
        return to_key(a) < to_key(b);
      });

    return *it;
  }

  const Node max_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "collection")
                     << (bi::Description ^ "the set or array to be searched")
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type
                                 << (bi::DynamicArray << (bi::Type << bi::Any)))
                             << (bi::Type
                                 << (bi::Set << (bi::Type << bi::Any)))))))
             << (bi::Result << (bi::Name ^ "n")
                            << (bi::Description ^ "the maximum of all elements")
                            << (bi::Type << bi::Any));

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
        return to_key(a) < to_key(b);
      });

    return *it;
  }

  const Node min_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "collection")
                     << (bi::Description ^ "the set or array to be searched")
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type
                                 << (bi::DynamicArray << (bi::Type << bi::Any)))
                             << (bi::Type
                                 << (bi::Set << (bi::Type << bi::Any)))))))
             << (bi::Result << (bi::Name ^ "n")
                            << (bi::Description ^ "the minimum of all elements")
                            << (bi::Type << bi::Any));

  Node sort(const Nodes& args)
  {
    Node collection =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Set}).func("sort"));
    if (collection->type() == Error)
    {
      return collection;
    }

    Nodes items;
    for (const Node& item : *collection)
    {
      items.push_back(item->clone());
    }

    std::sort(items.begin(), items.end(), [](const Node& a, const Node& b) {
      return to_key(a) < to_key(b);
    });

    return collection->type() << NodeRange{items.begin(), items.end()};
  }

  const Node sort_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "collection")
                     << (bi::Description ^ "the array or set to be sorted")
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type
                                 << (bi::DynamicArray << (bi::Type << bi::Any)))
                             << (bi::Type
                                 << (bi::Set << (bi::Type << bi::Any)))))))
             << (bi::Result
                 << (bi::Name ^ "n") << (bi::Description ^ "the sorted array")
                 << (bi::Type << (bi::DynamicArray << (bi::Type << bi::Any))));

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

  const Node sum_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "collection")
                     << (bi::Description ^ "the set or array of numbers to sum")
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type
                                 << (bi::DynamicArray
                                     << (bi::Type << bi::Number)))
                             << (bi::Type
                                 << (bi::Set << (bi::Type << bi::Number)))))))
             << (bi::Result << (bi::Name ^ "n")
                            << (bi::Description ^ "the sum of all elements")
                            << (bi::Type << bi::Number));

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

  const Node product_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg
            << (bi::Name ^ "collection")
            << (bi::Description ^ "the set or array of numbers to multiply")
            << (bi::Type
                << (bi::TypeSeq
                    << (bi::Type
                        << (bi::DynamicArray << (bi::Type << bi::Number)))
                    << (bi::Type << (bi::Set << (bi::Type << bi::Number)))))))
    << (bi::Result << (bi::Name ^ "n")
                   << (bi::Description ^ "the product of all elements")
                   << (bi::Type << bi::Number));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> aggregates()
    {
      return std::vector<BuiltIn>{
        BuiltInDef::create(Location("count"), count_decl, count),
        BuiltInDef::create(Location("max"), max_decl, max),
        BuiltInDef::create(Location("min"), min_decl, min),
        BuiltInDef::create(Location("product"), product_decl, product),
        BuiltInDef::create(Location("sort"), sort_decl, sort),
        BuiltInDef::create(Location("sum"), sum_decl, sum),
      };
    }
  }
}