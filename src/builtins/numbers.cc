#include "register.h"
#include "resolver.h"

namespace {
  using namespace rego;

  Node abs(const Nodes& args)
  {
    return Resolver::abs(args[0]);
  }

  Node ceil(const Nodes& args)
  {
    return Resolver::ceil(args[0]);
  }

  Node floor(const Nodes& args)
  {
    return Resolver::floor(args[0]);
  }
}

namespace rego {
  namespace builtins{
    std::vector<BuiltIn> numbers()
    {
      return {
        BuiltInDef::create(Location("abs"), 1, abs),
        BuiltInDef::create(Location("ceil"), 1, ceil),
        BuiltInDef::create(Location("floor"), 1, floor),
      };
    }
  }
}