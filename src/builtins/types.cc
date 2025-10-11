#include "builtins.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node is_array(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Array}));
  }

  Node is_array_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "input value")
                             << (bi::Type << bi::Any)))
             << (bi::Result << (bi::Name ^ "result")
                            << (bi::Description ^
                                "`true` if `x` is an array, `false` otherwise")
                            << (bi::Type << bi::Boolean));

  Node is_boolean(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {True, False}));
  }

  Node is_boolean_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "input value")
                             << (bi::Type << bi::Any)))
             << (bi::Result << (bi::Name ^ "result")
                            << (bi::Description ^
                                "`true` if `x` is a boolean, `false` otherwise")
                            << (bi::Type << bi::Boolean));

  Node is_null(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Null}));
  }

  Node is_null_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "input value")
                             << (bi::Type << bi::Any)))
             << (bi::Result << (bi::Name ^ "result")
                            << (bi::Description ^
                                "`true` if `x` is null, `false` otherwise")
                            << (bi::Type << bi::Boolean));

  Node is_number(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Int, Float}));
  }

  Node is_number_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "input value")
                             << (bi::Type << bi::Any)))
             << (bi::Result << (bi::Name ^ "result")
                            << (bi::Description ^
                                "`true` if `x` is a number, `false` otherwise")
                            << (bi::Type << bi::Boolean));

  Node is_object(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Object}));
  }

  Node is_object_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "input value")
                             << (bi::Type << bi::Any)))
             << (bi::Result << (bi::Name ^ "result")
                            << (bi::Description ^
                                "`true` if `x` is an object, `false` otherwise")
                            << (bi::Type << bi::Boolean));

  Node is_set(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {Set}));
  }

  Node is_set_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "input value")
                             << (bi::Type << bi::Any)))
             << (bi::Result << (bi::Name ^ "result")
                            << (bi::Description ^
                                "`true` if `x` is a set, `false` otherwise")
                            << (bi::Type << bi::Boolean));

  Node is_string(const Nodes& args)
  {
    return Resolver::scalar(is_instance(args[0], {JSONString}));
  }

  Node is_string_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "input value")
                             << (bi::Type << bi::Any)))
             << (bi::Result << (bi::Name ^ "result")
                            << (bi::Description ^
                                "`true` if `x` is a string, `false` otherwise")
                            << (bi::Type << bi::Boolean));

  Node type_name(const Nodes& args)
  {
    return JSONString ^ rego::type_name(args[0]);
  }

  Node type_name_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg << (bi::Name ^ "x") << (bi::Description ^ "input value")
                    << (bi::Type << bi::Any)))
    << (bi::Result
        << (bi::Name ^ "type")
        << (bi::Description ^
            R"(one of "null", "boolean", "number", "string", "array", "object", "set")")
        << (bi::Type << bi::String));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> types()
    {
      return {
        BuiltInDef::create(Location("is_array"), is_array_decl, is_array),
        BuiltInDef::create(Location("is_boolean"), is_boolean_decl, is_boolean),
        BuiltInDef::create(Location("is_null"), is_null_decl, is_null),
        BuiltInDef::create(Location("is_number"), is_number_decl, is_number),
        BuiltInDef::create(Location("is_object"), is_object_decl, is_object),
        BuiltInDef::create(Location("is_set"), is_set_decl, is_set),
        BuiltInDef::create(Location("is_string"), is_string_decl, is_string),
        BuiltInDef::create(Location("type_name"), type_name_decl, ::type_name),
      };
    }
  }
}