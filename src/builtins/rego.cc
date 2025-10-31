#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  namespace metadata
  {
    BuiltIn chain_factory()
    {
      const Node chain_decl = bi::Decl
        << bi::ArgSeq
        << (bi::Result
            << (bi::Name ^ "chain")
            << (bi::Description ^
                "each array entry represents a chain in the path "
                "ancestry (chain) "
                "of the active rule that also has declared annotations")
            << (bi::Type << (bi::DynamicArray << (bi::Type << bi::Any))));
      return BuiltInDef::placeholder(
        {"rego.metadata.chain"}, chain_decl, "Rego metadata not supported");
    }

    BuiltIn rule_factory()
    {
      const Node rule_decl = bi::Decl
        << bi::ArgSeq
        << (bi::Result << (bi::Name ^ "output")
                       << (bi::Description ^
                           "\"rule\" scope annotations for this rule; empty "
                           "object if no annotations exist")
                       << (bi::Type << bi::Any));
      return BuiltInDef::placeholder(
        {"rego.metadata.rule"}, rule_decl, "Rego metadata not supported");
    }
  } // namespace metadata

  BuiltIn parse_module_factory()
  {
    const Node parse_module_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg
                         << (bi::Name ^ "filename")
                         << (bi::Description ^
                             "file name to attached to AST nodes' lcoations")
                         << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "rego")
                                 << (bi::Description ^ "Rego module")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^ "AST object for the Rego module")
                     << (bi::Type
                         << (bi::DynamicObject << (bi::Type << bi::String)
                                               << (bi::Type << bi::Any))));
    return BuiltInDef::placeholder(
      {"rego.parse_module"},
      parse_module_decl,
      "Rego module parsing not supported");
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn rego(const Location& name)
    {
      assert(name.view().starts_with("rego."));
      std::string_view view = name.view().substr(5); // skip "rego."
      if (view == "metadata.chain")
      {
        return metadata::chain_factory();
      }
      if (view == "metadata.rule")
      {
        return metadata::rule_factory();
      }
      if (view == "parse_module")
      {
        return parse_module_factory();
      }

      return nullptr;
    }
  }
}