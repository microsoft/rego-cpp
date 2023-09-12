#include "builtins.h"

namespace
{
  using namespace rego;

  Node and_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args,
      UnwrapOpt(0).types({Int}).func("bits.and").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(
      args,
      UnwrapOpt(1).types({Int}).func("bits.and").specify_number(true));
    if (y->type() == Error)
    {
      return y;
    }

    std::int64_t x_int = get_int(x).to_int();
    std::int64_t y_int = get_int(y).to_int();

    return Resolver::scalar(BigInt(x_int & y_int));
  }

  Node lsh(const Nodes& args)
  {
    Node x = unwrap_arg(
      args,
      UnwrapOpt(0).types({Int}).func("bits.lsh").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node s = unwrap_arg(
      args,
      UnwrapOpt(1).types({Int}).func("bits.lsh").specify_number(true));
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

  Node negate(const Nodes& args)
  {
    Node x = unwrap_arg(
      args,
      UnwrapOpt(0).types({Int}).func("bits.negate").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    std::int64_t x_int = get_int(x).to_int();

    return Resolver::scalar(BigInt(~x_int));
  }

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

  Node rsh(const Nodes& args)
  {
    Node x = unwrap_arg(
      args,
      UnwrapOpt(0).types({Int}).func("bits.rsh").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node s = unwrap_arg(
      args,
      UnwrapOpt(1).types({Int}).func("bits.rsh").specify_number(true));
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

  Node xor_(const Nodes& args)
  {
    Node x = unwrap_arg(
      args,
      UnwrapOpt(0).types({Int}).func("bits.xor").specify_number(true));
    if (x->type() == Error)
    {
      return x;
    }

    Node y = unwrap_arg(
      args,
      UnwrapOpt(1).types({Int}).func("bits.xor").specify_number(true));
    if (x->type() == Error)
    {}

    std::int64_t x_int = get_int(x).to_int();
    std::int64_t y_int = get_int(y).to_int();

    return Resolver::scalar(BigInt(x_int ^ y_int));
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> bits()
    {
      return {
        BuiltInDef::create(Location("bits.and"), 2, and_),
        BuiltInDef::create(Location("bits.lsh"), 2, lsh),
        BuiltInDef::create(Location("bits.negate"), 1, negate),
        BuiltInDef::create(Location("bits.or"), 2, or_),
        BuiltInDef::create(Location("bits.rsh"), 2, rsh),
        BuiltInDef::create(Location("bits.xor"), 2, xor_)};
    }
  }
}