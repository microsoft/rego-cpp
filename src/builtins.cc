#include "builtins/builtins.h"

#include "internal.hh"

namespace
{
  using namespace rego;

  Node opa_runtime(const Nodes&)
  {
    return version();
  }

  Node print(const Nodes& args)
  {
    for (auto arg : args)
    {
      if (arg->type() == Undefined)
      {
        return Resolver::scalar(false);
      }
    }

    join(
      std::cout,
      args.begin(),
      args.end(),
      " ",
      [](std::ostream& stream, const Node& n) {
        stream << to_key(n);
        return true;
      })
      << std::endl;
    return Resolver::scalar(true);
  }

}

namespace rego
{
  BuiltInDef::BuiltInDef(
    Location name_, std::size_t arity_, BuiltInBehavior behavior_) :
    name(name_), arity(arity_), behavior(behavior_)
  {}

  void BuiltInDef::clear() {}

  BuiltIn BuiltInDef::create(
    const Location& name, std::size_t arity, BuiltInBehavior behavior)
  {
    return std::make_shared<BuiltInDef>(BuiltInDef{name, arity, behavior});
  }

  BuiltInsDef::BuiltInsDef() noexcept : m_strict_errors(false) {}

  void BuiltInsDef::clear()
  {
    for (auto& builtin : m_builtins)
    {
      builtin.second->clear();
    }
  }

  bool BuiltInsDef::strict_errors() const
  {
    return m_strict_errors;
  }

  BuiltInsDef& BuiltInsDef::strict_errors(bool strict_errors)
  {
    m_strict_errors = strict_errors;
    return *this;
  }

  bool BuiltInsDef::is_builtin(const Location& name) const
  {
    return contains(m_builtins, name);
  }

  bool BuiltInsDef::is_deprecated(
    const Location& version, const Location& name) const
  {
    if (version.view() == "v0")
    {
      return false;
    }

    std::vector<std::string> deprecated = {
      "any",
      "all",
      "re_match",
      "net.cidr_overlap",
      "set_diff",
      "cast_array",
      "cast_set",
      "cast_string",
      "cast_boolean",
      "cast_null",
      "cast_object"};

    return std::find_if(
             deprecated.begin(),
             deprecated.end(),
             [name](const std::string& n) { return n == name.view(); }) !=
      deprecated.end();
  }

  Node BuiltInsDef::call(
    const Location& name, const Location& version, const Nodes& args)
  {
    if (!is_builtin(name))
    {
      return err(args[0], "unknown builtin");
    }

    if (is_deprecated(version, name))
    {
      std::ostringstream err_buff;
      err_buff << "deprecated built-in function calls in expression: "
               << name.view();
      return err(args[0], err_buff.str(), RegoTypeError);
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

  BuiltInsDef& BuiltInsDef::register_builtin(const BuiltIn& built_in)
  {
    m_builtins[built_in->name] = built_in;
    return *this;
  }

  BuiltInsDef& BuiltInsDef::register_standard_builtins(
    const std::filesystem::path& tzdata_path)
  {
    register_builtins<std::initializer_list<BuiltIn>>({
      BuiltInDef::create(Location("print"), AnyArity, ::print),
      BuiltInDef::create(Location("opa.runtime"), 0, ::opa_runtime),
    });

    register_builtins(builtins::aggregates());
    register_builtins(builtins::arrays());
    register_builtins(builtins::bits());
    register_builtins(builtins::casts());
    register_builtins(builtins::encoding());
    register_builtins(builtins::graph());
    register_builtins(builtins::numbers());
    register_builtins(builtins::objects());
    register_builtins(builtins::regex());
    register_builtins(builtins::sets());
    register_builtins(builtins::semver());
    register_builtins(builtins::strings());
    register_builtins(builtins::time(tzdata_path));
    register_builtins(builtins::types());
    register_builtins(builtins::units());
    register_builtins(builtins::uuid());

    return *this;
  }

  BuiltIns BuiltInsDef::create()
  {
    BuiltIns builtins = std::make_shared<BuiltInsDef>();
    builtins->register_standard_builtins();
    return builtins;
  }

  std::map<Location, BuiltIn>::const_iterator BuiltInsDef::begin() const
  {
    return m_builtins.begin();
  }

  std::map<Location, BuiltIn>::const_iterator BuiltInsDef::end() const
  {
    return m_builtins.end();
  }

  const BuiltIn& BuiltInsDef::at(const Location& name) const
  {
    return m_builtins.at(name);
  }
}