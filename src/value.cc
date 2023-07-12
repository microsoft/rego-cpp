#include "value.h"

#include <sstream>

namespace rego
{
  ValueDef::ValueDef(const Node& value) : m_node(value), m_rank(0) {}

  ValueDef::ValueDef(const Location& var, const Node& value) : ValueDef(value)
  {
    m_var = var;
  }

  ValueDef::ValueDef(
    const Location& var, const Node& value, const Values& sources) :
    ValueDef(var, value)
  {
    m_sources = sources;
  }

  ValueDef::ValueDef(const RankedNode& value) :
    m_node(std::get<1>(value)), m_rank(std::get<0>(value))
  {}

  ValueDef::ValueDef(const Location& var, const RankedNode& value) :
    ValueDef(value)
  {
    m_var = var;
  }

  ValueDef::ValueDef(
    const Location& var, const RankedNode& value, const Values& sources) :
    ValueDef(var, value)
  {
    m_sources = sources;
  }

  Value ValueDef::copy_to(const Value& source, const Location& dest)
  {
    Values sources;
    if (source->m_var.len > 0)
    {
      sources.push_back(source);
    }
    return std::shared_ptr<ValueDef>(
      new ValueDef(dest, source->m_node, sources));
  }

  Value ValueDef::create(const Node& value)
  {
    return std::shared_ptr<ValueDef>(new ValueDef(value));
  }

  Value ValueDef::create(const Location& var, const Node& value)
  {
    return std::shared_ptr<ValueDef>(new ValueDef(var, value));
  }

  Value ValueDef::create(
    const Location& var, const Node& value, const Values& sources)
  {
    return std::shared_ptr<ValueDef>(new ValueDef(var, value, sources));
  }

  Value ValueDef::create(const RankedNode& value)
  {
    return std::shared_ptr<ValueDef>(new ValueDef(value));
  }

  Value ValueDef::create(const Location& var, const RankedNode& value)
  {
    return std::shared_ptr<ValueDef>(new ValueDef(var, value));
  }

  Value ValueDef::create(
    const Location& var, const RankedNode& value, const Values& sources)
  {
    return std::shared_ptr<ValueDef>(new ValueDef(var, value, sources));
  }

  bool ValueDef::merge_sources(const Value& other)
  {
    return merge_sources(other->m_sources);
  }

  bool ValueDef::merge_sources(const Values& others)
  {
    if (others.size() == 0)
    {
      return false;
    }

    Values all_sources;
    std::set_union(
      m_sources.begin(),
      m_sources.end(),
      others.begin(),
      others.end(),
      std::back_inserter(all_sources));
    if (all_sources.size() != m_sources.size())
    {
      m_sources = all_sources;
      return true;
    }

    return false;
  }

  bool ValueDef::depends_on(const Value& source) const
  {
    return std::find(m_sources.begin(), m_sources.end(), source) !=
      m_sources.end();
  }

  std::string ValueDef::json() const
  {
    return to_json(m_node);
  }

  std::string ValueDef::str() const
  {
    std::stringstream buf;
    buf << *this;
    return buf.str();
  }

  void ValueDef::to_string(std::ostream& os, const Location& root) const
  {
    if (m_var == root)
    {
      os << m_var.view();
      return;
    }

    os << m_var.view() << "(" << to_json(m_node) << ") -> " << m_sources.size()
       << "{";
    std::string sep = "";
    for (auto& source : m_sources)
    {
      os << sep;
      source->to_string(os, root);
      sep = ", ";
    }
    os << "}";
  }

  bool ValueDef::invalid() const
  {
    if (m_sources.size() == 0)
    {
      return m_invalid;
    }

    for (auto& source : m_sources)
    {
      if (source->invalid())
      {
        return true;
      }
    }

    return false;
  }

  void ValueDef::mark_as_valid()
  {
    if (m_sources.size() == 0)
    {
      m_invalid = false;
      return;
    }

    for (auto& source : m_sources)
    {
      source->mark_as_valid();
    }
  }

  void ValueDef::mark_as_invalid()
  {
    if (m_sources.size() == 0)
    {
      m_invalid = true;
      return;
    }

    for (auto& source : m_sources)
    {
      source->mark_as_invalid();
    }
  }

  std::int64_t ValueDef::rank() const
  {
    return m_rank;
  }

  const Location& ValueDef::var() const
  {
    return m_var;
  }

  const Node& ValueDef::node() const
  {
    return m_node;
  }

  const Values& ValueDef::sources() const
  {
    return m_sources;
  }

  Node ValueDef::to_term() const
  {
    if (
      m_node->type() == Term || m_node->type() == DefaultTerm ||
      m_node->type() == Error)
    {
      return m_node;
    }

    Node term = m_node;
    if (
      term->type() == JSONTrue || term->type() == JSONFalse ||
      term->type() == JSONInt || term->type() == JSONFloat ||
      term->type() == JSONString || term->type() == JSONNull)
    {
      return Term << (Scalar << term);
    }

    if (
      term->type() == Scalar || term->type() == Array ||
      term->type() == Object || term->type() == Set ||
      term->type() == Undefined)
    {
      return Term << term;
    }

    return err(term, "Not a term");
  }

  std::ostream& operator<<(std::ostream& os, const Value& value)
  {
    return os << *value;
  }

  std::ostream& operator<<(std::ostream& os, const ValueDef& value)
  {
    os << value.m_var.view() << "(" << value.json() << ") -> "
       << value.m_sources.size() << "{";
    std::string sep = "";
    for (auto& source : value.m_sources)
    {
      os << sep;
      source->to_string(os, value.m_var);
      sep = ", ";
    }
    return os << "}";
  }

  bool operator==(const Value& lhs, const Value& rhs)
  {
    return lhs->str() == rhs->str();
  }

  Values ValueDef::filter_by_rank(const Values& values)
  {
    Values filtered;
    std::int64_t min_index = std::numeric_limits<std::int64_t>::max();
    for (auto& value : values)
    {
      if (value->rank() == min_index)
      {
        filtered.push_back(value);
      }
      else if (value->rank() < min_index)
      {
        filtered.clear();
        filtered.push_back(value);
        min_index = value->rank();
      }
    }

    return filtered;
  }
}