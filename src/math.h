#pragma once

#include "lang.h"

#include <cstdint>

namespace rego
{
  using namespace trieste;

  std::int64_t get_int(const Node& node);

  double get_double(const Node& node);

  bool get_bool(const Node& node);

  Node negate(const Node& node);

  Node math(const Node& op, std::int64_t lhs, std::int64_t rhs);

  Node math(const Node& op, double lhs, double rhs);

  Node compare(const Node& op, std::int64_t lhs, std::int64_t rhs);

  Node compare(const Node& op, double lhs, double rhs);

}