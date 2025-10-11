#include "builtins.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node and_(const Nodes& args)
  {
    return Resolver::set_intersection(args[0], args[1]);
  }

  Node and_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "x") << (bi::Description ^ "the first set")
                     << (bi::Type << (bi::Set << (bi::Type << bi::Any))))
                 << (bi::Arg
                     << (bi::Name ^ "y") << (bi::Description ^ "the second set")
                     << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))
             << (bi::Result
                 << (bi::Name ^ "z")
                 << (bi::Description ^ "the intersection of `x` and `y`")
                 << (bi::Type << (bi::Set << (bi::Type << bi::Any))));

  Node or_(const Nodes& args)
  {
    return Resolver::set_union(args[0], args[1]);
  }

  Node or_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "x") << (bi::Description ^ "the first set")
                     << (bi::Type << (bi::Set << (bi::Type << bi::Any))))
                 << (bi::Arg
                     << (bi::Name ^ "y") << (bi::Description ^ "the second set")
                     << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))
             << (bi::Result
                 << (bi::Name ^ "z")
                 << (bi::Description ^ "the union of `x` and `y`")
                 << (bi::Type << (bi::Set << (bi::Type << bi::Any))));

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

  Node intersection_decl =
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

  Node union_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "xs")
                     << (bi::Description ^ "set of sets to merge")
                     << (bi::Type
                         << (bi::Set
                             << (bi::Type
                                 << (bi::Set << (bi::Type << bi::Any)))))))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "the union of all `xs` sets")
                 << (bi::Type << (bi::Set << (bi::Type << bi::Any))));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> sets()
    {
      return {
        BuiltInDef::create(Location("and"), and_decl, and_),
        BuiltInDef::create(
          Location("intersection"), intersection_decl, intersection),
        BuiltInDef::create(Location("or"), or_decl, or_),
        BuiltInDef::create(Location("union"), union_decl, union_)};
    }
  }
}