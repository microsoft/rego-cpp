/**
 * Minimal vcpkg integration test for the rego-cpp library.
 * Verifies that find_package(regocpp) + regocpp::rego works and
 * that a simple query can be evaluated.
 */
#include <cstdio>
#include <rego/rego.hh>

int main()
{
  rego::Interpreter rego;
  rego.add_module("test", R"(package test
    allow := true
  )");
  auto result = rego.query("data.test.allow");
  if (result != "{\"expressions\":[true]}")
  {
    std::fprintf(stderr, "Unexpected result: '%s'\n", result.c_str());
    return 1;
  }
  std::printf("rego-cpp vcpkg integration test passed\n");
  return 0;
}
