#pragma once

#include "value.h"

#include <set>
#include <vector>

namespace rego
{
  class Args
  {
  public:
    Args();
    void push_back(const Values& values);
    void mark_invalid_except(const std::set<Value>& active) const;
    Values at(std::size_t index) const;
    std::size_t size() const;
    std::string str() const;
    const Values& source_at(std::size_t index) const;
    Args subargs(std::size_t start) const;
    friend std::ostream& operator<<(std::ostream& os, const Args& args);

  private:
    std::vector<Values> m_values;
    std::vector<size_t> m_stride;
    std::size_t m_size;
  };
}