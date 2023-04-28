#pragma once

#include <trieste/driver.h>

namespace rego
{
  using namespace trieste;

  inline constexpr auto Paren = TokenDef("paren");
  inline constexpr auto Brace = TokenDef("brace");
  inline constexpr auto Assign = TokenDef("assign");
  inline constexpr auto TopKeyValueList = TokenDef("topkeyvaluelist");
  inline constexpr auto TopKeyValue = TokenDef("topkeyvalue", flag::lookdown);
  inline constexpr auto KeyValue = TokenDef("keyvalue", flag::lookdown);
  inline constexpr auto Square = TokenDef("square");
  inline constexpr auto List = TokenDef("list");
  inline constexpr auto SquareList = TokenDef("squarelist");
  inline constexpr auto KeyValueList = TokenDef("keyvaluelist");
  inline constexpr auto BraceList = TokenDef("bracelist");
  inline constexpr auto Dot = TokenDef("dot");

  inline constexpr auto Int = TokenDef("int", flag::print);
  inline constexpr auto Float = TokenDef("float", flag::print);
  inline constexpr auto String = TokenDef("string", flag::print);
  inline constexpr auto Bool = TokenDef("bool", flag::print);
  inline constexpr auto Ident = TokenDef("ident", flag::print);
  inline constexpr auto Object = TokenDef("object", flag::symtab);
  inline constexpr auto Key = TokenDef("key", flag::print);
  inline constexpr auto Null = TokenDef("null", flag::print);

  inline constexpr auto Print = TokenDef("print");
  inline constexpr auto Package = TokenDef("package");
  inline constexpr auto Query = TokenDef("query");
  inline constexpr auto Input = TokenDef("input", flag::symtab | flag::lookup);
  inline constexpr auto Data = TokenDef("data", flag::symtab | flag::lookup);

  inline constexpr auto Policy =
    TokenDef("policy", flag::symtab | flag::shadowing);
  inline constexpr auto Lookup = TokenDef("lookup");
  inline constexpr auto Local = TokenDef("local", flag::print);
  inline constexpr auto Index = TokenDef("index");
  inline constexpr auto Array = TokenDef("array");
  inline constexpr auto DataSeq = TokenDef("dataseq");
  inline constexpr auto ModuleSeq = TokenDef("moduleseq");
  inline constexpr auto Module = TokenDef("module", flag::symtab);
  inline constexpr auto RuleSeq = TokenDef("ruleseq");
  inline constexpr auto RuleBody = TokenDef("rulebody");
  inline constexpr auto RuleValue = TokenDef("rulevalue");
  inline constexpr auto Literal = TokenDef("literal");
  inline constexpr auto Rule =
    TokenDef("rule", flag::symtab | flag::lookup | flag::lookdown);
  inline constexpr auto Expression = TokenDef("expression");
  inline constexpr auto Math = TokenDef("math");
  inline constexpr auto Comparison = TokenDef("comparison");
  inline constexpr auto Ref = TokenDef("ref");
  inline constexpr auto Undefined = TokenDef("undefined");

  inline constexpr auto Add = TokenDef("+");
  inline constexpr auto Subtract = TokenDef("-");
  inline constexpr auto Multiply = TokenDef("*");
  inline constexpr auto Divide = TokenDef("/");
  inline constexpr auto Equals = TokenDef("==");
  inline constexpr auto NotEquals = TokenDef("!=");
  inline constexpr auto LessThan = TokenDef("<");
  inline constexpr auto LessThanOrEquals = TokenDef("<=");
  inline constexpr auto GreaterThan = TokenDef(">");
  inline constexpr auto GreaterThanOrEquals = TokenDef(">=");

  inline constexpr auto Id = TokenDef("id");
  inline constexpr auto Op = TokenDef("op");
  inline constexpr auto Lhs = TokenDef("lhs");
  inline constexpr auto Rhs = TokenDef("rhs");
  inline constexpr auto Head = TokenDef("head");
  inline constexpr auto Tail = TokenDef("tail");
  inline constexpr auto Value = TokenDef("value");

  inline auto err(NodeRange& r, const std::string& msg)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << r);
  }

  inline auto err(Node node, const std::string& msg)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << node);
  }

  inline auto err(const std::string& msg)
  {
    return Error << (ErrorMsg ^ msg);
  }

  Parse parser();
  Driver& driver();
  using PassCheck = std::tuple<std::string, Pass, const wf::Wellformed &>;
  std::vector<PassCheck> passes();
  std::string to_json(const Node& node);
  Node merge_lists(const Node& lhs, const Node& rhs);
}