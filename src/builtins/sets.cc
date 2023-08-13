#include "register.h"
#include "resolver.h"

namespace {
  using namespace rego;

  Node intersection(const Nodes& args)
  {
    return Resolver::set_intersection(args[0], args[1]);
  }

  Node union_(const Nodes& args)
  {
    return Resolver::set_union(args[0], args[1]);
  }  
}

namespace rego {
  namespace builtins {
    std::vector<BuiltIn> sets()
    {
      return {
        BuiltInDef::create(Location("intersection"), 2, intersection),
        BuiltInDef::create(Location("union"), 2, union_),
      };
    }
  }
}