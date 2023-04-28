#pragma once

#include "lang.h"

namespace rego
{
  using namespace trieste;

  int get_int(const Node& node);

  double get_double(const Node& node);

  bool get_bool(const Node& node);

  Node math(const Node& op, int lhs, int rhs);

  Node math(const Node& op, double lhs, double rhs);

  Node compare(const Node& op, int lhs, int rhs);

  Node compare(const Node& op, double lhs, double rhs);

}