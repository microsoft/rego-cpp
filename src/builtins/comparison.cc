#include "builtins.h"
#include "rego.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node equal(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(Equals), x, y);
  }

  const Node equal_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                               << (bi::Type << bi::Any))
                   << (bi::Arg << (bi::Name ^ "y") << bi::Description
                               << (bi::Type << bi::Any)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^
                       "true if `x` is equal to `y`; false otherwise")
                   << (bi::Type << bi::Boolean));

  Node gt(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(GreaterThan), x, y);
  }

  const Node gt_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                               << (bi::Type << bi::Any))
                   << (bi::Arg << (bi::Name ^ "y") << bi::Description
                               << (bi::Type << bi::Any)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^
                       "true if `x` is greater than `y`; false otherwise")
                   << (bi::Type << bi::Boolean));

  Node gte(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(GreaterThanOrEquals), x, y);
  }

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

  Node lt(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(LessThan), x, y);
  }

  const Node lt_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                               << (bi::Type << bi::Any))
                   << (bi::Arg << (bi::Name ^ "y") << bi::Description
                               << (bi::Type << bi::Any)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^
                       "true if `x` is less than `y`; false otherwise")
                   << (bi::Type << bi::Boolean));

  Node lte(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(LessThanOrEquals), x, y);
  }

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

  Node neq(const Nodes& args)
  {
    Node x = args[0];
    Node y = args[1];
    return Resolver::boolinfix(NodeDef::create(NotEquals), x, y);
  }

  const Node neq_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                               << (bi::Type << bi::Any))
                   << (bi::Arg << (bi::Name ^ "y") << bi::Description
                               << (bi::Type << bi::Any)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^
                       "true if `x` is not equal to `y`; false otherwise")
                   << (bi::Type << bi::Boolean));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> comparison()
    {
      return {
        BuiltInDef::create(Location("equal"), equal_decl, equal),
        BuiltInDef::create(Location("gt"), gt_decl, gt),
        BuiltInDef::create(Location("gte"), gte_decl, gte),
        BuiltInDef::create(Location("lt"), lt_decl, lt),
        BuiltInDef::create(Location("lte"), lte_decl, lte),
        BuiltInDef::create(Location("neq"), neq_decl, neq)};
    }
  }
}