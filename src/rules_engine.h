#pragma once

#include <map>
#include <trieste/driver.h>
#include <vector>

namespace rego
{
  using namespace trieste;

  class RulesEngineDef
  {
  public:
    Node resolve(const Node& node) const;

  private:
    Node resolve_expr(const Node& expr) const;
    Node resolve_notexpr(const Node& notexpr) const;
    Node resolve_arithinfix(const Node& arithinfix) const;
    Node resolve_boolinfix(const Node& boolinfix) const;
    Node resolve_unaryexpr(const Node& unaryexpr) const;
    Node resolve_aritharg(const Node& aritharg) const;
    Node resolve_term(const Node& term) const;
    Node resolve_numterm(const Node& numterm) const;
    Node resolve_refterm(const Node& refterm) const;
    Node resolve_object(const Node& object) const;
    Node resolve_array(const Node& array) const;
    Node resolve_set(const Node& set) const;
    Node resolve_query(const Node& query) const;
    Node resolve_literal(const Node& literal) const;
    Node resolve_ref(const Node& ref) const;
    Node resolve_var(const Node& var) const;
    Node resolve_rulecomp(const Node& rulecomp) const;
    Node resolve_rulefunc(const Node& rulefunc) const;
    Node resolve_localrule(const Node& localrule) const;
    Node resolve_refhead(const Nodes& defs) const;
    Nodes resolve_refdot(const Nodes& defs, const Node& refdot) const;
    Nodes resolve_refbrack(const Nodes& defs, const Node& refbrack) const;
    Nodes resolve_refcall(const Nodes& defs, const Node& refcall) const;
    Node resolve_set_membership(const Node& set, const Node& query) const;
    bool is_truthy(const Node& node) const;
    Nodes object_lookdown(const Node& object, const Node& query) const;
    Nodes lookdown(const Node& parent, const Node& query) const;
    Node find_valid_rulecomp(const Nodes& rules) const;
    Node find_valid_rulefunc(const Nodes& rules, const Nodes& args) const;
    Node evaluate_body(const Node& rulebody) const;
    Node find_valid(const Nodes& rules) const;
    Node inject_args(const Node& rulefunc, const Nodes& args) const;
  };

  using RulesEngine = std::shared_ptr<RulesEngineDef>;
}