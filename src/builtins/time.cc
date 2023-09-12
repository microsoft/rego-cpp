#include "builtins.h"

#include <chrono>

namespace
{
  using namespace rego;
  using namespace std::chrono;

  Node now_ns(const Nodes&)
  {
    auto now = high_resolution_clock::now().time_since_epoch();
    auto now_ns = duration_cast<nanoseconds>(now).count();

    return Int ^ std::to_string(now_ns);
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> time()
    {
      return {BuiltInDef::create(Location("time.now_ns"), 0, now_ns)};
    }
  }
}