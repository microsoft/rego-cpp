#include "args.h"

#include "value.h"

#include <sstream>

namespace rego
{

  Args::Args() : m_size(0) {}

  void Args::push_back(const Values& values)
  {
    m_values.push_back(values);
    for (auto& stride : m_stride)
    {
      stride *= values.size();
    }
    m_stride.push_back(1);

    if (m_size == 0)
    {
      m_size = values.size();
    }
    else
    {
      m_size *= values.size();
    }
  }

  Values Args::at(std::size_t index) const
  {
    Values values;
    for (std::size_t i = 0; i < m_values.size(); ++i)
    {
      const Values& current = m_values[i];
      values.push_back(current[(index / m_stride[i])]);
      index %= m_stride[i];
    }
    return values;
  }

  std::size_t Args::size() const
  {
    return m_size;
  }

  std::string Args::str() const
  {
    std::stringstream buf;
    buf << *this;
    return buf.str();
  }

  std::ostream& operator<<(std::ostream& os, const Args& args)
  {
    return os << args.str();
  }

  void Args::mark_invalid(const std::set<Value>& active) const
  {
    for (auto& values : m_values)
    {
      for (auto& value : values)
      {
        if (active.find(value) == active.end())
        {
          value->mark_as_invalid();
        }
      }
    }
  }
}
