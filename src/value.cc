#include "value.h"

#include "errors.h"
#include "utils.h"
#include "resolver.h"

#include <sstream>

namespace
{
  using namespace rego;

  std::set<Token> scalar_tokens = {
    JSONInt, JSONFloat, JSONString, JSONTrue, JSONFalse, JSONNull};
  std::set<Token> term_tokens = {Scalar, Array, Object, Set, Undefined};
}

namespace rego
{
  ValueDef::ValueDef(const Node& value) :
    m_node(value), m_invalid(false), m_rank(0)
  {}

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
    std::ostringstream buf;
    buf << *this;
    return buf.str();
  }

  void ValueDef::to_string(
    std::ostream& os, const Location& root, bool first) const
  {
    if (m_var == root && !first)
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
      source->to_string(os, root, false);
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

  rank_t ValueDef::rank() const
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
      m_node->type() == Term ||
      m_node->type() == Error)
    {
      return m_node;
    }

    Node term = m_node;
    if (scalar_tokens.contains(term->type()))
    {
      return Term << (Scalar << term);
    }

    if (term_tokens.contains(term->type()))
    {
      return Term << term;
    }

    if (term->type() == TermSet)
    {
      return term;
    }

    return err(term, "Not a term");
  }

  std::ostream& operator<<(std::ostream& os, const Value& value)
  {
    return os << *value;
  }

  std::ostream& operator<<(std::ostream& os, const ValueDef& value)
  {
    value.to_string(os, value.m_var, true);
    return os;
  }

  bool operator==(const Value& lhs, const Value& rhs)
  {
    return lhs->str() == rhs->str();
  }

  bool operator<(const Value& lhs, const Value& rhs)
  {
    if (lhs->m_sources.size() > 0 && rhs->m_sources.size() > 0)
    {
      if (lhs->m_sources[0]->m_var == rhs->m_sources[0]->m_var)
      {
        return lhs->m_sources[0] < rhs->m_sources[0];
      }
    }
    return lhs->str() < rhs->str();
  }

  Values ValueDef::filter_by_rank(const Values& values)
  {
    Values filtered;
    rank_t min_index = std::numeric_limits<rank_t>::max();
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

  rank_t ValueDef::get_rank(const Node& node)
  {
    return std::stoul(to_json(node));
  }
}