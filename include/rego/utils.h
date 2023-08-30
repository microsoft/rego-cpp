#pragma once

#include "lang.h"

namespace rego
{
  const inline auto ScalarToken =
    T(JSONInt) / T(JSONFloat) / T(JSONTrue) / T(JSONFalse) / T(JSONNull);
  const inline auto ArithToken =
    T(Add) / T(Subtract) / T(Multiply) / T(Divide) / T(Modulo);
  const inline auto ArithInfixArg = T(Expr) / T(NumTerm) / T(Ref) /
    T(UnaryExpr) / T(ArithInfix) / T(RefTerm) / T(ExprCall);
  const inline auto BinInfixArg = T(Expr) / T(Ref) / T(RefTerm) / T(ExprCall) /
    T(Set) / T(SetCompr) / T(BinInfix);
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack);
  const inline auto BoolToken = T(Equals) / T(NotEquals) / T(GreaterThan) /
    T(LessThan) / T(GreaterThanOrEquals) / T(LessThanOrEquals);
  const inline auto TermToken = T(Var) / T(Ref) / T(Array) / T(Object) /
    T(Set) / T(ArrayCompr) / T(ObjectCompr) / T(SetCompr);
  const inline auto StringToken = T(JSONString) / T(RawString);
  const inline auto ExprToken = T(Term) / ArithToken / BoolToken / StringToken /
    T(Expr) / ScalarToken / TermToken / T(JSONString) / T(Array) / T(Set) /
    T(Object) / T(Paren) / T(Not) / T(Dot) / T(And) / T(Or) / T(ExprCall);
  const auto inline MembershipToken = ScalarToken / T(JSONString) /
    T(RawString) / T(Var) / T(Object) / T(Array) / T(Set) / T(Dot) / T(Paren) /
    ArithToken / BoolToken / T(And) / T(Or);
  const auto inline RuleRefToken = T(Var) / T(Dot) / T(Array);

  inline const std::set<std::string> Keywords(
    {"if", "in", "contains", "every"});

  inline const std::set<Token> RuleTypes(
    {RuleComp, RuleFunc, RuleSet, RuleObj, DefaultRule});

  bool all_alnum(const std::string_view& str);
  bool contains_local(const Node& node);
  bool contains_ref(const Node& node);
  Node concat_refs(const Node& lhs, const Node& rhs);
  std::string flatten_ref(const Node& ref);
  bool is_in(const Node& node, const std::set<Token>& token);
  bool in_query(const Node& node);
  bool is_constant(const Node& node);
  std::string strip_quotes(const std::string_view& str);
  std::string to_json(const Node& node, bool sort = false, bool rego_set = true);
}