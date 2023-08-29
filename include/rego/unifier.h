#pragma once

#include "args.h"
#include "builtins.h"
#include "value.h"
#include "variable.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <trieste/ast.h>
#include <vector>

namespace rego
{
  using namespace trieste;

  class UnifierDef;
  using Unifier = std::shared_ptr<UnifierDef>;
  using CallStack = std::shared_ptr<std::vector<Location>>;
  using ValuesLookup = std::map<std::string, Values>;
  using WithStack = std::shared_ptr<std::vector<ValuesLookup>>;
  using UnifierCache = std::shared_ptr<NodeMap<Unifier>>;

  class UnifierDef
  {
  public:
    UnifierDef(
      const Location& rule,
      const Node& rulebody,
      CallStack call_stack,
      WithStack with_stack,
      const BuiltIns& builtins,
      UnifierCache cache);
    Node unify();
    Nodes expressions() const;
    Nodes bindings() const;
    std::string str() const;
    std::string dependency_str() const;
    static Unifier create(
      const Location& rule,
      const Node& rulebody,
      const CallStack& call_stack,
      const WithStack& with_stack,
      const BuiltIns& builtins,
      const UnifierCache& cache);
    std::optional<RankedNode> resolve_rulecomp(const Node& rulecomp) const;
    std::optional<RankedNode> resolve_rulefunc(
      const Node& rulefunc, const Nodes& args) const;
    std::optional<Node> resolve_ruleset(const Nodes& ruleset) const;
    std::optional<Node> resolve_ruleobj(const Nodes& ruleobj) const;
    Node resolve_module(const Node& module) const;

  private:
    struct Dependency
    {
      std::string name;
      std::set<std::size_t> dependencies;
      std::size_t score;
    };

    Unifier rule_unifier(const Location& rule, const Node& rulebody) const;
    void init_from_body(
      const Node& rulebody, std::vector<Node>& statements, std::size_t root);
    std::size_t add_variable(const Node& local);
    std::size_t add_unifyexpr(const Node& unifyexpr);
    void add_withpush(const Node& withpush);
    void add_withpop(const Node& withpop);
    void reset();

    void compute_dependency_scores();
    std::size_t compute_dependency_score(
      std::size_t index, std::set<size_t>& visited);
    std::size_t dependency_score(const Variable& var) const;
    std::size_t dependency_score(const Node& expr) const;
    std::size_t detect_cycles() const;
    bool has_cycle(std::size_t id) const;

    Values evaluate(const Location& var, const Node& value);
    Values evaluate_function(
      const Location& var, const std::string& func_name, const Args& args);
    Values resolve_var(const Node& var);
    Values enumerate(const Location& var, const Node& container);
    Values resolve_skip(const Node& skip);
    Values check_with(const Node& var);
    Values apply_access(const Location& var, const Values& args);
    std::optional<Value> call_function(const Location& var, const Values& args);
    std::optional<Value> call_named_function(
      const Location& var, const std::string& name, const Values& args);
    bool is_local(const Node& var);
    std::size_t scan_vars(const Node& expr, std::vector<Location>& locals);
    void pass();
    void execute_statements(Nodes::iterator begin, Nodes::iterator end);
    void remove_invalid_values();
    void mark_invalid_values();
    Variable& get_variable(const Location& name);
    bool is_variable(const Location& name) const;
    Node bind_variables();
    Args create_args(const Node& args);

    bool push_rule(const Location& rule);
    void pop_rule(const Location& rule);

    void push_with(const Node& withseq);
    void pop_with();

    Location m_rule;
    std::map<Location, Variable> m_variables;
    std::vector<Node> m_statements;
    NodeMap<std::vector<Node>> m_nested_statements;
    CallStack m_call_stack;
    WithStack m_with_stack;
    BuiltIns m_builtins;
    std::size_t m_retries;
    Token m_parent_type;
    UnifierCache m_cache;
    NodeMap<std::size_t> m_expr_ids;
    std::vector<Dependency> m_dependency_graph;
  };
}