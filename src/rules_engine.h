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
    Node resolve(const Node& node);

  private:
    Node resolve_expr(const Node& expr);
    Node resolve_notexpr(const Node& expr);
    Node resolve_arithinfix(const Node& exprinfix);
    Node resolve_boolinfix(const Node& exprinfix);
    Node resolve_unaryexpr(const Node& unaryexpr);
    Node resolve_aritharg(const Node& aritharg);
    Node resolve_term(const Node& term);
    Node resolve_numterm(const Node& numterm);
    Node resolve_refterm(const Node& refterm);
    Node resolve_object(const Node& object);
    Node resolve_array(const Node& array);
    Node resolve_set(const Node& set);
    Node resolve_query(const Node& query);
    Node resolve_literal(const Node& literal);
    Node resolve_ref(const Node& ref);
    Node resolve_var(const Node& var);
    Node resolve_rulecomp(const Node& rulecomp);
    Node resolve_localrule(const Node& localrule);
    Node resolve_refhead(const Node& refhead);
    Node resolve_refdot(const Node& refhead, const Node& refdot);
    Node resolve_refbrack(const Node& refhead, const Node& refbrack);
    Node resolve_set_membership(const Node& set, const Node& query);
    bool is_truthy(const Node& node);
    Nodes object_lookdown(const Node& object, const Node& query);
    Node lookdown(const Node& parent, const Node& query);
    Node lookup(const Node& query);
    Node find_valid_rule(const Nodes& rules);
    Node evaluate_body(const Node& rulebody);
  };

  using RulesEngine = std::shared_ptr<RulesEngineDef>;
}