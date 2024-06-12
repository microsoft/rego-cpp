#include "unify.hh"

namespace
{
  using namespace rego;

  std::set<Token> scalar_tokens = {Int, Float, JSONString, True, False, Null};
  std::set<Token> term_tokens = {
    Scalar, Array, DynamicObject, Object, DynamicSet, Set, Undefined};
}

namespace rego
{
  ValueDef::ValueDef(
    const Location& var,
    const Node& value,
    const Values& sources,
    rank_t rank) :
    m_var(var),
    m_node(value),
    m_sources(sources),
    m_invalid(false),
    m_rank(rank)
  {
    m_json = to_key(m_node);
    std::ostringstream os;
    ValueDef::build_string(os, *this, m_var, true);
    m_str = os.str();
  }

  ValueDef::ValueDef(const Node& value) : ValueDef(Location(), value, {}, 0) {}

  ValueDef::ValueDef(const RankedNode& value) :
    ValueDef(Location(), std::get<1>(value), {}, std::get<0>(value))
  {}

  ValueDef::ValueDef(const Location& var, const RankedNode& value) :
    ValueDef(var, std::get<1>(value), {}, std::get<0>(value))
  {}

  ValueDef::ValueDef(const Location& var, const Node& value) :
    ValueDef(var, value, {}, 0)
  {}

  ValueDef::ValueDef(
    const Location& var, const RankedNode& value, const Values& sources) :
    ValueDef(var, std::get<1>(value), sources, std::get<0>(value))
  {}

  ValueDef::ValueDef(
    const Location& var, const Node& value, const Values& sources) :
    ValueDef(var, value, sources, 0)
  {}

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

  const std::string& ValueDef::json() const
  {
    return m_json;
  }

  const std::string& ValueDef::str() const
  {
    return m_str;
  }

  void ValueDef::build_string(
    std::ostream& os, const ValueDef& current, const Location& root, bool first)
  {
    if (!first)
    {
      if (current.m_var == root)
      {
        os << current.m_var.view();
      }
      else
      {
        os << current.m_str;
      }
      return;
    }

    os << current.m_var.view() << "(" << current.m_json << ") -> "
       << current.m_sources.size() << "{";
    join(
      os,
      current.m_sources.begin(),
      current.m_sources.end(),
      ", ",
      [root](std::ostream& stream, const Value& value) {
        build_string(stream, *value, root, false);
        return true;
      });
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
    if (m_node->type() == Term || m_node->type() == Error)
    {
      return m_node->clone();
    }

    Node term = m_node->clone();
    if (contains(scalar_tokens, term->type()))
    {
      return Term << (Scalar << term);
    }

    if (contains(term_tokens, term->type()))
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
    return os << value.m_str;
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
    return std::stoul(to_key(node));
  }

  void ValueDef::reduce_set()
  {
    if (m_node->type() == TermSet)
    {
      m_node = Resolver::reduce_termset(m_node);
    }
  }
}