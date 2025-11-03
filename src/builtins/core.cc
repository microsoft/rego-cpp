#include "builtins.hh"
#include "trieste/json.h"
#include "trieste/utf8.h"

#include <bitset>

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

  BuiltIn count_factory()
  {
    const Node count_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg
              << (bi::Name ^ "collection")
              << (bi::Description ^ "the set/array/object/string to be counted")
              << (bi::Type
                  << (bi::TypeSeq
                      << (bi::Type << bi::String)
                      << (bi::Type
                          << (bi::DynamicArray << (bi::Type << bi::Any)))
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))))
      << (bi::Result << (bi::Name ^ "n")
                     << (bi::Description ^
                         "the count of elements, key/val pairs, or characters, "
                         "respectively.")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"count"}, count_decl, count);
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
        return to_key(a) < to_key(b);
      });

    return *it;
  }

  BuiltIn max_factory()
  {
    const Node max_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "collection")
                       << (bi::Description ^ "the set or array to be searched")
                       << (bi::Type
                           << (bi::TypeSeq
                               << (bi::Type
                                   << (bi::DynamicArray
                                       << (bi::Type << bi::Any)))
                               << (bi::Type
                                   << (bi::Set << (bi::Type << bi::Any)))))))
               << (bi::Result
                   << (bi::Name ^ "n")
                   << (bi::Description ^ "the maximum of all elements")
                   << (bi::Type << bi::Any));

    return BuiltInDef::create({"max"}, max_decl, max);
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
        return to_key(a) < to_key(b);
      });

    return *it;
  }

  BuiltIn min_factory()
  {
    const Node min_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "collection")
                       << (bi::Description ^ "the set or array to be searched")
                       << (bi::Type
                           << (bi::TypeSeq
                               << (bi::Type
                                   << (bi::DynamicArray
                                       << (bi::Type << bi::Any)))
                               << (bi::Type
                                   << (bi::Set << (bi::Type << bi::Any)))))))
               << (bi::Result
                   << (bi::Name ^ "n")
                   << (bi::Description ^ "the minimum of all elements")
                   << (bi::Type << bi::Any));

    return BuiltInDef::create({"min"}, min_decl, min);
  }

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

  BuiltIn sort_factory()
  {
    const Node sort_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "collection")
                       << (bi::Description ^ "the array or set to be sorted")
                       << (bi::Type
                           << (bi::TypeSeq
                               << (bi::Type
                                   << (bi::DynamicArray
                                       << (bi::Type << bi::Any)))
                               << (bi::Type
                                   << (bi::Set << (bi::Type << bi::Any)))))))
               << (bi::Result
                   << (bi::Name ^ "n") << (bi::Description ^ "the sorted array")
                   << (bi::Type
                       << (bi::DynamicArray << (bi::Type << bi::Any))));

    return BuiltInDef::create({"sort"}, sort_decl, sort);
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

  BuiltIn sum_factory()
  {
    const Node sum_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg
              << (bi::Name ^ "collection")
              << (bi::Description ^ "the set or array of numbers to sum")
              << (bi::Type
                  << (bi::TypeSeq
                      << (bi::Type
                          << (bi::DynamicArray << (bi::Type << bi::Number)))
                      << (bi::Type << (bi::Set << (bi::Type << bi::Number)))))))
      << (bi::Result << (bi::Name ^ "n")
                     << (bi::Description ^ "the sum of all elements")
                     << (bi::Type << bi::Number));

    return BuiltInDef::create({"sum"}, sum_decl, sum);
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

  BuiltIn product_factory()
  {
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
    return BuiltInDef::create({"product"}, product_decl, product);
  }

  Node equal(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(Equals), x, y);
  }

  BuiltIn equal_factory()
  {
    const Node equal_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Any)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true if `x` equals `y`; false otherwise")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"equal"}, equal_decl, equal);
  }

  Node gt(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(GreaterThan), x, y);
  }

  BuiltIn gt_factory()
  {
    const Node gt_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Any)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true if `x` is greater than `y`; false otherwise")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"gt"}, gt_decl, gt);
  }

  Node gte(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(GreaterThanOrEquals), x, y);
  }

  BuiltIn gte_factory()
  {
    const Node gte_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Any)))
      << (bi::Result
          << (bi::Name ^ "result")
          << (bi::Description ^
              "true if `x` is greater than or equal to `y`; false otherwise")
          << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"gte"}, gte_decl, gte);
  }

  Node lt(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(LessThan), x, y);
  }

  BuiltIn lt_factory()
  {
    const Node lt_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Any)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true if `x` is less than `y`; false otherwise")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"lt"}, lt_decl, lt);
  }

  Node lte(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(LessThanOrEquals), x, y);
  }

  BuiltIn lte_factory()
  {
    const Node lte_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Any)))
      << (bi::Result
          << (bi::Name ^ "result")
          << (bi::Description ^
              "true if `x` is less than or equal to `y`; false otherwise")
          << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"lte"}, lte_decl, lte);
  }

  Node neq(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(NotEquals), x, y);
  }

  BuiltIn neq_factory()
  {
    const Node neq_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Any)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true if `x` is not equal to `y`; false otherwise")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"neq"}, neq_decl, neq);
  }

  Node to_number(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float, JSONString, True, False, Null}));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == JSONString)
    {
      if (BigInt::is_int(number->location()))
      {
        return Int ^ number->location();
      }

      std::string number_str = get_string(number);
      try
      {
        double float_value = std::stod(number_str);

        if (std::isnan(float_value))
        {
          return err(number, "NAN", EvalTypeError);
        }

        if (std::isinf(float_value))
        {
          return err(number, "INF", EvalTypeError);
        }

        return Resolver::scalar(float_value);
      }
      catch (const std::invalid_argument&)
      {
        return err(args[0], "invalid syntax", EvalBuiltInError);
      }
    }

    if (number->type() == Null)
    {
      return Int ^ "0";
    }

    if (number->type() == True)
    {
      return Int ^ "1";
    }

    if (number->type() == False)
    {
      return Int ^ "0";
    }

    return number->clone();
  }

  BuiltIn to_number_factory()
  {
    Node to_number_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "x")
                       << (bi::Description ^ "value to convert")
                       << (bi::Type
                           << (bi::TypeSeq << (bi::Type << bi::Null)
                                           << (bi::Type << bi::Boolean)
                                           << (bi::Type << bi::Number)
                                           << (bi::Type << bi::String)))))
               << (bi::Result
                   << (bi::Name ^ "num")
                   << (bi::Description ^ "the numeric representation of `x`")
                   << (bi::Type << bi::Number));
    return BuiltInDef::create({"to_number"}, to_number_decl, to_number);
  }

  Node walk(const Nodes& args)
  {
    Node x = args[0];

    Node result = NodeDef::create(Array);
    std::deque<std::pair<Nodes, Node>> queue;
    queue.push_back({{}, x});
    while (!queue.empty())
    {
      auto [path_nodes, current] = queue.front();
      queue.pop_front();

      Node path_array = NodeDef::create(Array);
      for (auto& node : path_nodes)
      {
        path_array->push_back(Resolver::to_term(node->clone()));
      }

      result
        << (Term
            << (Array << (Term << path_array)
                      << Resolver::to_term(current->clone())));

      auto maybe_node = unwrap(current, {Array, Set, Object});
      if (!maybe_node.success)
      {
        continue;
      }

      current = maybe_node.node;
      if (current == Array)
      {
        for (size_t i = 0; i < current->size(); i++)
        {
          Node index = Resolver::term(BigInt(i));
          Nodes path(path_nodes);
          path.push_back(index);
          queue.push_back({path, current->at(i)});
        }
      }
      else if (current == Set)
      {
        for (auto child : *current)
        {
          Nodes path(path_nodes);
          path.push_back(child);
          queue.push_back({path, child});
        }
      }
      else if (current == Object)
      {
        for (auto child : *current)
        {
          Nodes path(path_nodes);
          path.push_back(child / Key);
          queue.push_back({path, child / Val});
        }
      }
    }

    return Term << result;
  }

  BuiltIn walk_factory()
  {
    Node walk_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "x") << (bi::Description ^ "value to walk")
                      << (bi::Type << bi::Any)))
      << (bi::Result
          << (bi::Name ^ "output")
          << (bi::Description ^
              "pairs of `path` and `value`: `path` is an array "
              "representing the pointer to `value` in `x`. If `path` is "
              "assigned a wildcard (`_`), the `walk` function will skip "
              "path creation entirely for faster evaluation.")
          << (bi::Type
              << (bi::DynamicArray
                  << (bi::Type
                      << (bi::StaticArray
                          << (bi::Type
                              << (bi::DynamicArray << (bi::Type << bi::Any)))
                          << (bi::Type << bi::Any))))));
    return BuiltInDef::create({"walk"}, walk_decl, walk);
  }

  Node print(const Nodes& args)
  {
    for (auto arg : args)
    {
      if (arg->type() == Undefined)
      {
        return Resolver::scalar(false);
      }
    }

    join(
      std::cout,
      args.begin(),
      args.end(),
      " ",
      [](std::ostream& stream, const Node& n) {
        stream << to_key(n);
        return true;
      })
      << std::endl;
    return Resolver::scalar(true);
  }

  BuiltIn print_factory()
  {
    Node print_decl = bi::Decl << bi::VarArgs << bi::Void;
    return BuiltInDef::create({"print"}, print_decl, print);
  }

  Node abs_(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == Int)
    {
      BigInt value = get_int(number).abs();
      return Int ^ value.loc();
    }
    else
    {
      double value = get_double(number);
      if (value < 0)
      {
        value *= -1;
      }
      return Float ^ std::to_string(value);
    }
  }

  BuiltIn abs_factory()
  {
    const Node abs_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^
                                   "the number to take the absolute value of")
                               << (bi::Type << bi::Number)))
               << (bi::Result << (bi::Name ^ "y")
                              << (bi::Description ^ "the absolute value of `x`")
                              << (bi::Type << bi::Number));

    return BuiltInDef::create({"abs"}, abs_decl, abs_);
  }

  Node ceil_(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == Int)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::ceil(get_double(number))));
      return Int ^ value.loc();
    }
  }

  BuiltIn ceil_factory()
  {
    const Node ceil_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "the number to round")
                               << (bi::Type << bi::Number)))
               << (bi::Result
                   << (bi::Name ^ "y")
                   << (bi::Description ^ "the result of rounding `x` _up_")
                   << (bi::Type << bi::Number));

    return BuiltInDef::create({"ceil"}, ceil_decl, ceil_);
  }

  Node floor_(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == Int)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::floor(get_double(number))));
      return Int ^ value.loc();
    }
  }

  BuiltIn floor_factory()
  {
    const Node floor_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "the number to round")
                               << (bi::Type << bi::Number)))
               << (bi::Result
                   << (bi::Name ^ "y")
                   << (bi::Description ^ "the result of rounding `x` _down_")
                   << (bi::Type << bi::Number));

    return BuiltInDef::create({"floor"}, floor_decl, floor_);
  }

  Node round_(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == Int)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::round(get_double(number))));
      return Int ^ value.loc();
    }
  }

  BuiltIn round_factory()
  {
    const Node round_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "the number to round")
                               << (bi::Type << bi::Number)))
               << (bi::Result
                   << (bi::Name ^ "y")
                   << (bi::Description ^ "the result of rounding `x`")
                   << (bi::Type << bi::Number));

    return BuiltInDef::create({"round"}, round_decl, round_);
  }

  Node plus_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Add), x, y);
  }

  BuiltIn plus_factory()
  {
    const Node plus_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the sum of `x` and `y`")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"plus"}, plus_decl, plus_);
  }

  Node minus_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float, Set}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float, Set}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Subtract), x, y);
  }

  BuiltIn minus_factory()
  {
    const Node minus_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg
                         << (bi::Name ^ "x") << bi::Description
                         << (bi::Type
                             << (bi::TypeSeq
                                 << (bi::Type << bi::Number)
                                 << (bi::Type
                                     << (bi::Set << (bi::Type << bi::Any))))))
                     << (bi::Arg
                         << (bi::Name ^ "y") << bi::Description
                         << (bi::Type
                             << (bi::TypeSeq
                                 << (bi::Type << bi::Number)
                                 << (bi::Type
                                     << (bi::Set << (bi::Type << bi::Any)))))))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the difference of `x` and `y`")
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type << bi::Number)
                             << (bi::Type
                                 << (bi::Set << (bi::Type << bi::Any))))));

    return BuiltInDef::create({"minus"}, minus_decl, minus_);
  }

  Node mul_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Multiply), x, y);
  }

  BuiltIn mul_factory()
  {
    const Node mul_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the product of `x` and `y`")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"mul"}, mul_decl, mul_);
  }

  Node div_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Divide), x, y);
  }

  BuiltIn div_factory()
  {
    const Node div_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the result of `x` divided by `y`")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"div"}, div_decl, div_);
  }

  Node rem_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Modulo), x, y);
  }

  BuiltIn rem_factory()
  {
    const Node rem_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "y") << bi::Description
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z") << (bi::Description ^ "the remainder")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"rem"}, rem_decl, rem_);
  }

  Node and_(const Nodes& args)
  {
    return Resolver::set_intersection(args[0], args[1]);
  }

  BuiltIn and_factory()
  {
    const Node and_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "x") << (bi::Description ^ "the first set")
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any))))
          << (bi::Arg << (bi::Name ^ "y")
                      << (bi::Description ^ "the second set")
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the intersection of `x` and `y`")
                     << (bi::Type << (bi::Set << (bi::Type << bi::Any))));
    return BuiltInDef::create({"and"}, and_decl, and_);
  }

  Node or_(const Nodes& args)
  {
    return Resolver::set_union(args[0], args[1]);
  }

  BuiltIn or_factory()
  {
    const Node or_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "x") << (bi::Description ^ "the first set")
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any))))
          << (bi::Arg << (bi::Name ^ "y")
                      << (bi::Description ^ "the second set")
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the union of `x` and `y`")
                     << (bi::Type << (bi::Set << (bi::Type << bi::Any))));
    return BuiltInDef::create({"or"}, or_decl, or_);
  }

  Node intersection(const Nodes& args)
  {
    Node xs_node =
      unwrap_arg(args, UnwrapOpt(0).type(Set).func("intersection"));
    if (xs_node->type() == Error)
    {
      return xs_node;
    }

    if (xs_node->size() == 0)
    {
      return NodeDef::create(Set);
    }

    Nodes xs(xs_node->begin(), xs_node->end());

    Node y = unwrap_arg(xs, UnwrapOpt(0).type(Set).pre("is set of sets"));
    if (y->type() == Error)
    {
      return y;
    }
    for (std::size_t i = 1; i < xs.size(); i++)
    {
      Node x = unwrap_arg(xs, UnwrapOpt(i).type(Set).pre("is set of sets"));
      if (x->type() == Error)
      {
        return x;
      }
      y = Resolver::set_intersection(y, x);
    }

    return y;
  }

  BuiltIn intersection_factory()
  {
    const Node intersection_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "xs")
                       << (bi::Description ^ "set of sets to intersect")
                       << (bi::Type
                           << (bi::Set
                               << (bi::Type
                                   << (bi::Set << (bi::Type << bi::Any)))))))
               << (bi::Result
                   << (bi::Name ^ "y")
                   << (bi::Description ^ "the intersection of all `xs` sets")
                   << (bi::Type << (bi::Set << (bi::Type << bi::Any))));
    return BuiltInDef::create(
      {"intersection"}, intersection_decl, intersection);
  }

  Node union_(const Nodes& args)
  {
    Node xs = unwrap_arg(args, UnwrapOpt(0).func("union").type(Set));
    if (xs->type() == Error)
    {
      return xs;
    }

    Node y = NodeDef::create(Set);
    for (Node x : *xs)
    {
      x = unwrap_arg({x}, UnwrapOpt(0).func("union").type(Set));
      if (x->type() == Error)
      {
        return x;
      }
      y = Resolver::set_union(y, x);
    }

    return y;
  }

  BuiltIn union_factory()
  {
    const Node union_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "xs")
                       << (bi::Description ^ "set of sets to union")
                       << (bi::Type
                           << (bi::Set
                               << (bi::Type
                                   << (bi::Set << (bi::Type << bi::Any)))))))
               << (bi::Result
                   << (bi::Name ^ "y")
                   << (bi::Description ^ "the union of all `xs` sets")
                   << (bi::Type << (bi::Set << (bi::Type << bi::Any))));
    return BuiltInDef::create({"union"}, union_decl, union_);
  }

  Node unwrap_strings(const Node& collection, std::vector<std::string>& items)
  {
    for (auto term : *collection)
    {
      auto maybe_item = unwrap(term, JSONString);
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

  BuiltIn concat_factory()
  {
    const Node concat_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "delimiter")
                      << (bi::Description ^ "string to use as a delimiter")
                      << (bi::Type << bi::String))
          << (bi::Arg << (bi::Name ^ "collection")
                      << (bi::Description ^ "strings to join")
                      << (bi::Type
                          << (bi::TypeSeq
                              << (bi::Type
                                  << (bi::DynamicArray
                                      << (bi::Type << bi::String)))
                              << (bi::Type
                                  << (bi::Set << (bi::Type << bi::String)))))))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^ "the joined string")
                     << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"concat"}, concat_decl, concat);
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
    return Resolver::scalar(search.starts_with(base));
  }

  BuiltIn startswith_factory()
  {
    const Node startswith_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "search")
                                 << (bi::Description ^ "search string")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "base")
                                 << (bi::Description ^ "base string")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^ "result of the prefix check")
                     << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"startswith"}, startswith_decl, startswith);
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

    return Resolver::scalar(search.ends_with(base));
  }

  BuiltIn endswith_factory()
  {
    const Node endswith_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "search")
                                 << (bi::Description ^ "search string")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "base")
                                 << (bi::Description ^ "base string")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^ "result of the suffix check")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"endswith"}, endswith_decl, endswith);
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

  BuiltIn contains_factory()
  {
    const Node contains_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "haystack")
                                 << (bi::Description ^ "string to search in")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "needle")
                                 << (bi::Description ^ "substring to look for")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^ "result of the containment check")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"contains"}, contains_decl, contains);
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
      auto maybe_int = get_int(number).to_int();
      if (!maybe_int.has_value())
      {
        return err(number, "not a valid integer", EvalTypeError);
      }
      value = maybe_int.value();
    }

    std::ostringstream result;
    auto maybe_base_size = get_int(base).to_size();
    if (!maybe_base_size.has_value())
    {
      return err(base, "not a valid base", EvalTypeError);
    }
    std::size_t base_size = maybe_base_size.value();
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

  BuiltIn format_int_factory()
  {
    const Node format_int_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "number")
                                 << (bi::Description ^ "number to format")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "base")
                                 << (bi::Description ^
                                     "base of number representation to use")
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^ "formatted number")
                     << (bi::Type << bi::String));

    return BuiltInDef::create({"format_int"}, format_int_decl, format_int);
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

  BuiltIn indexof_factory()
  {
    const Node indexof_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "haystack")
                                 << (bi::Description ^ "string to search in")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "needle")
                                 << (bi::Description ^ "substring to look for")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "index of first occurrence, `-1` if not found")
                     << (bi::Type << bi::Number));

    return BuiltInDef::create({"indexof"}, indexof_decl, indexof);
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

  BuiltIn indexof_n_factory()
  {
    const Node indexof_n_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "haystack")
                                 << (bi::Description ^ "string to search in")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "needle")
                                 << (bi::Description ^ "substring to look for")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "all indices at which `needle` occurs in `haystack`, "
                         "may be empty")
                     << (bi::Type
                         << (bi::DynamicArray << (bi::Type << bi::Number))));

    return BuiltInDef::create({"indexof_n"}, indexof_n_decl, indexof_n);
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

  BuiltIn lower_factory()
  {
    const Node lower_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^
                                   "string that is converted to lower-case")
                               << (bi::Type << bi::String)))
               << (bi::Result << (bi::Name ^ "y")
                              << (bi::Description ^ "lower-case of x")
                              << (bi::Type << bi::String));

    return BuiltInDef::create({"lower"}, lower_decl, lower);
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

  BuiltIn upper_factory()
  {
    const Node upper_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^
                                   "string that is converted to upper-case")
                               << (bi::Type << bi::String)))
               << (bi::Result << (bi::Name ^ "y")
                              << (bi::Description ^ "upper-case of x")
                              << (bi::Type << bi::String));
    return BuiltInDef::create({"upper"}, upper_decl, upper);
  }

  void do_replace(
    std::string& x, const std::string& old, const std::string& new_)
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
    do_replace(x_str, old_str, new_str);

    return Resolver::scalar(x_str);
  }

  BuiltIn replace_factory()
  {
    const Node replace_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string being processed")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "old")
                                 << (bi::Description ^ "substring to replace")
                                 << (bi::Type << bi::String))
                     << (bi::Arg
                         << (bi::Name ^ "new")
                         << (bi::Description ^ "string to replace `old` with")
                         << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "y")
                     << (bi::Description ^ "string with replaced substrings")
                     << (bi::Type << bi::String));

    return BuiltInDef::create({"replace"}, replace_decl, replace);
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

  BuiltIn split_factory()
  {
    const Node split_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string that is split")
                                 << (bi::Type << bi::String))
                     << (bi::Arg
                         << (bi::Name ^ "delimiter")
                         << (bi::Description ^ "delimiter used for splitting")
                         << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "ys") << (bi::Description ^ "split parts")
                     << (bi::Type
                         << (bi::DynamicArray << (bi::Type << bi::String))));

    return BuiltInDef::create({"split"}, split_decl, split);
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

  Node sprintf_(const Nodes& args)
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
          result << json::escape(to_key(node, false, false, ", "));
          break;

        case PrintVerbType::Boolean:
          result << std::boolalpha << is_truthy(node);
          break;

        case PrintVerbType::Binary: {
          auto maybe_bytes = get_int(values).to_int();
          if (!maybe_bytes.has_value())
          {
            return err(
              values, "sprintf: operand is not a valid integer", EvalTypeError);
          }
          result << std::bitset<8>(maybe_bytes.value());
          break;
        }

        case PrintVerbType::Integer: {
          node = unwrap_arg({node}, UnwrapOpt(0).type(Int).func("sprintf"));
          if (node->type() == Error)
          {
            return node;
          }
          auto maybe_int = get_int(node).to_int();
          if (!maybe_int.has_value())
          {
            return err(
              node, "sprintf: operand is not a valid integer", EvalTypeError);
          }
          std::snprintf(buf, buf_size, verb.format.c_str(), maybe_int.value());
          result << buf;
          break;
        }

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
          result << get_raw_string(node);
          break;

        case PrintVerbType::Literal:
          // Should never happen
          break;
      }
    }

    return JSONString ^ result.str();
  }

  BuiltIn sprintf_factory()
  {
    const Node sprintf_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg
                         << (bi::Name ^ "format")
                         << (bi::Description ^ "string with formatting verbs")
                         << (bi::Type << bi::String))
                     << (bi::Arg
                         << (bi::Name ^ "values")
                         << (bi::Description ^
                             "arguments to format into formatting verbs")
                         << (bi::Type
                             << (bi::DynamicArray << (bi::Type << bi::Any)))))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "`format` formatted by the values in `values`")
                     << (bi::Type << bi::String));

    return BuiltInDef::create({"sprintf"}, sprintf_decl, sprintf_);
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
    runestring value_runes = utf8_to_runestring(value_str);
    auto maybe_offset_int = get_int(offset).to_int();
    if (!maybe_offset_int)
    {
      return err(args[1], "offset out of range", EvalBuiltInError);
    }

    std::int64_t offset_int = maybe_offset_int.value();
    if (offset_int < 0)
    {
      return err(args[1], "negative offset", EvalBuiltInError);
    }

    std::size_t offset_size = static_cast<std::size_t>(offset_int);
    if (offset_size >= value_runes.size())
    {
      return JSONString ^ "";
    }

    auto maybe_length_int = get_int(length).to_int();
    if (!maybe_length_int)
    {
      return err(args[2], "length out of range", EvalBuiltInError);
    }

    std::int64_t length_int = maybe_length_int.value();
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

    std::ostringstream output;
    output << value_runes.substr(offset_size, length_size);
    return JSONString ^ output.str();
  }

  BuiltIn substring_factory()
  {
    const Node substring_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "value")
                       << (bi::Description ^ "string to extract substring from")
                       << (bi::Type << bi::String))
                   << (bi::Arg << (bi::Name ^ "offset")
                               << (bi::Description ^ "offset, must be positive")
                               << (bi::Type << bi::Number))
                   << (bi::Arg
                       << (bi::Name ^ "length")
                       << (bi::Description ^
                           "length of the substring starting from `offset`")
                       << (bi::Type << bi::Number)))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^
                       "substring of `value` from `offset`, of length `length`")
                   << (bi::Type << bi::String));
    return BuiltInDef::create({"substring"}, substring_decl, substring);
  }

  std::string do_trim(
    const std::string& value, const std::string& cutset, bool left, bool right)
  {
    runestring value_runes = utf8_to_runestring(json::unescape(value));
    runestring cutset_runes = utf8_to_runestring(json::unescape(cutset));

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

    std::ostringstream output;
    output << value_runes.substr(start, end - start + 1);
    return output.str();
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

    return JSONString ^
      do_trim(get_string(value), get_string(cutset), true, true);
  }

  BuiltIn trim_factory()
  {
    const Node trim_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "value")
                                 << (bi::Description ^ "string to trim")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "cutset")
                                 << (bi::Description ^
                                     "string of characters that are cut off")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "string trimmed of `cutset` characters")
                     << (bi::Type << bi::String));

    return BuiltInDef::create({"trim"}, trim_decl, trim);
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
      do_trim(get_string(value), get_string(cutset), true, false);
  }

  BuiltIn trim_left_factory()
  {
    const Node trim_left_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "value")
                               << (bi::Description ^ "string to trim")
                               << (bi::Type << bi::String))
                   << (bi::Arg
                       << (bi::Name ^ "cutset")
                       << (bi::Description ^
                           "string of characters that are cut off on the left")
                       << (bi::Type << bi::String)))
               << (bi::Result << (bi::Name ^ "output")
                              << (bi::Description ^
                                  "string left-trimmed of `cutset` characters")
                              << (bi::Type << bi::String));

    return BuiltInDef::create({"trim_left"}, trim_left_decl, trim_left);
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
      do_trim(get_string(value), get_string(cutset), false, true);
  }

  BuiltIn trim_right_factory()
  {
    const Node trim_right_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "value")
                               << (bi::Description ^ "string to trim")
                               << (bi::Type << bi::String))
                   << (bi::Arg
                       << (bi::Name ^ "cutset")
                       << (bi::Description ^
                           "string of characters that are cut off on the right")
                       << (bi::Type << bi::String)))
               << (bi::Result << (bi::Name ^ "output")
                              << (bi::Description ^
                                  "string right-trimmed of `cutset` characters")
                              << (bi::Type << bi::String));

    return BuiltInDef::create({"trim_right"}, trim_right_decl, trim_right);
  }

  Node trim_space(const Nodes& args)
  {
    Node value =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("trim_space"));
    if (value->type() == Error)
    {
      return value;
    }

    return JSONString ^ do_trim(get_string(value), " \t\n\r\v\f", true, true);
  }

  BuiltIn trim_space_factory()
  {
    const Node trim_space_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "value")
                               << (bi::Description ^ "string to trim")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^
                       "string leading and trailing white space cut off")
                   << (bi::Type << bi::String));

    return BuiltInDef::create({"trim_space"}, trim_space_decl, trim_space);
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
    if (value_str.starts_with(prefix_str))
    {
      return JSONString ^ value_str.substr(prefix_str.size());
    }

    return value;
  }

  BuiltIn trim_prefix_factory()
  {
    const Node trim_prefix_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "value")
                                 << (bi::Description ^ "string to trim")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "prefix")
                                 << (bi::Description ^ "prefix to cut off")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^ "string with `prefix` cut off")
                     << (bi::Type << bi::String));
    return BuiltInDef::create({"trim_prefix"}, trim_prefix_decl, trim_prefix);
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
    if (value_str.ends_with(suffix_str))
    {
      return JSONString ^
        value_str.substr(0, value_str.size() - suffix_str.size());
    }

    return value;
  }

  BuiltIn trim_suffix_factory()
  {
    const Node trim_suffix_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "value")
                                 << (bi::Description ^ "string to trim")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "suffix")
                                 << (bi::Description ^ "suffix to cut off")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^ "string with `suffix` cut off")
                     << (bi::Type << bi::String));
    return BuiltInDef::create({"trim_suffix"}, trim_suffix_decl, trim_suffix);
  }

  Node is_array(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Array}));
  }

  BuiltIn is_array_factory()
  {
    const Node is_array_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input value")
                               << (bi::Type << bi::Any)))
               << (bi::Result
                   << (bi::Name ^ "result")
                   << (bi::Description ^
                       "`true` if `x` is an array, `false` otherwise")
                   << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"is_array"}, is_array_decl, is_array);
  }

  Node is_boolean(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {True, False}));
  }

  BuiltIn is_boolean_factory()
  {
    const Node is_boolean_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input value")
                               << (bi::Type << bi::Any)))
               << (bi::Result
                   << (bi::Name ^ "result")
                   << (bi::Description ^
                       "`true` if `x` is a boolean, `false` otherwise")
                   << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"is_boolean"}, is_boolean_decl, is_boolean);
  }

  Node is_null(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Null}));
  }

  BuiltIn is_null_factory()
  {
    const Node is_null_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input value")
                               << (bi::Type << bi::Any)))
               << (bi::Result << (bi::Name ^ "result")
                              << (bi::Description ^
                                  "`true` if `x` is null, `false` otherwise")
                              << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"is_null"}, is_null_decl, is_null);
  }

  Node is_number(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Int, Float}));
  }

  BuiltIn is_number_factory()
  {
    const Node is_number_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input value")
                               << (bi::Type << bi::Any)))
               << (bi::Result
                   << (bi::Name ^ "result")
                   << (bi::Description ^
                       "`true` if `x` is a number, `false` otherwise")
                   << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"is_number"}, is_number_decl, is_number);
  }

  Node is_object(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Object}));
  }

  BuiltIn is_object_factory()
  {
    const Node is_object_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input value")
                               << (bi::Type << bi::Any)))
               << (bi::Result
                   << (bi::Name ^ "result")
                   << (bi::Description ^
                       "`true` if `x` is an object, `false` otherwise")
                   << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"is_object"}, is_object_decl, is_object);
  }

  Node is_set(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Set}));
  }

  BuiltIn is_set_factory()
  {
    const Node is_set_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input value")
                               << (bi::Type << bi::Any)))
               << (bi::Result << (bi::Name ^ "result")
                              << (bi::Description ^
                                  "`true` if `x` is a set, `false` otherwise")
                              << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"is_set"}, is_set_decl, is_set);
  }

  Node is_string(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {JSONString}));
  }

  BuiltIn is_string_factory()
  {
    const Node is_string_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "input value")
                               << (bi::Type << bi::Any)))
               << (bi::Result
                   << (bi::Name ^ "result")
                   << (bi::Description ^
                       "`true` if `x` is a string, `false` otherwise")
                   << (bi::Type << bi::Boolean));

    return BuiltInDef::create({"is_string"}, is_string_decl, is_string);
  }

  Node type_name_(const Nodes& args)
  {
    return JSONString ^ rego::type_name(args[0]);
  }

  BuiltIn type_name_factory()
  {
    const Node type_name_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "x") << (bi::Description ^ "input value")
                      << (bi::Type << bi::Any)))
      << (bi::Result
          << (bi::Name ^ "type")
          << (bi::Description ^
              R"(one of "null", "boolean", "number", "string", "array", "object", "set")")
          << (bi::Type << bi::String));

    return BuiltInDef::create({"type_name"}, type_name_decl, type_name_);
  }

  namespace strings
  {
    Node count(const Nodes& args)
    {
      Node search =
        unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("strings.count"));
      if (search->type() == Error)
      {
        return search;
      }

      Node substring =
        unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("strings.count"));
      if (substring->type() == Error)
      {
        return substring;
      }

      std::string search_str = get_string(search);
      std::string substring_str = get_string(substring);

      size_t pos = 0;
      size_t count = 0;
      while (pos < search_str.size())
      {
        pos = search_str.find(substring_str, pos);
        if (pos == std::string::npos)
        {
          break;
        }
        ++count;
        pos += substring_str.size();
      }

      return Int ^ std::to_string(count);
    }

    BuiltIn count_factory()
    {
      const Node count_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "search")
                                 << (bi::Description ^ "string to search in")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "substring")
                                 << (bi::Description ^ "substring to look for")
                                 << (bi::Type << bi::String)))
                 << (bi::Result << (bi::Name ^ "output")
                                << (bi::Description ^
                                    "count of occurrences, `0` if not found")
                                << (bi::Type << bi::Number));
      return BuiltInDef::create({"strings.count"}, count_decl, count);
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
          if (search_str.starts_with(base_str))
          {
            return True ^ "true";
          }
        }
      }

      return False ^ "false";
    }

    BuiltIn any_prefix_match_factory()
    {
      const Node any_prefix_match_decl = bi::Decl
        << (bi::ArgSeq
            << (bi::Arg << (bi::Name ^ "search")
                        << (bi::Description ^ "search string(s)")
                        << (bi::Type
                            << (bi::TypeSeq
                                << (bi::Type << bi::String)
                                << (bi::Type
                                    << (bi::DynamicArray
                                        << (bi::Type << bi::String)))
                                << (bi::Type
                                    << (bi::Set << (bi::Type << bi::String))))))
            << (bi::Arg
                << (bi::Name ^ "base") << (bi::Description ^ "base string(s)")
                << (bi::Type
                    << (bi::TypeSeq
                        << (bi::Type << bi::String)
                        << (bi::Type
                            << (bi::DynamicArray << (bi::Type << bi::String)))
                        << (bi::Type
                            << (bi::Set << (bi::Type << bi::String)))))))
        << (bi::Result << (bi::Name ^ "result")
                       << (bi::Description ^ "result of the prefix check")
                       << (bi::Type << bi::String));

      return BuiltInDef::create(
        {"strings.any_prefix_match"}, any_prefix_match_decl, any_prefix_match);
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
          if (search_str.ends_with(base_str))
          {
            return True ^ "true";
          }
        }
      }

      return False ^ "false";
    }

    BuiltIn any_suffix_match_factory()
    {
      const Node any_suffix_match_decl = bi::Decl
        << (bi::ArgSeq
            << (bi::Arg << (bi::Name ^ "search")
                        << (bi::Description ^ "search string(s)")
                        << (bi::Type
                            << (bi::TypeSeq
                                << (bi::Type << bi::String)
                                << (bi::Type
                                    << (bi::DynamicArray
                                        << (bi::Type << bi::String)))
                                << (bi::Type
                                    << (bi::Set << (bi::Type << bi::String))))))
            << (bi::Arg
                << (bi::Name ^ "base") << (bi::Description ^ "base string(s)")
                << (bi::Type
                    << (bi::TypeSeq
                        << (bi::Type << bi::String)
                        << (bi::Type
                            << (bi::DynamicArray << (bi::Type << bi::String)))
                        << (bi::Type
                            << (bi::Set << (bi::Type << bi::String)))))))
        << (bi::Result << (bi::Name ^ "result")
                       << (bi::Description ^ "result of the suffix check")
                       << (bi::Type << bi::String));

      return BuiltInDef::create(
        {"strings.any_suffix_match"}, any_suffix_match_decl, any_suffix_match);
    }

    BuiltIn render_template_factory()
    {
      const Node render_template_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "value")
                                 << (bi::Description ^ "a templated string")
                                 << (bi::Type << bi::String))
                     << (bi::Arg
                         << (bi::Name ^ "vars")
                         << (bi::Description ^
                             "a mapping of template variable keys to values")
                         << (bi::Type
                             << (bi::DynamicObject << (bi::Type << bi::String)
                                                   << (bi::Type << bi::Any)))))
                 << (bi::Result
                     << (bi::Name ^ "result")
                     << (bi::Description ^
                         "rendered template with template variables injected")
                     << (bi::Type << bi::String));

      return BuiltInDef::placeholder(
        {"render_template"},
        render_template_decl,
        "Render template is not supported");
    }

    Node replace_n(const Nodes& args)
    {
      Node patterns =
        unwrap_arg(args, UnwrapOpt(0).type(Object).func("strings.replace_n"));
      if (patterns->type() == Error)
      {
        return patterns;
      }

      Node value = unwrap_arg(
        args, UnwrapOpt(1).type(JSONString).func("strings.replace_n"));
      if (value->type() == Error)
      {
        return value;
      }

      std::map<std::string, std::string> pattern_map;

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
        pattern_map[old_str] = new_str;
      }

      for (const auto& [old_str, new_str] : pattern_map)
      {
        do_replace(value_str, old_str, new_str);
      }

      return JSONString ^ value_str;
    }

    BuiltIn replace_n_factory()
    {
      const Node replace_n_decl = bi::Decl
        << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "patterns")
                                   << (bi::Description ^ "replacement pairs")
                                   << (bi::Type
                                       << (bi::DynamicObject
                                           << (bi::Type << bi::String)
                                           << (bi::Type << bi::String))))

                       << (bi::Arg << (bi::Name ^ "value")
                                   << (bi::Description ^
                                       "string to replace substring matches in")
                                   << (bi::Type << bi::String)))
        << (bi::Result << (bi::Name ^ "output")
                       << (bi::Description ^ "string with replaced substrings")
                       << (bi::Type << bi::String));

      return BuiltInDef::create(
        {"strings.replace_n"}, replace_n_decl, replace_n);
    }

    Node reverse(const Nodes& args)
    {
      Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("reverse"));
      if (x->type() == Error)
      {
        return x;
      }

      std::string x_str = get_string(x);
      runestring x_runes = utf8_to_runestring(x_str);
      std::reverse(x_runes.begin(), x_runes.end());
      std::ostringstream y;
      y << x_runes;
      return JSONString ^ y.str();
    }

    BuiltIn reverse_factory()
    {
      const Node reverse_decl =
        bi::Decl << (bi::ArgSeq
                     << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "string to reverse")
                                 << (bi::Type << bi::String)))
                 << (bi::Result << (bi::Name ^ "y")
                                << (bi::Description ^ "reversed string")
                                << (bi::Type << bi::Number));

      return BuiltInDef::create({"reverse"}, reverse_decl, reverse);
    }
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn core(const Location& name)
    {
      const std::string_view& view = name.view();
      const char first_char = view.front();
      const char last_char = view.back();
      const std::size_t length = view.size();
      // BEGIN automatically generated by btree.py
      if (length > 5)
      {
        if (last_char > 'n')
        {
          if (first_char > 'i')
          {
            if (length > 9)
            {
              if (length > 10)
              {
                if (view == "trim_prefix")
                {
                  return trim_prefix_factory();
                }
                if (view == "trim_suffix")
                {
                  return trim_suffix_factory();
                }
              }
              if (view == "trim_right")
              {
                return trim_right_factory();
              }
            }
            if (length > 7)
            {
              if (last_char > 'r')
              {
                if (view == "trim_left")
                {
                  return trim_left_factory();
                }
              }
              if (view == "to_number")
              {
                return to_number_factory();
              }
            }
            if (view == "product")
            {
              return product_factory();
            }
          }
          if (length > 8)
          {
            if (length > 9)
            {
              if (view == "format_int")
              {
                return format_int_factory();
              }
            }
            if (last_char > 'r')
            {
              if (view == "is_object")
              {
                return is_object_factory();
              }
            }
            if (view == "is_number")
            {
              return is_number_factory();
            }
          }
          if (length > 6)
          {
            if (first_char > 'c')
            {
              if (view == "is_array")
              {
                return is_array_factory();
              }
            }
            if (view == "contains")
            {
              return contains_factory();
            }
          }
          if (first_char > 'c')
          {
            if (view == "is_set")
            {
              return is_set_factory();
            }
          }
          if (view == "concat")
          {
            return concat_factory();
          }
        }
        if (first_char > 'i')
        {
          if (last_char > 'e')
          {
            if (length > 7)
            {
              if (length > 9)
              {
                if (view == "startswith")
                {
                  return startswith_factory();
                }
              }
              if (view == "substring")
              {
                return substring_factory();
              }
            }
            if (view == "sprintf")
            {
              return sprintf_factory();
            }
          }
          if (length > 7)
          {
            if (length > 9)
            {
              if (view == "trim_space")
              {
                return trim_space_factory();
              }
            }
            if (view == "type_name")
            {
              return type_name_factory();
            }
          }
          if (view == "replace")
          {
            return replace_factory();
          }
        }
        if (last_char > 'h')
        {
          if (length > 7)
          {
            if (length > 9)
            {
              if (length > 10)
              {
                if (view == "intersection")
                {
                  return intersection_factory();
                }
              }
              if (view == "is_boolean")
              {
                return is_boolean_factory();
              }
            }
            if (view == "indexof_n")
            {
              return indexof_n_factory();
            }
          }
          if (view == "is_null")
          {
            return is_null_factory();
          }
        }
        if (length > 7)
        {
          if (length > 8)
          {
            if (view == "is_string")
            {
              return is_string_factory();
            }
          }
          if (view == "endswith")
          {
            return endswith_factory();
          }
        }
        if (view == "indexof")
        {
          return indexof_factory();
        }
      }
      if (length > 3)
      {
        if (first_char > 'm')
        {
          if (length > 4)
          {
            if (first_char > 'r')
            {
              if (first_char > 's')
              {
                if (last_char > 'n')
                {
                  if (view == "upper")
                  {
                    return upper_factory();
                  }
                }
                if (view == "union")
                {
                  return union_factory();
                }
              }
              if (view == "split")
              {
                return split_factory();
              }
            }
            if (view == "round")
            {
              return round_factory();
            }
          }
          if (first_char > 'p')
          {
            if (first_char > 's')
            {
              if (first_char > 't')
              {
                if (view == "walk")
                {
                  return walk_factory();
                }
              }
              if (view == "trim")
              {
                return trim_factory();
              }
            }
            if (view == "sort")
            {
              return sort_factory();
            }
          }
          if (view == "plus")
          {
            return plus_factory();
          }
        }
        if (first_char > 'c')
        {
          if (first_char > 'e')
          {
            if (first_char > 'f')
            {
              if (first_char > 'l')
              {
                if (view == "minus")
                {
                  return minus_factory();
                }
              }
              if (view == "lower")
              {
                return lower_factory();
              }
            }
            if (view == "floor")
            {
              return floor_factory();
            }
          }
          if (view == "equal")
          {
            return equal_factory();
          }
        }
        if (length > 4)
        {
          if (view == "count")
          {
            return count_factory();
          }
        }
        if (view == "ceil")
        {
          return ceil_factory();
        }
      }
      if (first_char > 'l')
      {
        if (first_char > 'm')
        {
          if (last_char > 'm')
          {
            if (length > 2)
            {
              if (view == "neq")
              {
                return neq_factory();
              }
            }
            if (view == "or")
            {
              return or_factory();
            }
          }
          if (first_char > 'r')
          {
            if (view == "sum")
            {
              return sum_factory();
            }
          }
          if (view == "rem")
          {
            return rem_factory();
          }
        }
        if (last_char > 'l')
        {
          if (last_char > 'n')
          {
            if (view == "max")
            {
              return max_factory();
            }
          }
          if (view == "min")
          {
            return min_factory();
          }
        }
        if (view == "mul")
        {
          return mul_factory();
        }
      }
      if (last_char > 'e')
      {
        if (length > 2)
        {
          if (first_char > 'a')
          {
            if (view == "div")
            {
              return div_factory();
            }
          }
          if (view == "abs")
          {
            return abs_factory();
          }
        }
        if (first_char > 'g')
        {
          if (view == "lt")
          {
            return lt_factory();
          }
        }
        if (view == "gt")
        {
          return gt_factory();
        }
      }
      if (first_char > 'a')
      {
        if (first_char > 'g')
        {
          if (view == "lte")
          {
            return lte_factory();
          }
        }
        if (view == "gte")
        {
          return gte_factory();
        }
      }
      if (view == "and")
      {
        return and_factory();
      }
      return nullptr;
      // END automatically generated by btree.py
    }

    BuiltIn strings(const Location& name)
    {
      std::string_view view = name.view().substr(8); // skip "strings."
      if (view == "count")
      {
        return strings::count_factory();
      }

      if (view == "any_prefix_match")
      {
        return strings::any_prefix_match_factory();
      }

      if (view == "any_suffix_match")
      {
        return strings::any_suffix_match_factory();
      }

      if (view == "render_template")
      {
        return strings::render_template_factory();
      }

      if (view == "replace_n")
      {
        return strings::replace_n_factory();
      }

      if (view == "reverse")
      {
        return strings::reverse_factory();
      }

      return nullptr;
    }
  }
}