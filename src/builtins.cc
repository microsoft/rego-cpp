#include "builtins.h"

#include "builtins/register.h"
#include "lang.h"
#include "resolver.h"

namespace
{
  using namespace rego;

  Node to_number(const Nodes& args)
  {
    auto maybe_number = Resolver::maybe_unwrap_string(args[0]);
    if (!maybe_number.has_value())
    {
      return err(args[0], "to_number: expected string argument");
    }

    if (BigInt::is_int(args[0]->location()))
    {
      return JSONInt ^ args[0]->location();
    }

    std::string number_str = Resolver::get_string(*maybe_number);
    try
    {
      double float_value = std::stod(number_str);
      return Resolver::scalar(float_value);
    }
    catch (const std::invalid_argument)
    {
      return err(args[0], "to_number: invalid number");
    }
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
      buf << sep << to_json(arg);
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
    return m_builtins.contains(name);
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
    register_builtin(BuiltInDef::create(Location("to_number"), 1, ::to_number));
    register_builtin(BuiltInDef::create(Location("print"), AnyArity, ::print));

    register_builtins(builtins::aggregates());
    register_builtins(builtins::arrays());
    register_builtins(builtins::encoding());
    register_builtins(builtins::numbers());
    register_builtins(builtins::objects());
    register_builtins(builtins::sets());
    register_builtins(builtins::semver());
    register_builtins(builtins::strings());

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