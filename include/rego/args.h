// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "value.h"

#include <set>
#include <vector>

namespace rego
{
  /**
   * The Args class provides an interable interface over the Cartesian product
   * of all sets of arguments to a function. It will contain N sets of argument
   * sources, and will produce s_0 * s_1 * ... * s_n arguments, where s_i is the
   * size of the i-th argument source.
   */
  class Args
  {
  public:
    Args();
    void push_back_source(const Values& values);
    void mark_invalid(const std::set<Value>& active) const;
    void mark_invalid_except(const std::set<Value>& active) const;
    Values at(std::size_t index) const;
    std::size_t size() const;
    std::string str() const;
    std::size_t source_size() const;
    const Values& source_at(std::size_t index) const;
    Args subargs(std::size_t start) const;
    friend std::ostream& operator<<(std::ostream& os, const Args& args);

  private:
    std::vector<Values> m_values;
    std::vector<size_t> m_stride;
    std::size_t m_size;
  };
}