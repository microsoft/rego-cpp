#include <rego/rego.hh>
#include <trieste/driver.h>

int main(int argc, char** argv)
{
  auto builtins = rego::BuiltIns().register_standard_builtins();
  trieste::Driver driver(
    "rego", nullptr, rego::parser(), rego::passes(builtins));
  return driver.run(argc, argv);
}
