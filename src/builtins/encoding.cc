#include "base64/base64.h"
#include "errors.h"
#include "register.h"
#include "resolver.h"
#include "utils.h"

namespace
{
  using namespace rego;

  Node base64_encode_(const Nodes& args)
  {
    auto maybe_x_string = Resolver::maybe_unwrap_string(args[0]);
    if (!maybe_x_string.has_value())
    {
      return err(args[0], "Not a string");
    }

    std::string x_string =
      std::string(maybe_x_string.value()->location().view());
    std::string encoded = ::base64_encode(x_string);
    return JSONString ^ encoded;
  }

  Node base64_decode_(const Nodes& args)
  {
    auto maybe_x_string = Resolver::maybe_unwrap_string(args[0]);
    if (!maybe_x_string.has_value())
    {
      return err(args[0], "Not a string");
    }

    std::string x_string =
      strip_quotes(maybe_x_string.value()->location().view());
    std::string decoded = ::base64_decode(x_string);
    return JSONString ^ decoded;
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> encoding()
    {
      return {
        BuiltInDef::create(Location("base64.encode"), 1, base64_encode_),
        BuiltInDef::create(Location("base64.decode"), 1, base64_decode_)};
    }
  }
}