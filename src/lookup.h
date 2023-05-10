#pragma once

#include <trieste/driver.h>

namespace rego
{
  using namespace trieste;

  bool any_refs(const Node& n);
  Node search(const Node& lookup);
  bool can_replace(const NodeRange& n);
  bool exists(const NodeRange& n);
  bool can_remove(const NodeRange& rule);
}