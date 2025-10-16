#include "builtins.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node to_number(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float, JSONString, True, False, Null}));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == JSONString)
    {
      if (BigInt::is_int(number->location()))
      {
        return Int ^ number->location();
      }

      std::string number_str = get_string(number);
      try
      {
        double float_value = std::stod(number_str);

        if (std::isnan(float_value))
        {
          return err(number, "NAN", EvalTypeError);
        }

        if (std::isinf(float_value))
        {
          return err(number, "INF", EvalTypeError);
        }

        return Resolver::scalar(float_value);
      }
      catch (const std::invalid_argument&)
      {
        return err(args[0], "invalid syntax", EvalBuiltInError);
      }
    }

    if (number->type() == Null)
    {
      return Int ^ "0";
    }

    if (number->type() == True)
    {
      return Int ^ "1";
    }

    if (number->type() == False)
    {
      return Int ^ "0";
    }

    return number->clone();
  }

  Node to_number_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "value to convert")
                             << (bi::Type
                                 << (bi::TypeSeq << (bi::Type << bi::Null)
                                                 << (bi::Type << bi::Boolean)
                                                 << (bi::Type << bi::Number)
                                                 << (bi::Type << bi::String)))))
             << (bi::Result
                 << (bi::Name ^ "num")
                 << (bi::Description ^ "the numeric representation of `x`")
                 << (bi::Type << bi::Number));

}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> conversions()
    {
      return {
        BuiltInDef::create(Location("to_number"), to_number_decl, to_number),
      };
    }
  }
}