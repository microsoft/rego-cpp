#include "builtins.hh"
#include "rego.hh"
#include "trieste/parse.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  const char* Message = "GraphQL is not supported";

  BuiltIn is_valid_factory()
  {
    const Node is_valid_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "query")
                      << (bi::Description ^ "the GraphQA query")
                      << (bi::Type
                          << (bi::TypeSeq << (bi::Type << bi::String)
                                          << (bi::Type
                                              << (bi::DynamicObject
                                                  << (bi::Type << bi::Any)
                                                  << (bi::Type << bi::Any))))))
          << (bi::Arg << (bi::Name ^ "schema")
                      << (bi::Description ^ "the GraphQL schema")
                      << (bi::Type
                          << (bi::TypeSeq << (bi::Type << bi::String)
                                          << (bi::Type
                                              << (bi::DynamicObject
                                                  << (bi::Type << bi::Any)
                                                  << (bi::Type << bi::Any)))))))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "`true` if the query is valid under the given schema. "
                         "`false` otherwise.")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::placeholder(
      {"graphql.is_valid"}, is_valid_decl, Message);
  }

  BuiltIn parse_factory()
  {
    const Node parse_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "query")
                       << (bi::Description ^ "the GraphQA query")
                       << (bi::Type
                           << (bi::TypeSeq << (bi::Type << bi::String)
                                           << (bi::Type
                                               << (bi::DynamicObject
                                                   << (bi::Type << bi::Any)
                                                   << (bi::Type << bi::Any))))))
                   << (bi::Arg << (bi::Name ^ "schema")
                               << (bi::Description ^ "the GraphQL schema")
                               << (bi::Type
                                   << (bi::TypeSeq
                                       << (bi::Type << bi::String)
                                       << (bi::Type
                                           << (bi::DynamicObject
                                               << (bi::Type << bi::Any)
                                               << (bi::Type << bi::Any)))))))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^
                       "`output` is of the form `[query_ast, schema_ast]`. If "
                       "the GraphQL query is valid given the provided schema, "
                       "then `query_ast` and `schema_ast` are objects "
                       "describing the ASTs for the query and schema.")
                   << (bi::Type
                       << (bi::StaticArray
                           << (bi::Type
                               << (bi::DynamicObject << (bi::Type << bi::Any)
                                                     << (bi::Type << bi::Any)))
                           << (bi::Type
                               << (bi::DynamicObject
                                   << (bi::Type << bi::Any)
                                   << (bi::Type << bi::Any))))));
    return BuiltInDef::placeholder({"graphql.parse"}, parse_decl, Message);
  }

  BuiltIn parse_and_verify_factory()
  {
    const Node parse_and_verify_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "query")
                      << (bi::Description ^ "the GraphQA query")
                      << (bi::Type
                          << (bi::TypeSeq << (bi::Type << bi::String)
                                          << (bi::Type
                                              << (bi::DynamicObject
                                                  << (bi::Type << bi::Any)
                                                  << (bi::Type << bi::Any))))))
          << (bi::Arg << (bi::Name ^ "schema")
                      << (bi::Description ^ "the GraphQL schema")
                      << (bi::Type
                          << (bi::TypeSeq << (bi::Type << bi::String)
                                          << (bi::Type
                                              << (bi::DynamicObject
                                                  << (bi::Type << bi::Any)
                                                  << (bi::Type << bi::Any)))))))
      << (bi::Result
          << (bi::Name ^ "output")
          << (bi::Description ^
              "`output` is of the form `[valid, query_ast, schema_ast]`. If "
              "the "
              "query is valid given the provided schema, then `valid` is "
              "`true`, "
              "and `query_ast` and `schema_ast` are objects describing the "
              "ASTs "
              "for the GraphQL query and schema. Otherwise, `valid` is `false` "
              "and `query_ast` and `schema_ast` are `{}`.")
          << (bi::Type
              << (bi::StaticArray
                  << (bi::Type << bi::Boolean)
                  << (bi::Type
                      << (bi::DynamicObject << (bi::Type << bi::Any)
                                            << (bi::Type << bi::Any)))
                  << (bi::Type
                      << (bi::DynamicObject << (bi::Type << bi::Any)
                                            << (bi::Type << bi::Any))))));
    return BuiltInDef::placeholder(
      {"graphql.parse_and_verify"}, parse_and_verify_decl, Message);
  }

  BuiltIn parse_query_factory()
  {
    const Node parse_query_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "query")
                               << (bi::Description ^ "GraphQL query string")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^ "AST object for the GraphQL query")
                   << (bi::Type
                       << (bi::DynamicObject << (bi::Type << bi::Any)
                                             << (bi::Type << bi::Any))));
    return BuiltInDef::placeholder(
      {"graphql.parse_query"}, parse_query_decl, Message);
  }

  BuiltIn parse_schema_factory()
  {
    const Node parse_schema_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "query")
                               << (bi::Description ^ "GraphQL schema string")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^ "AST object for the GraphQL schema")
                   << (bi::Type
                       << (bi::DynamicObject << (bi::Type << bi::Any)
                                             << (bi::Type << bi::Any))));
    return BuiltInDef::placeholder(
      {"graphql.parse_schema"}, parse_schema_decl, Message);
  }

  BuiltIn schema_is_valid_factory()
  {
    const Node schema_is_valid_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "query")
                               << (bi::Description ^ "GraphQL schema string")
                               << (bi::Type << bi::String)))
               << (bi::Result << (bi::Name ^ "output")
                              << (bi::Description ^
                                  "`true` if the schema is a valid GraphQL "
                                  "schema. `false` otherwise.")
                              << (bi::Type << bi::Boolean));
    return BuiltInDef::placeholder(
      {"graphql.schema_is_valid"}, schema_is_valid_decl, Message);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn graphql(const Location& name)
    {
      assert(name.view().starts_with("graphql."));
      std::string_view view = name.view().substr(8); // skip "graphql."
      if (view == "is_valid")
      {
        return is_valid_factory();
      }
      if (view == "parse")
      {
        return parse_factory();
      }
      if (view == "parse_and_verify")
      {
        return parse_and_verify_factory();
      }
      if (view == "parse_query")
      {
        return parse_query_factory();
      }
      if (view == "parse_schema")
      {
        return parse_schema_factory();
      }
      if (view == "schema_is_valid")
      {
        return schema_is_valid_factory();
      }

      return nullptr;
    }
  }
}