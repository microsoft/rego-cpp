// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "encoding.h"
#include "interpreter.h"
#include "log.h"
#include "tokens.h"

#include <trieste/driver.h>

namespace rego
{
  using namespace trieste;

  Parse parser();
  Driver& driver(const BuiltIns& builtins);
  using PassCheck = std::tuple<std::string, Pass, const wf::Wellformed*>;
  std::vector<PassCheck> passes(const BuiltIns& builtins);
  Node version();
}