#pragma once

#include <trieste/driver.h>

namespace rego
{
  using namespace trieste;

  // grammar tokens
  inline constexpr auto Module = TokenDef("module", flag::symtab);
  inline constexpr auto Package = TokenDef("package");
  inline constexpr auto Policy = TokenDef("policy");
  inline constexpr auto Rule = TokenDef("rule");
  inline constexpr auto RuleHead = TokenDef("rule-head");
  inline constexpr auto RuleHeadComp = TokenDef("rule-head-comp");
  inline constexpr auto RuleBody = TokenDef("rule-body");
  inline constexpr auto Query = TokenDef("query");
  inline constexpr auto Literal = TokenDef("literal");
  inline constexpr auto Expr = TokenDef("expr");
  inline constexpr auto ExprInfix = TokenDef("expr-infix");
  inline constexpr auto UnaryExpr = TokenDef("unary-expr");
  inline constexpr auto NotExpr = TokenDef("not-expr");
  inline constexpr auto Term = TokenDef("term");
  inline constexpr auto InfixOperator = TokenDef("infix-operator");
  inline constexpr auto BoolOperator = TokenDef("bool-operator");
  inline constexpr auto ArithOperator = TokenDef("arith-operator");
  inline constexpr auto AssignOperator = TokenDef("assign-operator");
  inline constexpr auto Ref = TokenDef("ref");
  inline constexpr auto RefArg = TokenDef("ref-arg");
  inline constexpr auto RefArgBrack = TokenDef("ref-arg-brack");
  inline constexpr auto RefArgDot = TokenDef("ref-arg-dot");
  inline constexpr auto Var = TokenDef("var", flag::print);
  inline constexpr auto Scalar = TokenDef("scalar");
  inline constexpr auto String = TokenDef("string");
  inline constexpr auto Array = TokenDef("array");
  inline constexpr auto Object = TokenDef("object", flag::symtab);
  inline constexpr auto Set = TokenDef("set");
  inline constexpr auto EmptySet = TokenDef("empty-set");
  inline constexpr auto ObjectItem = TokenDef("object-item", flag::lookdown);
  inline constexpr auto RefObjectItem = TokenDef("ref-object-item");
  inline constexpr auto RawString = TokenDef("raw-string", flag::print);
  inline constexpr auto JSONString = TokenDef("STRING", flag::print);
  inline constexpr auto JSONInt = TokenDef("INT", flag::print);
  inline constexpr auto JSONFloat = TokenDef("FLOAT", flag::print);
  inline constexpr auto JSONTrue = TokenDef("true");
  inline constexpr auto JSONFalse = TokenDef("false");
  inline constexpr auto JSONNull = TokenDef("null");
  inline constexpr auto Equals = TokenDef("==");
  inline constexpr auto NotEquals = TokenDef("!=");
  inline constexpr auto LessThan = TokenDef("<");
  inline constexpr auto GreaterThan = TokenDef(">");
  inline constexpr auto LessThanOrEquals = TokenDef("<=");
  inline constexpr auto GreaterThanOrEquals = TokenDef(">=");
  inline constexpr auto Add = TokenDef("+");
  inline constexpr auto Subtract = TokenDef("-");
  inline constexpr auto Multiply = TokenDef("*");
  inline constexpr auto Divide = TokenDef("/");
  inline constexpr auto Modulo = TokenDef("%");
  inline constexpr auto Assign = TokenDef(":=");

  // intermediate tokens
  inline constexpr auto RuleBodySeq = TokenDef("rule-body-seq");
  inline constexpr auto RefHead = TokenDef("ref-head");
  inline constexpr auto RefArgSeq = TokenDef("ref-arg-seq");
  inline constexpr auto DataSeq = TokenDef("data-seq");
  inline constexpr auto ModuleSeq = TokenDef("module-seq");
  inline constexpr auto DataItem = TokenDef("data-item");
  inline constexpr auto DataItemSeq = TokenDef("data-item");
  inline constexpr auto DataModule = TokenDef("data-module", flag::lookdown);
  inline constexpr auto DataModuleSeq = TokenDef("data-module-seq");
  inline constexpr auto ObjectItemHead = TokenDef("object-item-head");
  inline constexpr auto ObjectItemSeq = TokenDef("object-item-seq");

  // data and input
  inline constexpr auto Input = TokenDef("input", flag::symtab | flag::lookup);
  inline constexpr auto Data = TokenDef("data", flag::symtab | flag::lookup);

  // fused
  inline constexpr auto RuleComp =
    TokenDef("rule-comp", flag::symtab | flag::lookup | flag::lookdown);
  inline constexpr auto DefaultRule =
    TokenDef("default-rule", flag::lookup | flag::lookdown);
  inline constexpr auto LocalRule = TokenDef("local-rule", flag::lookup);
  inline constexpr auto ArithInfix = TokenDef("arith-infix");
  inline constexpr auto BoolInfix = TokenDef("bool-infix");

  // utility
  inline constexpr auto Undefined = TokenDef("undefined");
  inline constexpr auto Rego = TokenDef("rego", flag::symtab);
  inline constexpr auto Math = TokenDef("math");
  inline constexpr auto Op = TokenDef("op");
  inline constexpr auto Comparison = TokenDef("comparison");
  inline constexpr auto Value = TokenDef("value");
  inline constexpr auto Id = TokenDef("id");
  inline constexpr auto Head = TokenDef("head");
  inline constexpr auto Tail = TokenDef("tail");
  inline constexpr auto Lhs = TokenDef("lhs");
  inline constexpr auto Rhs = TokenDef("rhs");
  inline constexpr auto Key = TokenDef("key", flag::print);
  inline constexpr auto RefTerm = TokenDef("ref-term");
  inline constexpr auto NumTerm = TokenDef("num-term");
  inline constexpr auto ArithArg = TokenDef("arith-arg");

  // lists
  inline constexpr auto List = TokenDef("list");

  // parser
  inline constexpr auto Brace = TokenDef("brace");
  inline constexpr auto Dot = TokenDef("dot");
  inline constexpr auto Colon = TokenDef("colon");
  inline constexpr auto Square = TokenDef("square");
  inline constexpr auto Paren = TokenDef("paren");
  inline constexpr auto Not = TokenDef("not");
  inline constexpr auto Default = TokenDef("default");

  inline auto err(NodeRange& r, const std::string& msg)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << r);
  }

  inline auto err(Node node, const std::string& msg)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << node->clone());
  }

  inline auto err(const std::string& msg)
  {
    return Error << (ErrorMsg ^ msg);
  }

  Parse parser();
  Driver& driver();
  using PassCheck = std::tuple<std::string, Pass, const wf::Wellformed&>;
  std::vector<PassCheck> passes();
  std::string to_json(const Node& node);
}