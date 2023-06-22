#pragma once

#include "lang.h"

#include <trieste/driver.h>

namespace rego
{
  class ValueDef;
  using Value = std::shared_ptr<ValueDef>;
  using Values = std::vector<Value>;

  class ValueDef
  {
  public:
    bool merge_sources(const Values& others);
    bool merge_sources(const Value& other);
    void mark_as_valid();
    void mark_as_invalid();

    std::string str() const;
    std::string json() const;
    bool depends_on(const Value& value) const;
    bool invalid() const;
    const Location& var() const;
    const Node& node() const;
    const Values& sources() const;
    Node to_term() const;
    friend std::ostream& operator<<(std::ostream& os, const Value& value);
    friend std::ostream& operator<<(std::ostream& os, const ValueDef& value);
    friend bool operator==(const Value& lhs, const Value& rhs);

    static Value create(const Node& value);
    static Value create(const Location& var, const Node& value);
    static Value create(
      const Location& var, const Node& value, const Values& sources);
    static Value copy_to(const Value& value, const Location& var);

  private:
    void to_string(std::ostream& buf, const Location& root) const;
    Location m_var;
    Node m_node;
    Values m_sources;
    bool m_invalid;
    ValueDef(const Node& value);
    ValueDef(const Location& var, const Node& value);
    ValueDef(const Location& var, const Node& value, const Values& sources);
  };
}