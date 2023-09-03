#include "errors.h"
#include "helpers.h"
#include "register.h"
#include "resolver.h"

namespace
{
  using namespace rego;

  Node is_array(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Array}));
  }

  Node is_boolean(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {JSONTrue, JSONFalse}));
  }

  Node is_null(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {JSONNull}));
  }

  Node is_number(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {JSONInt, JSONFloat}));
  }

  Node is_object(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Object}));
  }

  Node is_set(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Set}));
  }

  Node is_string(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {JSONString}));
  }

  Node type_name(const Nodes& args)
  {
    return JSONString ^ rego::type_name(args[0]);
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> types()
    {
      return {
        BuiltInDef::create(Location("is_array"), 1, is_array),
        BuiltInDef::create(Location("is_boolean"), 1, is_boolean),
        BuiltInDef::create(Location("is_null"), 1, is_null),
        BuiltInDef::create(Location("is_number"), 1, is_number),
        BuiltInDef::create(Location("is_object"), 1, is_object),
        BuiltInDef::create(Location("is_set"), 1, is_set),
        BuiltInDef::create(Location("is_string"), 1, is_string),
        BuiltInDef::create(Location("type_name"), 1, ::type_name),
      };
    }
  }
}