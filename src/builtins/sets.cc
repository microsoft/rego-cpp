#include "errors.h"
#include "helpers.h"
#include "register.h"
#include "resolver.h"

namespace
{
  using namespace rego;

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

  Node difference(const Nodes& args)
  {
    return Resolver::set_difference(args[0], args[1]);
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> sets()
    {
      return {
        BuiltInDef::create(Location("intersection"), 1, intersection),
        BuiltInDef::create(Location("union"), 1, union_),
        BuiltInDef::create(Location("set_diff"), 2, difference)};
    }
  }
}