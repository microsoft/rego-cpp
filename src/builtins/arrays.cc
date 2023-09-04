#include "errors.h"
#include "helpers.h"
#include "register.h"

namespace
{
  using namespace rego;

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

  Node slice(const Nodes& args)
  {
    Node arr = unwrap_arg(args, UnwrapOpt(0).func("array.slice").type(Array));
    Node start_number =
      unwrap_arg(args, UnwrapOpt(1).func("array.slice").type(JSONInt));
    Node end_number =
      unwrap_arg(args, UnwrapOpt(2).func("array.slice").type(JSONInt));

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
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> arrays()
    {
      return {
        BuiltInDef::create(Location("array.concat"), 2, concat),
        BuiltInDef::create(Location("array.reverse"), 1, reverse),
        BuiltInDef::create(Location("array.slice"), 3, slice),
      };
    }
  }
}