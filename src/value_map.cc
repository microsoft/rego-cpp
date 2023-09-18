#include "internal.hh"

namespace rego
{
  ValueMap::ValueMap() {}

  ValueMap::ValueMap(
    const Values::const_iterator& begin, const Values::const_iterator& end)
  {
    for (auto it = begin; it != end; ++it)
    {
      insert(*it);
    }
  }

  void ValueMap::clear()
  {
    m_map.clear();
    m_keys.clear();
    m_values.clear();
  }

  bool ValueMap::contains(const Value& value) const
  {
    return rego::contains(
      m_values, std::make_pair(value->json(), value->str()));
  }

  bool ValueMap::intersect_with(const Values& values)
  {
    bool changed = false;
    for (auto& value : values)
    {
      auto key = value->json();
      if (m_keys.count(key) == 0)
      {
        value->mark_as_invalid();
        changed = true;
        continue;
      }

      if (insert(value))
      {
        changed = true;
      }
    }

    return changed;
  }

  bool ValueMap::remove_values_not_contained_in(const Values& values)
  {
    std::set<std::string> other;
    std::transform(
      values.begin(),
      values.end(),
      std::inserter(other, other.begin()),
      [](const Value& value) { return value->json(); });

    std::set<std::string> to_remove;

    bool changed = false;
    for (auto it = m_map.begin(); it != m_map.end(); ++it)
    {
      if (!rego::contains(other, it->first))
      {
        it->second->mark_as_invalid();
        to_remove.insert(it->first);
        changed = true;
      }
    }

    for (auto key : to_remove)
    {
      erase(key);
    }

    return changed;
  }

  bool ValueMap::insert(const Value& value)
  {
    auto key = value->json();
    if (contains(value))
    {
      return false;
    }

    m_map.insert({key, value});
    m_keys.insert(key);
    m_values.insert({key, value->str()});
    return true;
  }

  bool ValueMap::erase(const std::string& json)
  {
    if (m_keys.count(json) == 0)
    {
      return false;
    }

    auto begin = m_map.lower_bound(json);
    auto end = m_map.upper_bound(json);
    m_keys.erase(json);
    for (auto it = begin; it != end; ++it)
    {
      m_values.erase({it->first, it->second->str()});
    }

    m_map.erase(begin, end);

    return true;
  }

  bool ValueMap::empty() const
  {
    return m_map.empty();
  }

  Values ValueMap::valid_values() const
  {
    Values values;
    for (const auto& [_, value] : m_map)
    {
      if (!value->invalid())
      {
        values.push_back(value);
      }
    }

    return values;
  }

  Nodes ValueMap::to_terms() const
  {
    Values values = valid_values();
    std::sort(values.begin(), values.end());
    Nodes nodes;
    std::transform(
      values.begin(),
      values.end(),
      std::back_inserter(nodes),
      [](const auto& value) { return value->to_term(); });
    return nodes;
  }

  std::ostream& operator<<(std::ostream& os, const ValueMap& values)
  {
    os << "{";
    std::string sep = "";
    for (const auto& [key, value] : values.m_map)
    {
      os << sep;
      if (!value->invalid())
      {
        os << "*";
      }
      os << key;
      sep = ", ";
    }
    return os << "}";
  }

  bool ValueMap::remove_invalid_values()
  {
    bool changed = false;

    for (auto it = m_map.begin(); it != m_map.end();)
    {
      if (it->second->invalid())
      {
        m_values.erase({it->first, it->second->str()});
        it = m_map.erase(it);
        changed = true;
      }
      else
      {
        ++it;
      }
    }

    // Rebuild the keys
    m_keys.clear();
    std::transform(
      m_map.begin(),
      m_map.end(),
      std::inserter(m_keys, m_keys.begin()),
      [](const auto& pair) { return pair.first; });

    return changed;
  }

  void ValueMap::mark_invalid_values()
  {
    for (auto& [_, value] : m_map)
    {
      if (is_falsy(value->to_term()))
      {
        value->mark_as_invalid();
      }
    }
  }

  void ValueMap::mark_valid_values(bool include_falsy)
  {
    for (auto& [_, value] : m_map)
    {
      if (include_falsy || !is_falsy(value->node()))
      {
        value->mark_as_valid();
      }
    }
  }
}