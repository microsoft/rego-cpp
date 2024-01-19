#include "trieste/driver.h"

#include "rego_test.h"

int main(int argc, char** argv)
{
  trieste::Driver driver(
    "rego_test", nullptr, rego_test::parser(), rego_test::passes());
  return driver.run(argc, argv);
}
