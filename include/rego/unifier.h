#pragma once

#include "args.h"
#include "value.h"
#include "variable.h"

#include <map>
#include <set>
#include <string>
#include <trieste/driver.h>
#include <vector>

namespace rego
{
  using namespace trieste;
  using CallStack = std::shared_ptr<std::vector<Location>>;

  class Unifier
  {
  public:
    Unifier(
      const Location& rule,
      const Node& rulebody,
      CallStack& call_stack,
      std::size_t max_passes = 100);
    ~Unifier();
    Node unify();
    Nodes expressions() const;
    Nodes bindings() const;
    std::string str() const;
  friend std::ostream& operator<<(std::ostream& os, const Unifier& unifier);

  private:
    void init_from_body(const Node& rulebody);
    void add_variable(const Node& local);
    void add_unifyexpr(const Node& unifyexpr);
    void compute_dependency_scores();
    std::size_t compute_dependency_score(const Node& unifyexpr);
    Values evaluate(const Location& var, const Node& value);
    Values evaluate_function(
      const Location& var, const std::string& func_name, const Args& args);
    Values resolve_var(const Node& var);
    Values access_args(const Location& var, const Node& container);
    std::optional<Node> resolve_rulecomp(const Node& rulecomp);
    std::optional<Node> resolve_rulefunc(
      const Node& rulefunc, const Nodes& args);
    Values call_function(
      const Location& var, const std::string& name, const Values& args);
    bool is_local(const Node& var);
    std::size_t scan_vars(const Node& expr, std::vector<Location>& locals);
    bool pass();
    bool mark_invalid_values();
    bool remove_invalid_values();
    Node bind_variables();
    Args create_args(const Node& args);

    static bool push_rule(CallStack& callstack, const Location& rule);
    static void pop_rule(CallStack& callstack, const Location& rule);

    Location m_rule;
    std::map<Location, Variable> m_variables;
    std::vector<Node> m_unifyexprs;
    CallStack m_call_stack;
    std::size_t m_max_passes;
    std::size_t m_retries;
  };
}