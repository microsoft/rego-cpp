#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node and_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).type(Int).func("bits.and").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(
      args, UnwrapOpt(1).type(Int).func("bits.and").specify_number(true));
    if (y->type() == Error)
    {
      return y;
    }

    auto maybe_x_int = get_int(x).to_int();
    if (!maybe_x_int.has_value())
    {
      return err(
        x, "bits.and: first operand is not a valid integer", EvalTypeError);
    }
    std::int64_t x_int = maybe_x_int.value();

    auto maybe_y_int = get_int(y).to_int();
    if (!maybe_y_int.has_value())
    {
      return err(
        y, "bits.and: second operand is not a valid integer", EvalTypeError);
    }
    std::int64_t y_int = maybe_y_int.value();

    return Resolver::scalar(BigInt(x_int & y_int));
  }

  BuiltIn and_factory()
  {
    const Node and_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the first integer")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "y")
                                 << (bi::Description ^ "the second integer")
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the bitwise AND of `x` and `y`")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"bits.and"}, and_decl, and_);
  }

  Node lsh(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).type(Int).func("bits.lsh").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node s = unwrap_arg(
      args, UnwrapOpt(1).type(Int).func("bits.lsh").specify_number(true));
    if (s->type() == Error)
    {
      return s;
    }

    BigInt x_int = get_int(x);

    auto maybe_s_int = get_int(s).to_int();
    if (!maybe_s_int.has_value())
    {
      return err(
        s, "bits.lsh: second operand is not a valid integer", EvalTypeError);
    }
    std::int64_t s_int = maybe_s_int.value();

    if (s_int < 0)
    {
      return err(
        s,
        "bits.lsh: operand 2 must be an unsigned integer number but got a "
        "negative integer",
        EvalTypeError);
    }

    const std::size_t scale = static_cast<std::size_t>(1) << s_int;

    return Resolver::scalar(x_int * scale);
  }

  BuiltIn lsh_factory()
  {
    const Node lsh_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the integer to shift")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg
                         << (bi::Name ^ "s")
                         << (bi::Description ^ "the number of bits to shift")
                         << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^
                         "the result of shifting `x` `s` bits to the left")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"bits.lsh"}, lsh_decl, lsh);
  }

  Node negate(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).type(Int).func("bits.negate").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    auto maybe_x_int = get_int(x).to_int();
    if (!maybe_x_int.has_value())
    {
      return err(
        x, "bits.negate: operand is not a valid integer", EvalTypeError);
    }
    std::int64_t x_int = maybe_x_int.value();

    return Resolver::scalar(BigInt(~x_int));
  }

  BuiltIn negate_factory()
  {
    const Node negate_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "the integer to negate")
                               << (bi::Type << bi::Number)))
               << (bi::Result
                   << (bi::Name ^ "z")
                   << (bi::Description ^ "the bitwise negation of `x`")
                   << (bi::Type << bi::Number));
    return BuiltInDef::create({"bits.negate"}, negate_decl, negate);
  }

  Node or_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).type(Int).func("bits.or").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(
      args, UnwrapOpt(1).type(Int).func("bits.or").specify_number(true));
    if (y->type() == Error)
    {
      return y;
    }

    auto maybe_x_int = get_int(x).to_int();
    if (!maybe_x_int.has_value())
    {
      return err(
        x, "bits.or: first operand is not a valid integer", EvalTypeError);
    }
    std::int64_t x_int = maybe_x_int.value();

    auto maybe_y_int = get_int(y).to_int();
    if (!maybe_y_int.has_value())
    {
      return err(
        y, "bits.or: second operand is not a valid integer", EvalTypeError);
    }
    std::int64_t y_int = maybe_y_int.value();

    return Resolver::scalar(BigInt(x_int | y_int));
  }

  BuiltIn or_factory()
  {
    const Node or_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the first integer")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "y")
                                 << (bi::Description ^ "the second integer")
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the bitwise OR of `x` and `y`")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"bits.or"}, or_decl, or_);
  }

  Node rsh(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).type(Int).func("bits.rsh").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node s = unwrap_arg(
      args, UnwrapOpt(1).type(Int).func("bits.rsh").specify_number(true));
    if (s->type() == Error)
    {
      return s;
    }

    auto maybe_x_int = get_int(x).to_int();
    if (!maybe_x_int.has_value())
    {
      return err(
        x, "bits.rsh: first operand is not a valid integer", EvalTypeError);
    }
    std::int64_t x_int = maybe_x_int.value();

    auto maybe_s_int = get_int(s).to_int();
    if (!maybe_s_int.has_value())
    {
      return err(
        s, "bits.rsh: second operand is not a valid integer", EvalTypeError);
    }
    std::int64_t s_int = maybe_s_int.value();

    if (s_int < 0)
    {
      return err(
        s,
        "bits.rsh: operand 2 must be an unsigned integer number but got a "
        "negative integer",
        EvalTypeError);
    }

    return Resolver::scalar(BigInt(x_int >> s_int));
  }

  BuiltIn rsh_factory()
  {
    const Node rsh_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the integer to shift")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg
                         << (bi::Name ^ "s")
                         << (bi::Description ^ "the number of bits to shift")
                         << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^
                         "the result of shifting `x` `s` bits to the right")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"bits.rsh"}, rsh_decl, rsh);
  }

  Node xor_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).type(Int).func("bits.xor").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(
      args, UnwrapOpt(1).type(Int).func("bits.xor").specify_number(true));
    if (y->type() == Error)
    {
      return y;
    }

    auto maybe_x_int = get_int(x).to_int();
    if (!maybe_x_int.has_value())
    {
      return err(
        x, "bits.xor: first operand is not a valid integer", EvalTypeError);
    }
    std::int64_t x_int = maybe_x_int.value();

    auto maybe_y_int = get_int(y).to_int();
    if (!maybe_y_int.has_value())
    {
      return err(
        y, "bits.xor: second operand is not a valid integer", EvalTypeError);
    }
    std::int64_t y_int = maybe_y_int.value();

    return Resolver::scalar(BigInt(x_int ^ y_int));
  }

  BuiltIn xor_factory()
  {
    const Node xor_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "x")
                                 << (bi::Description ^ "the first integer")
                                 << (bi::Type << bi::Number))
                     << (bi::Arg << (bi::Name ^ "y")
                                 << (bi::Description ^ "the second integer")
                                 << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "z")
                     << (bi::Description ^ "the bitwise XOR of `x` and `y`")
                     << (bi::Type << bi::Number));
    return BuiltInDef::create({"bits.xor"}, xor_decl, xor_);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn bits(const Location& name)
    {
      assert(name.view().starts_with("bits."));
      std::string_view view = name.view().substr(5); // skip "bits."
      if (view == "and")
      {
        return and_factory();
      }
      if (view == "lsh")
      {
        return lsh_factory();
      }
      if (view == "negate")
      {
        return negate_factory();
      }
      if (view == "or")
      {
        return or_factory();
      }
      if (view == "rsh")
      {
        return rsh_factory();
      }
      if (view == "xor")
      {
        return xor_factory();
      }

      return nullptr;
    }
  }
}