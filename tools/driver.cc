#include "rego.h"

int main(int argc, char** argv)
{
  auto builtins = rego::BuiltIns().register_standard_builtins();
  return rego::driver(builtins).run(argc, argv);
}
