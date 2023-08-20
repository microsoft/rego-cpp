#include "register.h"
#include "resolver.h"
#include "errors.h"

namespace
{
  using namespace rego;

  Node intersection(const Nodes& args)
  {
    Node xs = Resolver::unwrap(args[0], Set, "intersection: operand 1 ", EvalTypeError);
    if(xs->type() == Error){
      return xs;
    }

    if(xs->size() == 0){
      return NodeDef::create(Set);
    }

    Node y = Resolver::unwrap(xs->at(0), Set, "intersection: operand 1 is set of sets ", EvalTypeError);
    if(y->type() == Error){
      return y;
    }
    for(std::size_t i = 1; i < xs->size(); i++){
      Node x = Resolver::unwrap(xs->at(i), Set, "intersection: operand 1 is set of sets ", EvalTypeError);
      if(x->type() == Error){
        return x;
      }
      y = Resolver::set_intersection(y, x);
    }

    return y;
  }

  Node union_(const Nodes& args)
  {
    Node xs = Resolver::unwrap(args[0], Set, "union: operand 1 ", EvalTypeError);
    if(xs->type() == Error){
      return xs;
    }

    Node y = NodeDef::create(Set);
    for(Node x : *xs){
      x = Resolver::unwrap(x, Set, "union: operand 1 is set of sets ", EvalTypeError);
      if(x->type() == Error){
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