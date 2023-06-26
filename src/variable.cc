#include "variable.h"

#include "log.h"
#include "resolver.h"

namespace {
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  inline const auto wfi =
      (Local <<= Var * (Val >>= Undefined))
    | (Term <<= Scalar | Array | Object | Set | Undefined)
    | (DefaultTerm <<= Scalar | Array | Object | Set | Undefined)
    | (Scalar <<= JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    ;
  // clang-format on
}

namespace rego
{
  Variable::Variable(const Node& local) :
    m_local(local),
    m_visited(false),
    m_initialized(false),
    m_dependency_score(1)
  {
    Location name = local->at(wfi / Local / Var)->location();
    std::string name_str = std::string(name.view());
    m_unify = name_str.starts_with("unify$");
    m_user_var = name_str.find('$') == std::string::npos ||
      name_str[0] == '$' || name_str.starts_with("value$");
  }

  bool Variable::depends_on(const Location& var) const
  {
    return m_dependencies.find(var) != m_dependencies.end();
  }

  void Variable::remove(const Value& value)
  {
    std::string key = to_json(value->node());
    m_values.erase(key);
  }

  std::string Variable::str() const
  {
    std::stringstream buf;
    buf << *this;
    return buf.str();
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

  std::ostream& operator<<(std::ostream& os, const Variable& variable)
  {
    return os << variable.m_local->at(wfi / Local / Var)->location().view() << "("
              << variable.m_dependency_score << ") = {" << variable.m_values;
  }

  std::ostream& operator<<(
    std::ostream& os, const std::map<Location, Variable>& variables)
  {
    os << "{";
    std::string sep = "";
    for (const auto& [loc, var] : variables)
    {
      os << sep << loc.view() << "(" << var.m_dependency_score << ") -> "
         << var.m_dependencies;
      sep = ", ";
    }
    os << "}";
    return os;
  }

  void Variable::mark_invalid_values()
  {
    m_values.mark_invalid_values();
  }

  void Variable::mark_valid_values()
  {
    // `false` and `undefined` are value values for everything
    // except a unification statement.
    m_values.mark_valid_values(!m_unify);
  }

  std::size_t Variable::compute_dependency_score(
    std::map<Location, Variable>& variables)
  {
    if (m_visited)
    {
      return m_dependency_score;
    }

    m_visited = false;
    m_dependency_score = std::transform_reduce(
      m_dependencies.cbegin(),
      m_dependencies.cend(),
      m_dependency_score,
      std::plus{},
      [&variables](auto& dep) {
        return variables.at(dep).compute_dependency_score(variables);
      });

    return m_dependency_score;
  }

  bool Variable::all_falsy() const
  {
    return m_values.all_falsy();
  }

  Node Variable::to_term() const
  {
    Nodes nodes = m_values.nodes();

    if (nodes.size() == 1)
    {
      if (nodes[0]->type() == DefaultTerm)
      {
        return Term << nodes[0]->at(wfi / DefaultTerm / DefaultTerm);
      }

      return nodes[0];
    }

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

      term_set->push_back(node);
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

  bool Variable::initialized() const
  {
    return m_initialized;
  }

  std::size_t Variable::dependency_score() const
  {
    return m_dependency_score;
  }

  const Node& Variable::local() const
  {
    return m_local;
  }

  const std::set<Location>& Variable::dependencies() const
  {
    return m_dependencies;
  }

  void Variable::compute_dependency_scores(
    std::map<Location, Variable>& variables)
  {
    for (auto& [_, variable] : variables)
    {
      variable.m_visited = false;
    }

    for (auto& [_, variable] : variables)
    {
      variable.compute_dependency_score(variables);
    }
  }

  std::size_t Variable::increase_dependency_score(std::size_t amount)
  {
    m_dependency_score += amount;
    return m_dependency_score;
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
      return JSONTrue;
    }

    Node term = to_term();
    if (term->type() == Error)
    {
      m_local->at(wfi / Local / Val) = term;
      return term;
    }

    if (Resolver::is_truthy(term) || m_user_var)
    {
      m_local->at(wfi / Local / Val) = term;
    }
    else
    {
      return Undefined;
    }

    return term;
  }

  Location Variable::name() const
  {
    return m_local->at(wfi / Local / Var)->location();
  }

  bool Variable::has_cycle(const std::map<Location, Variable>& variables) const
  {
    std::set<Location> visited;

    std::vector<Location> stack({name()});
    while (!stack.empty())
    {
      Location loc = stack.back();
      stack.pop_back();

      if (visited.contains(loc))
      {
        return true;
      }

      visited.insert(loc);

      const Variable& variable = variables.at(loc);
      for (const auto& dep : variable.dependencies())
      {
        stack.push_back(dep);
      }
    }

    return false;
  }

  std::size_t Variable::detect_cycles(std::map<Location, Variable>& variables)
  {
    std::size_t cycles = 0;
    for (auto& [_, variable] : variables)
    {
      if (variable.has_cycle(variables))
      {
        ++cycles;
      }
    }

    return cycles;
  }
}