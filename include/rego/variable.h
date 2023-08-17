#pragma once

#include "value.h"
#include "value_map.h"

namespace rego
{
  class Variable
  {
  public:
    Variable(const Node& local, std::size_t id);
    bool unify(const Values& others);
    std::string str() const;
    bool remove_invalid_values();
    void mark_valid_values();
    Values valid_values() const;
    Node to_term() const;
    Node bind();
    void reset();

    bool is_unify() const;
    bool is_user_var() const;
    Location name() const;
    std::size_t id() const;

    static bool is_unify(const std::string_view& name);
    static bool is_user_var(const std::string_view& name);

    friend std::ostream& operator<<(std::ostream& os, const Variable& variable);
    friend std::ostream& operator<<(
      std::ostream& os, const std::map<Location, Variable>& variables);

  private:
    bool intersect_with(const Values& others);
    bool initialize(const Values& others);

    Node m_local;
    ValueMap m_values;
    bool m_unify;
    bool m_user_var;
    bool m_initialized;
    std::size_t m_id;
  };
}