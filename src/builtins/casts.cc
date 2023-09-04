#include "errors.h"
#include "helpers.h"
#include "register.h"
#include "resolver.h"

namespace
{
  using namespace rego;

  Node cast_array(const Nodes& args)
  {
    auto items =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Set}).exclude_got(true));
    if (items->type() == Error)
    {
      return items;
    }

    return Resolver::array(items);
  }

  Node cast_set(const Nodes& args)
  {
    auto items =
      unwrap_arg(args, UnwrapOpt(0).types({Array, Set}).exclude_got(true));
    if (items->type() == Error)
    {
      return items;
    }

    return Resolver::set(items);
  }

  Node cast_object(const Nodes& args)
  {
    Node object = unwrap_arg(args, UnwrapOpt(0).type(Object));
    if (object->type() == Error)
    {
      return object;
    }

    return object->clone();
  }

  Node cast_boolean(const Nodes& args)
  {
    Node boolean = unwrap_arg(args, UnwrapOpt(0).types({JSONTrue, JSONFalse}));
    if (boolean->type() == Error)
    {
      return boolean;
    }

    return boolean->clone();
  }

  Node cast_string(const Nodes& args)
  {
    Node string = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (string->type() == Error)
    {
      return string;
    }

    return string->clone();
  }

  Node cast_null(const Nodes& args)
  {
    Node null = unwrap_arg(args, UnwrapOpt(0).type(JSONNull));
    if (null->type() == Error)
    {
      return null;
    }

    return null->clone();
  }

  Node to_number(const Nodes& args)
  {
    Node number = unwrap_arg(
      args,
      UnwrapOpt(0).types(
        {JSONInt, JSONFloat, JSONString, JSONTrue, JSONFalse, JSONNull}));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == JSONString)
    {
      if (BigInt::is_int(number->location()))
      {
        return JSONInt ^ number->location();
      }

      std::string number_str = get_string(number);
      try
      {
        double float_value = std::stod(number_str);
        return Resolver::scalar(float_value);
      }
      catch (const std::invalid_argument)
      {
        return err(args[0], "invalid syntax", EvalBuiltInError);
      }
    }

    if (number->type() == JSONNull)
    {
      return JSONInt ^ "0";
    }

    if (number->type() == JSONTrue)
    {
      return JSONInt ^ "1";
    }

    if (number->type() == JSONFalse)
    {
      return JSONInt ^ "0";
    }

    return number->clone();
  }

}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> casts()
    {
      return {
        BuiltInDef::create(Location("cast_array"), 1, cast_array),
        BuiltInDef::create(Location("cast_boolean"), 1, cast_boolean),
        BuiltInDef::create(Location("cast_null"), 1, cast_null),
        BuiltInDef::create(Location("cast_set"), 1, cast_set),
        BuiltInDef::create(Location("cast_string"), 1, cast_string),
        BuiltInDef::create(Location("cast_object"), 1, cast_object),
        BuiltInDef::create(Location("to_number"), 1, to_number),
      };
    }
  }
}