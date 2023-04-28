#pragma once

#include <trieste/driver.h>

namespace rego
{
  using namespace trieste;

  Node singleton_value(const Nodes& defs);

  Node search(const Node& lookup);

  bool any_refs(NodeDef& n);

  bool can_replace(const NodeRange& n);
  bool exists(const NodeRange& n);
}