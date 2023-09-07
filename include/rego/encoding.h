#pragma once

#include <trieste/source.h>

namespace rego
{
  std::string to_json(
    const Node& node, bool sort = false, bool rego_set = true);
}