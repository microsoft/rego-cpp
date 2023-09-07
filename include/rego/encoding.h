#pragma once

#include <trieste/source.h>

namespace rego
{
  std::string to_json(
    const trieste::Node& node, bool sort = false, bool rego_set = true);
}