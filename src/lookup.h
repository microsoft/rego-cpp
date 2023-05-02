#pragma once

#include <trieste/driver.h>

namespace rego
{
  using namespace trieste;

  Node search(const Node& lookup);
  bool can_replace(const NodeRange& n);
  bool exists(const NodeRange& n);
}