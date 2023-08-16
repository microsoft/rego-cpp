#include "builtins.h"

#include "builtins/register.h"
#include "lang.h"
#include "resolver.h"

namespace
{
  using namespace rego;

  Node startswith(const Nodes& args)
  {
    auto maybe_search = Resolver::maybe_unwrap_string(args[0]);
    auto maybe_base = Resolver::maybe_unwrap_string(args[1]);
    if (!maybe_search.has_value() || !maybe_base.has_value())
    {
      return err(args[0], "startswith: expected string arguments");
    }

    std::string search = Resolver::get_string(*maybe_search);
    std::string base = Resolver::get_string(*maybe_base);
    return Resolver::scalar(search.starts_with(base));
  }

  Node endswith(const Nodes& args)
  {
    auto maybe_search = Resolver::maybe_unwrap_string(args[0]);
    auto maybe_base = Resolver::maybe_unwrap_string(args[1]);
    if (!maybe_search.has_value() || !maybe_base.has_value())
    {
      return err(args[0], "endswith: expected string arguments");
    }

    std::string search = Resolver::get_string(*maybe_search);
    std::string base = Resolver::get_string(*maybe_base);
    return Resolver::scalar(search.ends_with(base));
  }

  Node count(const Nodes& args)
  {
    Node collection = args[0];
    if (collection->type() == Term)
    {
      collection = collection->front();
    }

    if (
      collection->type() == Object || collection->type() == Array ||
      collection->type() == Set)
    {
      return Resolver::scalar(BigInt(collection->size()));
    }

    if (collection->type() == Scalar)
    {
      collection = collection->front();
    }

    if (collection->type() == JSONString)
    {
      std::string collection_str =
        strip_quotes(std::string(collection->location().view()));
      return Resolver::scalar(BigInt(collection_str.size()));
    }

    return err(args[0], "count: expected collection");
  }

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

  bool BuiltIns::is_builtin(const Location& name) const
  {
    return m_builtins.contains(name);
  }

  Node BuiltIns::call(const Location& name, const Nodes& args) const
  {
    return m_builtins.at(name)->behavior(args);
  }

  BuiltIns& BuiltIns::register_builtin(const BuiltIn& built_in)
  {
    m_builtins[built_in->name] = built_in;
    return *this;
  }

  BuiltIns& BuiltIns::register_standard_builtins()
  {
    register_builtin(
      BuiltInDef::create(Location("startswith"), 2, ::startswith));
    register_builtin(BuiltInDef::create(Location("endswith"), 2, ::endswith));
    register_builtin(BuiltInDef::create(Location("count"), 1, ::count));
    register_builtin(BuiltInDef::create(Location("to_number"), 1, ::to_number));
    register_builtin(BuiltInDef::create(Location("print"), AnyArity, ::print));

    register_builtins(builtins::sets());
    register_builtins(builtins::numbers());

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