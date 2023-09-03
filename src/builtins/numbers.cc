#include "errors.h"
#include "helpers.h"
#include "register.h"

#include <numeric>

namespace
{
  using namespace rego;

  Node abs(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({JSONInt, JSONFloat}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == JSONInt)
    {
      BigInt value = get_int(number);
      if (value.is_negative())
      {
        value = value.negate();
      }
      return JSONInt ^ value.loc();
    }
    else
    {
      double value = get_double(number);
      if (value < 0)
      {
        value *= -1;
      }
      return JSONFloat ^ std::to_string(value);
    }
  }

  Node ceil(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({JSONInt, JSONFloat}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == JSONInt)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::ceil(get_double(number))));
      return JSONInt ^ value.loc();
    }
  }

  Node floor(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({JSONInt, JSONFloat}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == JSONInt)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::floor(get_double(number))));
      return JSONInt ^ value.loc();
    }
  }

  Node round(const Nodes& args)
  {
    Node number = unwrap_arg(
      args, UnwrapOpt(0).types({JSONInt, JSONFloat}).message("Not a number"));
    if (number->type() == Error)
    {
      return number;
    }

    if (number->type() == JSONInt)
    {
      return number;
    }
    else
    {
      BigInt value(static_cast<std::int64_t>(std::round(get_double(number))));
      return JSONInt ^ value.loc();
    }
  }

  Node numbers_range(const Nodes& args)
  {
    Node lhs_number = unwrap_arg(
      args,
      UnwrapOpt(0).type(JSONInt).func("numbers.range").specify_number(true));
    if (lhs_number->type() == Error)
    {
      return lhs_number;
    }

    Node rhs_number = unwrap_arg(
      args,
      UnwrapOpt(1).type(JSONInt).func("numbers.range").specify_number(true));
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
        array->push_back(Term << (Scalar << (JSONInt ^ curr.loc())));
        curr = curr.increment();
      }
      array->push_back(Term << (Scalar << (JSONInt ^ curr.loc())));
    }
    else
    {
      BigInt curr = lhs;
      while (curr > rhs)
      {
        array->push_back(Term << (Scalar << (JSONInt ^ curr.loc())));
        curr = curr.decrement();
      }
      array->push_back(Term << (Scalar << (JSONInt ^ curr.loc())));
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

    Node n_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONInt).func("rand.intn"));
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
    return JSONInt ^ std::to_string(value);
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> numbers()
    {
      return {
        BuiltInDef::create(Location("abs"), 1, abs),
        BuiltInDef::create(Location("ceil"), 1, ceil),
        BuiltInDef::create(Location("floor"), 1, floor),
        BuiltInDef::create(Location("round"), 1, round),
        BuiltInDef::create(Location("numbers.range"), 2, numbers_range),
        BuiltInDef::create(Location("rand.intn"), 2, rand_intn)};
    }
  }
}