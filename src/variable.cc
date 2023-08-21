#include "variable.h"

#include "log.h"
#include "resolver.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  inline const auto wfi =
      (Local <<= Var * (Val >>= Undefined | Term))
    | (Term <<= Scalar | Array | Object | Set | Undefined)
    | (DefaultTerm <<= Scalar | Array | Object | Set | Undefined)
    | (Scalar <<= JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    ;
  // clang-format on
}

namespace rego
{
  Variable::Variable(const Node& local, std::size_t id) :
    m_local(local), m_initialized(false), m_id(id)
  {
    Location name = (wfi / local / Var)->location();
    m_unify = is_unify(name.view());
    m_user_var = is_user_var(name.view());
  }

  bool Variable::is_unify(const std::string_view& name)
  {
    return name.starts_with("unify$");
  }

  bool Variable::is_user_var(const std::string_view& name)
  {
    if (name.starts_with("__") && name.ends_with("__"))
    {
      // OPA test local variables use this convention
      return true;
    }

    return name.find('$') == std::string::npos || name[0] == '$' ||
      name.starts_with("value$") || name.starts_with("out$");
  }

  std::string Variable::str() const
  {
    std::ostringstream buf;
    buf << *this;
    return buf.str();
  }

  void Variable::reset()
  {
    m_values.clear();
    m_initialized = false;
  }

  bool Variable::initialize(const Values& others)
  {
    bool changed = false;
    for (auto& value : others)
    {
      if (value->node()->type() != Undefined)
      {
        m_initialized = true;
      }

      if (m_values.insert(value))
      {
        changed = true;
      }
    }

    return changed;
  }

  bool Variable::unify(const Values& others)
  {
    bool changed;
    if (m_initialized)
    {
      changed = intersect_with(others);
    }
    else
    {
      changed = initialize(others);
    }
    mark_valid_values();
    return changed;
  }

  std::ostream& operator<<(std::ostream& os, const Variable& variable)
  {
    return os << (wfi / variable.m_local / Var)->location().view() << " = "
              << variable.m_values;
  }

  void Variable::mark_valid_values()
  {
    // `false` and `undefined` are value values for everything
    // except a unification statement.
    m_values.mark_valid_values(!m_unify);
  }

  Node Variable::to_term() const
  {
    Nodes nodes = m_values.nodes();

    if (nodes.size() == 1)
    {
      if (nodes[0]->type() == DefaultTerm)
      {
        return Term << nodes[0]->front();
      }

      return nodes[0];
    }

    std::set<std::string> values;
    Node term_set = NodeDef::create(TermSet);
    for (const auto& node : nodes)
    {
      if (node->type() == Error)
      {
        return node;
      }

      if (node->type() == DefaultTerm || node->type() == Undefined)
      {
        continue;
      }

      std::string json = to_json(node);
      if (!values.contains(json))
      {
        values.insert(json);
        term_set->push_back(node);
      }
    }

    if (term_set->size() == 1)
    {
      return term_set->front();
    }

    return term_set;
  }

  bool Variable::is_unify() const
  {
    return m_unify;
  }

  bool Variable::is_user_var() const
  {
    return m_user_var;
  }

  Values Variable::valid_values() const
  {
    return m_values.valid_values();
  }

  bool Variable::remove_invalid_values()
  {
    return m_values.remove_invalid_values();
  }

  bool Variable::intersect_with(const Values& others)
  {
    bool changed = false;
    if (m_values.intersect_with(others))
    {
      changed = true;
    }

    if (m_values.remove_values_not_contained_in(others))
    {
      changed = true;
    }

    return changed;
  }

  Node Variable::bind()
  {
    if (!m_unify && !m_user_var)
    {
      return JSONTrue ^ "true";
    }

    Node term = to_term();
    if (term->type() == Error)
    {
      m_local->back() = term;
      return term;
    }

    if (term->type() == TermSet && term->size() == 0)
    {
      if (term->size() == 0)
      {
        m_local->back() = Undefined;
        return Undefined;
      }
    }

    m_local->back() = term;
    return term;
  }

  Location Variable::name() const
  {
    return (wfi / m_local / Var)->location();
  }

  std::size_t Variable::id() const
  {
    return m_id;
  }
}