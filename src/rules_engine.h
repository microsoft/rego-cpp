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
    Node resolve_arithinfix(const Node& exprinfix);
    Node resolve_boolinfix(const Node& exprinfix);
    Node resolve_unaryexpr(const Node& unaryexpr);
    Node resolve_aritharg(const Node& aritharg);
    Node resolve_term(const Node& term);
    Node resolve_numterm(const Node& numterm);
    Node resolve_refterm(const Node& refterm);
    Node resolve_object(const Node& object);
    Node resolve_array(const Node& array);
    Node resolve_query(const Node& query);
    Node resolve_literal(const Node& literal);
    Node resolve_ref(const Node& ref);
    Node resolve_var(const Node& var);
    Node resolve_rulecomp(const Node& rulecomp);
    Node resolve_refhead(const Node& refhead);
    Node resolve_refdot(const Node& refhead, const Node& refdot);
    Node resolve_refbrack(const Node& refhead, const Node& refbrack);
    bool is_truthy(const Node& node);
  };

  using RulesEngine = std::shared_ptr<RulesEngineDef>;
}