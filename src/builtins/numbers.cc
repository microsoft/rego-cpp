#include "builtins.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node abs_(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == Int)
    {
      BigInt value = get_int(number).abs();
      return Int ^ value.loc();
    }
    else
    {
      double value = get_double(number);
      if (value < 0)
      {
        value *= -1;
      }
      return Float ^ std::to_string(value);
    }
  }

  const Node abs_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^
                                 "the number to take the absolute value of")
                             << (bi::Type << bi::Number)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^ "the absolute value of `x`")
                            << (bi::Type << bi::Number));

  Node ceil_(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == Int)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::ceil(get_double(number))));
      return Int ^ value.loc();
    }
  }

  const Node ceil_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the number to round")
                             << (bi::Type << bi::Number)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "the result of rounding `x` _up_")
                 << (bi::Type << bi::Number));

  Node floor_(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == Int)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::floor(get_double(number))));
      return Int ^ value.loc();
    }
  }

  const Node floor_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the number to round")
                             << (bi::Type << bi::Number)))
             << (bi::Result
                 << (bi::Name ^ "y")
                 << (bi::Description ^ "the result of rounding `x` _down_")
                 << (bi::Type << bi::Number));

  Node round_(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == Int)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::round(get_double(number))));
      return Int ^ value.loc();
    }
  }

  const Node round_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the number to round")
                             << (bi::Type << bi::Number)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^ "the result of rounding `x`")
                            << (bi::Type << bi::Number));

  Node numbers_range(const Nodes& args)
  {
    Node lhs_number = unwrap_arg(
      args, UnwrapOpt(0).type(Int).func("numbers.range").specify_number(true));
    if (lhs_number->type() == Error)
    {
      return lhs_number;
    }

    Node rhs_number = unwrap_arg(
      args, UnwrapOpt(1).type(Int).func("numbers.range").specify_number(true));
    if (rhs_number->type() == Error)
    {
      return rhs_number;
    }

    BigInt lhs = get_int(lhs_number);
    BigInt rhs = get_int(rhs_number);
    Node array = Array ^ args[0];
    if (lhs < rhs)
    {
      BigInt curr = lhs;
      while (curr < rhs)
      {
        array->push_back(Term << (Scalar << (Int ^ curr.loc())));
        curr = curr.increment();
      }
      array->push_back(Term << (Scalar << (Int ^ curr.loc())));
    }
    else
    {
      BigInt curr = lhs;
      while (curr > rhs)
      {
        array->push_back(Term << (Scalar << (Int ^ curr.loc())));
        curr = curr.decrement();
      }
      array->push_back(Term << (Scalar << (Int ^ curr.loc())));
    }

    return array;
  }

  const Node numbers_range_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "a")
                               << (bi::Description ^ "the start of the range")
                               << (bi::Type << bi::Number))
                   << (bi::Arg
                       << (bi::Name ^ "b")
                       << (bi::Description ^ "the end of the range (inclusive)")
                       << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "range")
                   << (bi::Description ^ "the range between `a` and `b`")
                   << (bi::Type
                       << (bi::DynamicArray << (bi::Type << bi::Number))));

  Node numbers_range_step(const Nodes& args)
  {
    Node lhs_number = unwrap_arg(
      args,
      UnwrapOpt(0).type(Int).func("numbers.range_step").specify_number(true));
    if (lhs_number->type() == Error)
    {
      return lhs_number;
    }

    Node rhs_number = unwrap_arg(
      args,
      UnwrapOpt(1).type(Int).func("numbers.range_step").specify_number(true));
    if (rhs_number->type() == Error)
    {
      return rhs_number;
    }

    Node step_number = unwrap_arg(
      args,
      UnwrapOpt(2).type(Int).func("numbers.range_step").specify_number(true));
    if (step_number->type() == Error)
    {
      return step_number;
    }

    BigInt lhs = get_int(lhs_number);
    BigInt rhs = get_int(rhs_number);
    BigInt step = get_int(step_number);

    if (step < BigInt({"1"}))
    {
      return err(
        step_number,
        "numbers.range_step: step must be a positive number above zero",
        EvalBuiltInError);
    }

    Node array = Array ^ args[0];
    if (lhs < rhs)
    {
      BigInt curr = lhs;
      while (curr < rhs)
      {
        array->push_back(Term << (Scalar << (Int ^ curr.loc())));
        curr = curr + step;
      }

      if (curr == rhs)
      {
        array->push_back(Term << (Scalar << (Int ^ curr.loc())));
      }
    }
    else
    {
      BigInt curr = lhs;
      while (curr > rhs)
      {
        array->push_back(Term << (Scalar << (Int ^ curr.loc())));
        curr = curr - step;
      }

      if (curr == rhs)
      {
        array->push_back(Term << (Scalar << (Int ^ curr.loc())));
      }
    }

    return array;
  }

  const Node numbers_range_step_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "a")
                               << (bi::Description ^ "the start of the range")
                               << (bi::Type << bi::Number))
                   << (bi::Arg
                       << (bi::Name ^ "b")
                       << (bi::Description ^ "the end of the range (inclusive)")
                       << (bi::Type << bi::Number))
                   << (bi::Arg << (bi::Name ^ "step")
                               << (bi::Description ^
                                   "the step between numbers in the range")
                               << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "range")
                   << (bi::Description ^ "the range between `a` and `b`")
                   << (bi::Type
                       << (bi::DynamicArray << (bi::Type << bi::Number))));

  Node rand_intn(const Nodes& args)
  {
    Node seed_string_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("rand.intn"));
    if (seed_string_node->type() == Error)
    {
      return seed_string_node;
    }

    Node n_node = unwrap_arg(args, UnwrapOpt(1).type(Int).func("rand.intn"));
    if (n_node->type() == Error)
    {
      return n_node;
    }

    std::string seed_string = get_string(seed_string_node);
    std::size_t n = BigInt(n_node->location()).to_size();
    std::hash<std::string> hash;
    auto seed = static_cast<std::mt19937::result_type>(hash(seed_string));
    std::mt19937 rng(seed);
    std::size_t value = rng() % n;
    return Int ^ std::to_string(value);
  }

  const Node rand_intn_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "str")
                     << (bi::Description ^ "seed string for the random number")
                     << (bi::Type << bi::String))
                 << (bi::Arg << (bi::Name ^ "n")
                             << (bi::Description ^
                                 "upper bound of the random number (exclusive)")
                             << (bi::Type << bi::Number)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^
                                "random integer in the range `[0, abs(n))`")
                            << (bi::Type << bi::Number));

  Node plus_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Add), x, y);
  }

  const Node plus_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                               << (bi::Type << bi::Number))
                   << (bi::Arg << (bi::Name ^ "y") << bi::Description
                               << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "z")
                   << (bi::Description ^ "the sum of `x` and `y`")
                   << (bi::Type << bi::Number));

  Node minus_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float, Set}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float, Set}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Subtract), x, y);
  }

  const Node minus_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "x") << bi::Description
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type << bi::Number)
                             << (bi::Type
                                 << (bi::Set << (bi::Type << bi::Any))))))
                 << (bi::Arg
                     << (bi::Name ^ "y") << bi::Description
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type << bi::Number)
                             << (bi::Type
                                 << (bi::Set << (bi::Type << bi::Any)))))))
             << (bi::Result
                 << (bi::Name ^ "z")
                 << (bi::Description ^ "the difference of `x` and `y`")
                 << (bi::Type
                     << (bi::TypeSeq
                         << (bi::Type << bi::Number)
                         << (bi::Type << (bi::Set << (bi::Type << bi::Any))))));

  Node mul_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Multiply), x, y);
  }

  const Node mul_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                               << (bi::Type << bi::Number))
                   << (bi::Arg << (bi::Name ^ "y") << bi::Description
                               << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "z")
                   << (bi::Description ^ "the product of `x` and `y`")
                   << (bi::Type << bi::Number));

  Node div_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Divide), x, y);
  }

  const Node div_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "the dividend")
                               << (bi::Type << bi::Number))
                   << (bi::Arg << (bi::Name ^ "y")
                               << (bi::Description ^ "the divisor")
                               << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "z")
                   << (bi::Description ^ "the result of `x` divided by `y`")
                   << (bi::Type << bi::Number));

  Node rem_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int, Float}).message("Not a number"));
    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int, Float}).message("Not a number"));
    return Resolver::arithinfix(NodeDef::create(Modulo), x, y);
  }

  const Node rem_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x") << bi::Description
                               << (bi::Type << bi::Number))
                   << (bi::Arg << (bi::Name ^ "y") << bi::Description
                               << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "z") << (bi::Description ^ "the remainder")
                   << (bi::Type << bi::Number));

}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> numbers()
    {
      return {
        BuiltInDef::create(Location("plus"), plus_decl, plus_),
        BuiltInDef::create(Location("minus"), minus_decl, minus_),
        BuiltInDef::create(Location("mul"), mul_decl, mul_),
        BuiltInDef::create(Location("div"), div_decl, div_),
        BuiltInDef::create(Location("rem"), rem_decl, rem_),
        BuiltInDef::create(Location("abs"), abs_decl, abs_),
        BuiltInDef::create(Location("ceil"), ceil_decl, ceil_),
        BuiltInDef::create(Location("floor"), floor_decl, floor_),
        BuiltInDef::create(Location("round"), round_decl, round_),
        BuiltInDef::create(
          Location("numbers.range"), numbers_range_decl, numbers_range),
        BuiltInDef::create(
          Location("numbers.range_step"),
          numbers_range_step_decl,
          numbers_range_step),
        BuiltInDef::create(Location("rand.intn"), rand_intn_decl, rand_intn)};
    }
  }
}