#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node opa_runtime(const Nodes&)
  {
    return version();
  }

  BuiltIn opa_runtime_factory()
  {
    Node opa_runtime_decl = bi::Decl
      << bi::ArgSeq
      << (bi::Result
          << (bi::Name ^ "output")
          << (bi::Description ^
              "includes a `config` key if OPA was started with a configuration "
              "file; an `env` key containing the environment variables that "
              "the "
              "OPA process was started with; includes `version` and `commit` "
              "keys containing the version and build commit of OPA")
          << (bi::Type
              << (bi::DynamicObject << (bi::Type << bi::String)
                                    << (bi::Type << bi::Any))));
    return BuiltInDef::create(
      Location("opa.runtime"), opa_runtime_decl, ::opa_runtime);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn opa(const Location& name)
    {
      assert(name.view().starts_with("opa."));
      std::string_view view = name.view().substr(4); // skip "opa."
      if (view == "runtime")
      {
        return opa_runtime_factory();
      }

      return nullptr;
    }
  }
}