#include <rego/rego.hh>

using namespace rego;

Node add(const Nodes& args)
{
  Node a = unwrap_arg(args, UnwrapOpt(0).types({JSONInt, JSONFloat}));
  if(a->type() == Error){
    return a;
  }

  Node b = unwrap_arg(args, UnwrapOpt(1).types({JSONInt, JSONFloat}));
  if(b->type() == Error){
    return b;
  }

  if(a->type() == JSONInt && b->type() == JSONInt){
    return scalar(get_int(a) + get_int(b));
  }

  return scalar(get_double(a) + get_double(b));
}

int main()
{
  Interpreter rego;
  rego.builtins().register_builtin(BuiltInDef::create(Location("myadd"), 2, add));
  std::cout << rego.query("myadd(2, 3)");
}