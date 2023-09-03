#include "base64/base64.h"
#include "errors.h"
#include "helpers.h"
#include "register.h"

namespace
{
  using namespace rego;

  Node base64_encode_(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
    std::string encoded = ::base64_encode(x_string);
    return JSONString ^ encoded;
  }

  Node base64_decode_(const Nodes& args)
  {
    Node x_string_node = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x_string_node->type() == Error)
    {
      return x_string_node;
    }

    std::string x_string = get_string(x_string_node);
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