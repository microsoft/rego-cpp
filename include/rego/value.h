#pragma once

#include <trieste/source.h>

namespace rego
{
  class ValueDef;
  using rank_t = std::size_t;
  using Value = std::shared_ptr<ValueDef>;
  using Values = std::vector<Value>;
  using RankedNode = std::pair<rank_t, Node>;

  class ValueDef
  {
  public:
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
    rank_t rank() const;
    friend std::ostream& operator<<(std::ostream& os, const Value& value);
    friend std::ostream& operator<<(std::ostream& os, const ValueDef& value);
    friend bool operator==(const Value& lhs, const Value& rhs);
    friend bool operator<(const Value& lhs, const Value& rhs);

    static Value create(const RankedNode& value);
    static Value create(const Location& var, const RankedNode& value);
    static Value create(
      const Location& var, const RankedNode& value, const Values& sources);
    static Value create(const Node& value);
    static Value create(const Location& var, const Node& value);
    static Value create(
      const Location& var, const Node& value, const Values& sources);
    static Value copy_to(const Value& value, const Location& var);
    static Values filter_by_rank(const Values& values);
    static rank_t get_rank(const Node& node);

  private:
    void to_string(std::ostream& buf, const Location& root, bool first) const;
    Location m_var;
    Node m_node;
    Values m_sources;
    bool m_invalid;
    rank_t m_rank;
    ValueDef(const RankedNode& value);
    ValueDef(const Location& var, const RankedNode& value);
    ValueDef(
      const Location& var, const RankedNode& value, const Values& sources);
    ValueDef(const Node& value);
    ValueDef(const Location& var, const Node& value);
    ValueDef(const Location& var, const Node& value, const Values& sources);
  };
}