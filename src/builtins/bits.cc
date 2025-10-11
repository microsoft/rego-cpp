#include "builtins.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node and_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int}).func("bits.and").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int}).func("bits.and").specify_number(true));
    if (y->type() == Error)
    {
      return y;
    }

    std::int64_t x_int = get_int(x).to_int();
    std::int64_t y_int = get_int(y).to_int();

    return Resolver::scalar(BigInt(x_int & y_int));
  }

  Node and_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the first integer")
                             << (bi::Type << bi::Number))
                 << (bi::Arg << (bi::Name ^ "y")
                             << (bi::Description ^ "the second integer")
                             << (bi::Type << bi::Number)))
             << (bi::Result
                 << (bi::Name ^ "z")
                 << (bi::Description ^ "the bitwise AND of `x` and `y`")
                 << (bi::Type << bi::Number));

  Node lsh(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int}).func("bits.lsh").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node s = unwrap_arg(
      args, UnwrapOpt(1).types({Int}).func("bits.lsh").specify_number(true));
    if (s->type() == Error)
    {
      return s;
    }

    BigInt x_int = get_int(x);
    std::int64_t s_int = get_int(s).to_int();

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

  Node lsh_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the integer to shift")
                             << (bi::Type << bi::Number))
                 << (bi::Arg
                     << (bi::Name ^ "s")
                     << (bi::Description ^ "the number of bits to shift")
                     << (bi::Type << bi::Number)))
             << (bi::Result
                 << (bi::Name ^ "z")
                 << (bi::Description ^
                     "the result of shifting `x` `s` bits to the left")
                 << (bi::Type << bi::Number));

  Node negate(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int}).func("bits.negate").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    std::int64_t x_int = get_int(x).to_int();

    return Resolver::scalar(BigInt(~x_int));
  }

  Node negate_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the integer to negate")
                             << (bi::Type << bi::Number)))
             << (bi::Result << (bi::Name ^ "z")
                            << (bi::Description ^ "the bitwise negation of `x`")
                            << (bi::Type << bi::Number));

  Node or_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int}).func("bits.or").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int}).func("bits.or").specify_number(true));
    if (y->type() == Error)
    {
      return y;
    }

    std::int64_t x_int = get_int(x).to_int();
    std::int64_t y_int = get_int(y).to_int();

    return Resolver::scalar(BigInt(x_int | y_int));
  }

  Node or_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the first integer")
                             << (bi::Type << bi::Number))
                 << (bi::Arg << (bi::Name ^ "y")
                             << (bi::Description ^ "the second integer")
                             << (bi::Type << bi::Number)))
             << (bi::Result
                 << (bi::Name ^ "z")
                 << (bi::Description ^ "the bitwise OR of `x` and `y`")
                 << (bi::Type << bi::Number));

  Node rsh(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int}).func("bits.rsh").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node s = unwrap_arg(
      args, UnwrapOpt(1).types({Int}).func("bits.rsh").specify_number(true));
    if (s->type() == Error)
    {
      return s;
    }

    std::int64_t x_int = get_int(x).to_int();
    std::int64_t s_int = get_int(s).to_int();

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

  Node rsh_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the integer to shift")
                             << (bi::Type << bi::Number))
                 << (bi::Arg
                     << (bi::Name ^ "s")
                     << (bi::Description ^ "the number of bits to shift")
                     << (bi::Type << bi::Number)))
             << (bi::Result
                 << (bi::Name ^ "z")
                 << (bi::Description ^
                     "the result of shifting `x` `s` bits to the right")
                 << (bi::Type << bi::Number));

  Node xor_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args, UnwrapOpt(0).types({Int}).func("bits.xor").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(
      args, UnwrapOpt(1).types({Int}).func("bits.xor").specify_number(true));
    if (y->type() == Error)
    {
      return y;
    }

    std::int64_t x_int = get_int(x).to_int();
    std::int64_t y_int = get_int(y).to_int();

    return Resolver::scalar(BigInt(x_int ^ y_int));
  }

  Node xor_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the first integer")
                             << (bi::Type << bi::Number))
                 << (bi::Arg << (bi::Name ^ "y")
                             << (bi::Description ^ "the second integer")
                             << (bi::Type << bi::Number)))
             << (bi::Result
                 << (bi::Name ^ "z")
                 << (bi::Description ^ "the bitwise XOR of `x` and `y`")
                 << (bi::Type << bi::Number));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> bits()
    {
      return {
        BuiltInDef::create(Location("bits.and"), and_decl, and_),
        BuiltInDef::create(Location("bits.lsh"), lsh_decl, lsh),
        BuiltInDef::create(Location("bits.negate"), negate_decl, negate),
        BuiltInDef::create(Location("bits.or"), or_decl, or_),
        BuiltInDef::create(Location("bits.rsh"), rsh_decl, rsh),
        BuiltInDef::create(Location("bits.xor"), xor_decl, xor_)};
    }
  }
}