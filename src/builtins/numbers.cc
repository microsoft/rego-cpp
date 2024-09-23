#include "builtins.h"

#include <numeric>

namespace
{
  using namespace rego;

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
      BigInt value = get_int(number);
      if (value.is_negative())
      {
        value = value.negate();
      }
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
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> numbers()
    {
      return {
        BuiltInDef::create(Location("abs"), 1, abs_),
        BuiltInDef::create(Location("ceil"), 1, ceil_),
        BuiltInDef::create(Location("floor"), 1, floor_),
        BuiltInDef::create(Location("round"), 1, round_),
        BuiltInDef::create(Location("numbers.range"), 2, numbers_range),
        BuiltInDef::create(
          Location("numbers.range_step"), 3, numbers_range_step),
        BuiltInDef::create(Location("rand.intn"), 2, rand_intn)};
    }
  }
}