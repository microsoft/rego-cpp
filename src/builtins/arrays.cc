#include "errors.h"
#include "register.h"
#include "resolver.h"
#include "utils.h"

namespace
{
  using namespace rego;

  Node concat(const Nodes& args)
  {
    Node x = Resolver::unwrap(
      args[0], Array, "array.concat: operand 1 ", EvalTypeError);
    if (x->type() == Error)
    {
      return x;
    }

    Node y = Resolver::unwrap(
      args[1], Array, "array.concat: operand 2 ", EvalTypeError);
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
    Node arr = Resolver::unwrap(
      args[0], Array, "array.reverse: operand 1 ", EvalTypeError);
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
    Node arr = Resolver::unwrap(
      args[0], Array, "array.slice: operand 1 ", EvalTypeError);
    Node start_number = Resolver::unwrap(
      args[1], JSONInt, "array.slice: operand 2 ", EvalTypeError);
    Node end_number = Resolver::unwrap(
      args[2], JSONInt, "array.slice: operand 3 ", EvalTypeError);

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