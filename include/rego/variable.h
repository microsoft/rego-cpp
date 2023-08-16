#pragma once

#include "value.h"
#include "value_map.h"

namespace rego
{
  class Variable
  {
  public:
    Variable(const Node& local);
    bool unify(const Values& others);
    std::string str() const;
    bool remove_invalid_values();
    void mark_invalid_values();
    void mark_valid_values();
    Values valid_values() const;
    std::size_t increase_dependency_score(std::size_t amount);
    Node to_term() const;
    const std::set<Location>& dependencies() const;
    Node bind();
    void reset();

    std::size_t dependency_score() const;
    Variable& dependency_score(std::size_t score);
    bool is_unify() const;
    bool is_user_var() const;
    Location name() const;

    static std::size_t detect_cycles(std::map<Location, Variable>& variables);
    static bool is_unify(const std::string_view& name);
    static bool is_user_var(const std::string_view& name);

    template <typename T>
    void insert_dependencies(T start, T end)
    {
      m_dependencies.insert(start, end);
    }

    friend std::ostream& operator<<(std::ostream& os, const Variable& variable);
    friend std::ostream& operator<<(
      std::ostream& os, const std::map<Location, Variable>& variables);

  private:
    bool intersect_with(const Values& others);
    bool initialize(const Values& others);
    bool has_cycle(const std::map<Location, Variable>& variables) const;

    Node m_local;
    std::set<Location> m_dependencies;
    ValueMap m_values;
    bool m_unify;
    bool m_user_var;
    bool m_initialized;
    std::size_t m_dependency_score;
  };
}