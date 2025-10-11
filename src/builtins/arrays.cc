#include "builtins.h"
#include "rego.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node concat(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).func("array.concat").type(Array));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(args, UnwrapOpt(1).func("array.concat").type(Array));
    if (y->type() == Error)
    {
      return y;
    }

    Node z = x->clone();
    y = y->clone();
    z->insert(z->end(), y->begin(), y->end());
    return z;
  }

  Node concat_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg << (bi::Name ^ "x") << (bi::Description ^ "the first array")
                    << (bi::Type
                        << (bi::DynamicArray << (bi::Type << bi::Any))))
        << (bi::Arg << (bi::Name ^ "y")
                    << (bi::Description ^ "the second array")
                    << (bi::Type
                        << (bi::DynamicArray << (bi::Type << bi::Any)))))
    << (bi::Result << (bi::Name ^ "z")
                   << (bi::Description ^ "the concatenation of `x` and `y`")
                   << (bi::Type
                       << (bi::DynamicArray << (bi::Type << bi::Any))));

  Node reverse(const Nodes& args)
  {
    Node arr = unwrap_arg(args, UnwrapOpt(0).func("array.reverse").type(Array));
    if (arr->type() == Error)
    {
      return arr;
    }

    Node rev = NodeDef::create(Array);
    if (arr->size() == 0)
    {
      return rev;
    }

    for (auto it = arr->rbegin(); it != arr->rend(); ++it)
    {
      Node node = *it;
      rev->push_back(node->clone());
    }

    return rev;
  }

  Node reverse_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg << (bi::Name ^ "arr")
                    << (bi::Description ^ "the array to be reverse")
                    << (bi::Type
                        << (bi::DynamicArray << (bi::Type << bi::Any)))))
    << (bi::Result
        << (bi::Name ^ "rev")
        << (bi::Description ^
            "an array containing the elements of `arr` in reverse order")
        << (bi::Type << (bi::DynamicArray << (bi::Type << bi::Any))));

  Node slice(const Nodes& args)
  {
    Node arr = unwrap_arg(args, UnwrapOpt(0).func("array.slice").type(Array));
    Node start_number =
      unwrap_arg(args, UnwrapOpt(1).func("array.slice").type(Int));
    Node end_number =
      unwrap_arg(args, UnwrapOpt(2).func("array.slice").type(Int));

    std::int64_t raw_start = BigInt(start_number->location()).to_int();
    std::int64_t raw_end = BigInt(end_number->location()).to_int();
    std::size_t start, end;
    if (raw_start < 0)
    {
      start = 0;
    }
    else
    {
      start = std::min(static_cast<std::size_t>(raw_start), arr->size());
    }

    if (raw_end < 0)
    {
      end = 0;
    }
    else
    {
      end = std::min(static_cast<std::size_t>(raw_end), arr->size());
    }

    Node array = NodeDef::create(Array);

    if (start >= end)
    {
      return array;
    }

    auto start_it = arr->begin() + start;
    auto end_it = arr->begin() + end;
    for (auto it = start_it; it != end_it; ++it)
    {
      Node node = *it;
      array->push_back(node->clone());
    }

    return array;
  }

  Node slice_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg
                       << (bi::Name ^ "arr")
                       << (bi::Description ^ "the array to be reverse")
                       << (bi::Type
                           << (bi::DynamicArray << (bi::Type << bi::Any))))
                   << (bi::Arg << (bi::Name ^ "start")
                               << (bi::Description ^
                                   "the start index of the returned slice; if "
                                   "less than zero, it's clamped to 0")
                               << (bi::Type << bi::Number))
                   << (bi::Arg
                       << (bi::Name ^ "stop")
                       << (bi::Description ^
                           "the stop index of the returned slice; if larger "
                           "than `count(arr)`, it's clamped to `count(arr)`")
                       << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "slice")
                   << (bi::Description ^
                       "the subslice of `array`, from `start` to `end`, "
                       "including `arr[start]`, but excluding `arr[end]`")
                   << (bi::Type
                       << (bi::DynamicArray << (bi::Type << bi::Any))));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> arrays()
    {
      return {
        BuiltInDef::create(Location("array.concat"), concat_decl, concat),
        BuiltInDef::create(Location("array.reverse"), reverse_decl, reverse),
        BuiltInDef::create(Location("array.slice"), slice_decl, slice),
      };
    }
  }
}