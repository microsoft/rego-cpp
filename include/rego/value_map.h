#pragma once

#include "value.h"

#include <map>
#include <set>
#include <string>

namespace rego
{
  class ValueMap
  {
  public:
    ValueMap();
    ValueMap(
      const Values::const_iterator& begin, const Values::const_iterator& end);
    bool contains(const Value& value) const;
    bool intersect_with(const Values& values);
    bool remove_values_not_contained_in(const Values& values);
    void clear();
    bool insert(const Value& value);
    bool erase(const std::string& json);
    bool empty() const;
    Nodes nodes() const;
    Values valid_values() const;
    bool remove_invalid_values();
    void mark_valid_values(bool include_falsy);
    void mark_invalid_values();

    friend std::ostream& operator<<(std::ostream& os, const ValueMap& values);

  private:
    std::multimap<std::string, Value> m_map;
    std::set<std::pair<std::string, std::string>> m_values;
    std::set<std::string> m_keys;
  };
}