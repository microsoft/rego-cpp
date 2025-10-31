#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  const char* Message = "glob not supported";

  BuiltIn match_factory()
  {
    const Node match_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "pattern")
                      << (bi::Description ^ "glob pattern")
                      << (bi::Type << bi::String))
          << (bi::Arg << (bi::Name ^ "delimiters")
                      << (bi::Description ^
                          "glob pattern delimiters, e.g. `[\".\", \":\"]`, "
                          "defaults to `[\".x\"]` if unset. If `delimiters` is "
                          "`null`, glob match without a delimiter.")
                      << (bi::Type
                          << (bi::TypeSeq
                              << (bi::Type << bi::Null)
                              << (bi::Type
                                  << (bi::DynamicArray
                                      << (bi::Type << bi::String))))))
          << (bi::Arg << (bi::Name ^ "match")
                      << (bi::Description ^ "string to match against `pattern`")
                      << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true if `match` can be found in `pattern` which is "
                         "separated by `delimiters`")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::placeholder({"glob.match"}, match_decl, Message);
  }

  BuiltIn quote_meta_factory()
  {
    const Node quote_meta_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "pattern")
                               << (bi::Description ^ "glob patter")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^ "the escaped string of `pattern`")
                   << (bi::Type << bi::String));
    return BuiltInDef::placeholder(
      {"glob.quote_meta"}, quote_meta_decl, Message);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn glob(const Location& name)
    {
      assert(name.view().starts_with("glob."));
      std::string_view view = name.view().substr(5); // skip "glob."
      if (view == "match")
      {
        return match_factory();
      }
      if (view == "quote_meta")
      {
        return quote_meta_factory();
      }

      return nullptr;
    }
  }
}