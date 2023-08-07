#include "builtins.h"

#include "lang.h"
#include "resolver.h"

namespace builtins
{
  using namespace rego;

  Node startswith(const Nodes& args)
  {
    auto maybe_search = Resolver::maybe_unwrap_string(args[0]);
    auto maybe_base = Resolver::maybe_unwrap_string(args[1]);
    if (!maybe_search.has_value() || !maybe_base.has_value())
    {
      return err("startswith: expected string arguments");
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
      return err("endswith: expected string arguments");
    }

    std::string search = Resolver::get_string(*maybe_search);
    std::string base = Resolver::get_string(*maybe_base);
    return Resolver::scalar(search.ends_with(base));
  }

  Node count(const Nodes& args)
  {
    Node collection = args[0];
    if (
      collection->type() == Object || collection->type() == Array ||
      collection->type() == Set)
    {
      return Resolver::scalar(static_cast<std::int64_t>(collection->size()));
    }
    else if (collection->type() == JSONString)
    {
      std::string collection_str =
        strip_quotes(std::string(collection->location().view()));
      return Resolver::scalar(static_cast<std::int64_t>(collection_str.size()));
    }
    else
    {
      return err("count: expected collection");
    }
  }

  Node to_number(const Nodes& args)
  {
    auto maybe_number = Resolver::maybe_unwrap_string(args[0]);
    if (!maybe_number.has_value())
    {
      return err("to_number: expected string argument");
    }

    std::string number_str = Resolver::get_string(*maybe_number);

    try
    {
      std::int64_t int_value = std::stoll(number_str);
      return Resolver::scalar(int_value);
    }
    catch (const std::invalid_argument)
    {
      try
      {
        double float_value = std::stod(number_str);
        return Resolver::scalar(float_value);
      }
      catch (const std::invalid_argument)
      {
        return err("to_number: invalid number");
      }
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

  Node intersection(const Nodes& args)
  {
    return Resolver::set_intersection(args[0], args[1]);
  }

  Node union_(const Nodes& args)
  {
    return Resolver::set_union(args[0], args[1]);
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
    return s_builtins.contains(name);
  }

  Node BuiltIns::call(const Location& name, const Nodes& args) const
  {
    return s_builtins.at(name)->behavior(args);
  }

  BuiltIns& BuiltIns::register_builtin(const BuiltIn& built_in)
  {
    s_builtins[built_in->name] = built_in;
    return *this;
  }

  BuiltIns& BuiltIns::register_standard_builtins()
  {
    register_builtin(
      BuiltInDef::create(Location("startswith"), 2, builtins::startswith));
    register_builtin(
      BuiltInDef::create(Location("endswith"), 2, builtins::endswith));
    register_builtin(BuiltInDef::create(Location("count"), 1, builtins::count));
    register_builtin(
      BuiltInDef::create(Location("to_number"), 1, builtins::to_number));
    register_builtin(
      BuiltInDef::create(Location("print"), AnyArity, builtins::print));
    register_builtin(
      BuiltInDef::create(Location("union"), 2, builtins::union_));
    register_builtin(
      BuiltInDef::create(Location("intersection"), 2, builtins::intersection));
    return *this;
  }

  std::map<Location, BuiltIn>::const_iterator BuiltIns::begin() const
  {
    return s_builtins.begin();
  }

  std::map<Location, BuiltIn>::const_iterator BuiltIns::end() const
  {
    return s_builtins.end();
  }
}