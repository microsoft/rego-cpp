#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node range(const Nodes& args)
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

  BuiltIn range_factory()
  {
    const Node range_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "a")
                                 << (bi::Description ^ "the start of the range")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "b")
                                 << (bi::Description ^
                                     "the end of the range (inclusive)")
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "range")
                     << (bi::Description ^ "the range between `a` and `b`")
                     << (bi::Type
                         << (bi::DynamicArray << (bi::Type << bi::Number))));
    return BuiltInDef::create({"numbers.range"}, range_decl, range);
  }

  Node range_step(const Nodes& args)
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

  BuiltIn range_step_factory()
  {
    const Node range_step_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "a")
                                 << (bi::Description ^ "the start of the range")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "b")
                                 << (bi::Description ^
                                     "the end of the range (inclusive)")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "step")
                                 << (bi::Description ^
                                     "the step between numbers in the range")
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "range")
                     << (bi::Description ^ "the range between `a` and `b`")
                     << (bi::Type
                         << (bi::DynamicArray << (bi::Type << bi::Number))));
    return BuiltInDef::create(
      {"numbers.range_step"}, range_step_decl, range_step);
  }

  namespace rand
  {
    Node intn(const Nodes& args)
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

      auto maybe_n = BigInt(n_node->location()).to_size();
      if (!maybe_n.has_value())
      {
        return err(
          n_node,
          "rand.intn: upper bound is not a valid integer",
          EvalBuiltInError);
      }
      std::size_t n = maybe_n.value();

      std::hash<std::string> hash;
      auto seed = static_cast<std::mt19937::result_type>(hash(seed_string));
      std::mt19937 rng(seed);
      std::size_t value = rng() % n;
      return Int ^ std::to_string(value);
    }

    BuiltIn intn_factory()
    {
      const Node intn_decl = bi::Decl
        << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "str")
                                   << (bi::Description ^
                                       "seed string for the random number")
                                   << (bi::Type << bi::String))
                       << (bi::Arg
                           << (bi::Name ^ "n")
                           << (bi::Description ^
                               "upper bound of the random number (exclusive)")
                           << (bi::Type << bi::Number)))
        << (bi::Result << (bi::Name ^ "y")
                       << (bi::Description ^
                           "random integer in the range `[0, abs(n))`")
                       << (bi::Type << bi::Number));
      return BuiltInDef::create({"rand.intn"}, intn_decl, intn);
    }
  } // namespace rand
}

namespace rego
{
  namespace builtins
  {
    BuiltIn numbers(const Location& name)
    {
      std::string_view view = name.view().substr(8); // skip "numbers."
      if (view == "range")
      {
        return range_factory();
      }
      if (view == "range_step")
      {
        return range_step_factory();
      }

      return nullptr;
    }

    BuiltIn rand(const Location& name)
    {
      std::string_view view = name.view().substr(5); // skip "rand."
      if (view == "intn")
      {
        return rand::intn_factory();
      }

      return nullptr;
    }
  }
}