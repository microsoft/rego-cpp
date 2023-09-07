// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "bigint.h"
#include "rego.h"
#include "tokens.h"

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
    ArithToken / BoolToken / T(And) / T(Or) / T(ExprCall);
  const auto inline RuleRefToken = T(Var) / T(Dot) / T(Array);

  inline const std::set<std::string> Keywords(
    {"if", "in", "contains", "every"});

  inline const std::set<Token> RuleTypes(
    {RuleComp, RuleFunc, RuleSet, RuleObj, DefaultRule});

  struct UnwrapResult
  {
    Node node;
    bool success;
  };

  class UnwrapOpt
  {
  public:
    UnwrapOpt(std::size_t index);
    bool exclude_got() const;
    UnwrapOpt& exclude_got(bool exclude_got);
    bool specify_number() const;
    UnwrapOpt& specify_number(bool specify_number);
    const std::string& code() const;
    UnwrapOpt& code(const std::string& value);
    const std::string& pre() const;
    UnwrapOpt& pre(const std::string& value);
    const std::string& message() const;
    UnwrapOpt& message(const std::string& value);
    const std::string& func() const;
    UnwrapOpt& func(const std::string& value);
    const std::vector<Token>& types() const;
    UnwrapOpt& types(const std::vector<Token>& value);
    UnwrapOpt& type(const Token& value);

    Node unwrap(const Nodes& args) const;

  private:
    bool m_exclude_got;
    bool m_specify_number;
    std::string m_code;
    std::string m_prefix;
    std::string m_message;
    std::string m_func;
    std::vector<Token> m_types;
    std::size_t m_index;
  };

  bool all_alnum(const std::string_view& str);
  bool contains_local(const Node& node);
  bool contains_ref(const Node& node);
  Node concat_refs(const Node& lhs, const Node& rhs);
  std::string flatten_ref(const Node& ref);
  bool is_in(const Node& node, const std::set<Token>& token);
  bool in_query(const Node& node);
  bool is_constant(const Node& node);
  bool is_instance(const Node& value, const std::set<Token>& types);
  bool is_falsy(const Node& node);
  bool is_truthy(const Node& node);
  bool is_undefined(const Node& node);
  std::string strip_quotes(const std::string_view& str);
  BigInt get_int(const Node& node);
  double get_double(const Node& node);
  std::string get_string(const Node& node);
  bool get_bool(const Node& node);
  std::string type_name(const Token& type, bool specify_number = false);
  std::string type_name(const Node& node, bool specify_number = false);
  Node unwrap_arg(const Nodes& args, const UnwrapOpt& options);
  UnwrapResult unwrap(const Node& term, const std::set<Token>& types);
}