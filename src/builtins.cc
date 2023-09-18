#include "builtins/builtins.h"

namespace
{
  using namespace rego;

  Node opa_runtime(const Nodes&)
  {
    return version();
  }

  Node json_marshal(const Nodes& args)
  {
    return JSONString ^ to_json(args[0], true);
  }

  Node print(const Nodes& args)
  {
    std::ostringstream buf;
    std::string sep = "";
    for (auto arg : args)
    {
      if (arg->type() == Undefined)
      {
        return Resolver::scalar(false);
      }
      std::string json = to_json(arg);
      buf << sep << json;
      sep = " ";
    }
    buf << std::endl;
    std::cout << buf.str();
    return Resolver::scalar(true);
  }

}

namespace rego
{
  BuiltIn BuiltInDef::create(
    const Location& name, std::size_t arity, BuiltInBehavior behavior)
  {
    return std::make_shared<BuiltInDef>(BuiltInDef{name, arity, behavior});
  }

  BuiltIns::BuiltIns() noexcept : m_strict_errors(false) {}

  bool BuiltIns::strict_errors() const
  {
    return m_strict_errors;
  }

  BuiltIns& BuiltIns::strict_errors(bool strict_errors)
  {
    m_strict_errors = strict_errors;
    return *this;
  }

  bool BuiltIns::is_builtin(const Location& name) const
  {
    return contains(m_builtins, name);
  }

  Node BuiltIns::call(const Location& name, const Nodes& args) const
  {
    if (!is_builtin(name))
    {
      return err(args[0], "unknown builtin");
    }

    auto& builtin = m_builtins.at(name);
    if (builtin->arity != AnyArity && builtin->arity != args.size())
    {
      return err(args[0], "wrong number of arguments");
    }

    for (auto& arg : args)
    {
      if (arg->type() == Error)
      {
        return arg;
      }
    }

    Node result = builtin->behavior(args);
    if (result->type() == Error)
    {
      if (m_strict_errors)
      {
        return result;
      }
      else
      {
        return Undefined;
      }
    }

    return result;
  }

  BuiltIns& BuiltIns::register_builtin(const BuiltIn& built_in)
  {
    m_builtins[built_in->name] = built_in;
    return *this;
  }

  BuiltIns& BuiltIns::register_standard_builtins()
  {
    register_builtins<std::initializer_list<BuiltIn>>({
      BuiltInDef::create(Location("print"), AnyArity, ::print),
      BuiltInDef::create(Location("json.marshal"), 1, ::json_marshal),
      BuiltInDef::create(Location("opa.runtime"), 0, ::opa_runtime),
    });

    register_builtins(builtins::aggregates());
    register_builtins(builtins::arrays());
    register_builtins(builtins::bits());
    register_builtins(builtins::casts());
    register_builtins(builtins::encoding());
    register_builtins(builtins::numbers());
    register_builtins(builtins::objects());
    register_builtins(builtins::regex());
    register_builtins(builtins::sets());
    register_builtins(builtins::semver());
    register_builtins(builtins::strings());
    register_builtins(builtins::time());
    register_builtins(builtins::types());
    register_builtins(builtins::units());

    return *this;
  }

  std::map<Location, BuiltIn>::const_iterator BuiltIns::begin() const
  {
    return m_builtins.begin();
  }

  std::map<Location, BuiltIn>::const_iterator BuiltIns::end() const
  {
    return m_builtins.end();
  }

  const BuiltIn& BuiltIns::at(const Location& name) const
  {
    return m_builtins.at(name);
  }
}