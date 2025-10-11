#include <rego/rego.hh>

using namespace rego;
namespace bi = rego::builtins;

Node add(const Nodes& args)
{
  Node a = unwrap_arg(args, UnwrapOpt(0).types({Int, Float}));
  if (a->type() == Error)
  {
    return a;
  }

  Node b = unwrap_arg(args, UnwrapOpt(1).types({Int, Float}));
  if (b->type() == Error)
  {
    return b;
  }

  if (a->type() == Int && b->type() == Int)
  {
    return scalar(get_int(a) + get_int(b));
  }

  return scalar(get_double(a) + get_double(b));
}

Node add_decl =
  bi::Decl << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "a") << bi::Description
                                      << (bi::Type << bi::Number))
                          << (bi::Arg << (bi::Name ^ "b") << bi::Description
                                      << (bi::Type << bi::Number)))
           << (bi::Result << (bi::Name ^ "result")
                          << (bi::Description ^ "The sum of `a` and `b`.")
                          << (bi::Type << bi::Number));

int main()
{
  Interpreter rego;
  rego.builtins()->register_builtin(
    bi::BuiltInDef::create(Location("myadd"), add_decl, add));
  logging::Output() << rego.query("myadd(2, 3)");
}