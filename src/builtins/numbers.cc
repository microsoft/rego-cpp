#include "register.h"
#include "resolver.h"

namespace
{
  using namespace rego;

  Node abs(const Nodes& args)
  {
    auto maybe_number = Resolver::maybe_unwrap_number(args[0]);
    if (!maybe_number.has_value())
    {
      return err(args[0], "Not a number");
    }

    Node number = maybe_number.value();
    if (number->type() == JSONInt)
    {
      BigInt value = Resolver::get_int(number);
      if (value.is_negative())
      {
        value = value.negate();
      }
      return JSONInt ^ value.loc();
    }
    else
    {
      double value = Resolver::get_double(number);
      if (value < 0)
      {
        value *= -1;
      }
      return JSONFloat ^ std::to_string(value);
    }
  }

  Node ceil(const Nodes& args)
  {
    auto maybe_number = Resolver::maybe_unwrap_number(args[0]);
    if (!maybe_number.has_value())
    {
      return err(args[0], "Not a number");
    }

    Node number = maybe_number.value();
    if (number->type() == JSONInt)
    {
      return number;
    }
    else
    {
      BigInt value(
        static_cast<std::int64_t>(std::ceil(Resolver::get_double(number))));
      return JSONInt ^ value.loc();
    }
  }

  Node floor(const Nodes& args)
  {
    auto maybe_number = Resolver::maybe_unwrap_number(args[0]);
    if (!maybe_number.has_value())
    {
      return err(args[0], "Not a number");
    }

    Node number = maybe_number.value();
    if (number->type() == JSONInt)
    {
      return number;
    }
    else
    {
      BigInt value(
        static_cast<std::int64_t>(std::floor(Resolver::get_double(number))));
      return JSONInt ^ value.loc();
    }
  }

  Node round(const Nodes& args)
  {
    auto maybe_number = Resolver::maybe_unwrap_number(args[0]);
    if (!maybe_number.has_value())
    {
      return err(args[0], "Not a number");
    }

    Node number = maybe_number.value();
    if (number->type() == JSONInt)
    {
      return number;
    }
    else
    {
      BigInt value(
        static_cast<std::int64_t>(std::round(Resolver::get_double(number))));
      return JSONInt ^ value.loc();
    }
  }

  Node numbers_range(const Nodes& args)
  {
    auto maybe_lhs_number = Resolver::maybe_unwrap_number(args[0]);
    if (!maybe_lhs_number.has_value())
    {
      return err(args[0], "Not a number");
    }
    auto maybe_rhs_number = Resolver::maybe_unwrap_number(args[1]);
    if (!maybe_rhs_number.has_value())
    {
      return err(args[1], "Not a number");
    }

    Node lhs_number = maybe_lhs_number.value();
    Node rhs_number = maybe_rhs_number.value();
    if (lhs_number->type() != JSONInt)
    {
      return err(
        args[0],
        "numbers.range: operand 1 must be integer number but got "
        "floating-point number",
        "eval_type_error");
    }

    if (rhs_number->type() != JSONInt)
    {
      return err(
        args[1],
        "numbers.range: operand 2 must be integer number but got "
        "floating-point number",
        "eval_type_error");
    }

    BigInt lhs = Resolver::get_int(lhs_number);
    BigInt rhs = Resolver::get_int(rhs_number);
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
    auto maybe_seed_string = Resolver::maybe_unwrap_string(args[0]);
    if (!maybe_seed_string.has_value())
    {
      return err(args[0], "Not a string");
    }

    auto maybe_n_number = Resolver::maybe_unwrap_number(args[1]);
    if (!maybe_n_number.has_value())
    {
      return err(args[1], "Not an number");
    }

    std::string seed_string =
      strip_quotes(std::string(maybe_seed_string.value()->location().view()));
    std::size_t n = BigInt(args[1]->location()).to_size();
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