#pragma once

#include "builtins.h"
#include "trieste/driver.h"
#include "trieste/token.h"

namespace rego
{
  using namespace trieste;

  // grammar tokens
  inline const auto Module = TokenDef("module", flag::symtab);
  inline const auto Package = TokenDef("package");
  inline const auto Policy = TokenDef("policy");
  inline const auto Rule = TokenDef("rule");
  inline const auto RuleHead = TokenDef("rule-head");
  inline const auto RuleHeadComp = TokenDef("rule-head-comp");
  inline const auto RuleHeadFunc = TokenDef("rule-head-func");
  inline const auto RuleHeadSet = TokenDef("rule-head-set");
  inline const auto RuleHeadObj = TokenDef("rule-head-obj");
  inline const auto RuleArgs = TokenDef("rule-args");
  inline const auto Query = TokenDef("query", flag::symtab);
  inline const auto Literal = TokenDef("literal");
  inline const auto Expr = TokenDef("expr");
  inline const auto ExprInfix = TokenDef("expr-infix");
  inline const auto ExprCall = TokenDef("expr-call");
  inline const auto ExprEvery = TokenDef("expr-every");
  inline const auto UnaryExpr = TokenDef("unary-expr");
  inline const auto Term = TokenDef("term");
  inline const auto InfixOperator = TokenDef("infix-operator");
  inline const auto BoolOperator = TokenDef("bool-operator");
  inline const auto ArithOperator = TokenDef("arith-operator");
  inline const auto AssignOperator = TokenDef("assign-operator");
  inline const auto Ref = TokenDef("ref");
  inline const auto RefArgBrack = TokenDef("ref-arg-brack");
  inline const auto RefArgDot = TokenDef("ref-arg-dot");
  inline const auto Var = TokenDef("var", flag::print);
  inline const auto Scalar = TokenDef("scalar");
  inline const auto String = TokenDef("string");
  inline const auto Array = TokenDef("array");
  inline const auto Object = TokenDef("object", flag::symtab);
  inline const auto Set = TokenDef("set");
  inline const auto EmptySet = TokenDef("empty-set");
  inline const auto ObjectItem = TokenDef("object-item");
  inline const auto RawString = TokenDef("raw-string", flag::print);
  inline const auto JSONString = TokenDef("STRING", flag::print);
  inline const auto JSONInt = TokenDef("INT", flag::print);
  inline const auto JSONFloat = TokenDef("FLOAT", flag::print);
  inline const auto JSONTrue = TokenDef("true");
  inline const auto JSONFalse = TokenDef("false");
  inline const auto JSONNull = TokenDef("null");
  inline const auto Equals = TokenDef("==");
  inline const auto NotEquals = TokenDef("!=");
  inline const auto LessThan = TokenDef("<");
  inline const auto GreaterThan = TokenDef(">");
  inline const auto LessThanOrEquals = TokenDef("<=");
  inline const auto GreaterThanOrEquals = TokenDef(">=");
  inline const auto Add = TokenDef("+");
  inline const auto Subtract = TokenDef("-");
  inline const auto Multiply = TokenDef("*");
  inline const auto Divide = TokenDef("/");
  inline const auto Modulo = TokenDef("%");
  inline const auto And = TokenDef("&");
  inline const auto Or = TokenDef("|");
  inline const auto Assign = TokenDef(":=");
  inline const auto Unify = TokenDef("=");
  inline const auto Default = TokenDef("default");
  inline const auto Some = TokenDef("some");
  inline const auto SomeDecl = TokenDef("some-decl");
  inline const auto SomeExpr = TokenDef("some-expr");
  inline const auto If = TokenDef("if");
  inline const auto IsIn = TokenDef("in");
  inline const auto Contains = TokenDef("contains");
  inline const auto Else = TokenDef("else");
  inline const auto As = TokenDef("as");
  inline const auto With = TokenDef("with");
  inline const auto Every = TokenDef("every");
  inline const auto ArrayCompr = TokenDef("array-compr");
  inline const auto ObjectCompr = TokenDef("object-compr");
  inline const auto SetCompr = TokenDef("set-compr");
  inline const auto Membership = TokenDef("membership");

  // intermediate tokens
  inline const auto UnifyBody = TokenDef("unify-body");
  inline const auto RefHead = TokenDef("ref-head");
  inline const auto RefArgSeq = TokenDef("ref-arg-seq");
  inline const auto DataSeq = TokenDef("data-seq");
  inline const auto ModuleSeq = TokenDef("module-seq");
  inline const auto DataItemSeq = TokenDef("data-item-seq");
  inline const auto DataItem = TokenDef("data-item", flag::lookdown);
  inline const auto DataObject = TokenDef("data-object", flag::symtab);
  inline const auto DataArray = TokenDef("data-array");
  inline const auto DataSet = TokenDef("data-set");
  inline const auto Submodule = TokenDef("submodule", flag::lookdown);
  inline const auto DataTerm = TokenDef("data-term");
  inline const auto ObjectItemSeq = TokenDef("object-item-seq");
  inline const auto ImportSeq = TokenDef("import-seq");
  inline const auto VarSeq = TokenDef("var-seq");
  inline const auto ElseSeq = TokenDef("else-seq");
  inline const auto WithSeq = TokenDef("with-seq");
  inline const auto SkipSeq = TokenDef("skip-seq");
  inline const auto ItemSeq = TokenDef("item-seq");
  inline const auto ItemSeq1 = TokenDef("item-seq-1");

  // data and input
  inline const auto Input = TokenDef("input", flag::symtab | flag::lookup);
  inline const auto Data = TokenDef("data", flag::symtab | flag::lookup);

  // fused
  inline const auto RuleComp = TokenDef(
    "rule-comp",
    flag::symtab | flag::lookup | flag::lookdown | flag::defbeforeuse);
  inline const auto RuleFunc = TokenDef(
    "rule-func",
    flag::symtab | flag::lookup | flag::lookdown | flag::defbeforeuse);
  inline const auto RuleSet = TokenDef(
    "rule-set",
    flag::symtab | flag::lookup | flag::lookdown | flag::defbeforeuse);
  inline const auto RuleObj = TokenDef(
    "rule-obj",
    flag::symtab | flag::lookup | flag::lookdown | flag::defbeforeuse);
  inline const auto DefaultRule =
    TokenDef("default-rule", flag::lookup | flag::lookdown);
  inline const auto Local = TokenDef("local", flag::lookup | flag::shadowing);
  inline const auto ArithInfix = TokenDef("arith-infix");
  inline const auto BinInfix = TokenDef("bin-infix");
  inline const auto BoolInfix = TokenDef("bool-infix");
  inline const auto AssignInfix = TokenDef("assign-infix");
  inline const auto ArgVar = TokenDef("arg-var", flag::lookup);
  inline const auto ArgVal = TokenDef("arg-val");
  inline const auto LiteralWith = TokenDef("literal-with");
  inline const auto LiteralEnum = TokenDef("literal-enum");
  inline const auto LiteralInit = TokenDef("literal-init");
  inline const auto LiteralNot = TokenDef("literal-not");

  // utility
  inline const auto Undefined = TokenDef("undefined");
  inline const auto Rego = TokenDef("rego", flag::symtab);
  inline const auto Math = TokenDef("math");
  inline const auto Op = TokenDef("op");
  inline const auto Comparison = TokenDef("comparison");
  inline const auto Function = TokenDef("function");
  inline const auto Arg = TokenDef("arg");
  inline const auto ArgSeq = TokenDef("arg-seq");
  inline const auto Val = TokenDef("value");
  inline const auto Id = TokenDef("id");
  inline const auto Head = TokenDef("head");
  inline const auto Tail = TokenDef("tail");
    inline const auto Head1 = TokenDef("head-1");
  inline const auto Tail1 = TokenDef("tail-1");
    inline const auto Head2 = TokenDef("head-2");
  inline const auto Tail2 = TokenDef("tail-2");
  inline const auto Lhs = TokenDef("lhs");
  inline const auto Rhs = TokenDef("rhs");
  inline const auto Key = TokenDef("key", flag::print);
  inline const auto RefTerm = TokenDef("ref-term");
  inline const auto NumTerm = TokenDef("num-term");
  inline const auto ArithArg = TokenDef("arith-arg");
  inline const auto BinArg = TokenDef("bin-arg");
  inline const auto BoolArg = TokenDef("bool-arg");
  inline const auto AssignArg = TokenDef("assign-arg");
  inline const auto RuleHeadType = TokenDef("rule-head-type");
  inline const auto UnifyExpr = TokenDef("unify-expr");
  inline const auto UnifyExprWith = TokenDef("unify-expr-with");
  inline const auto UnifyExprCompr = TokenDef("unify-expr-compr");
  inline const auto UnifyExprEnum = TokenDef("unify-expr-enum");
  inline const auto UnifyExprNot = TokenDef("unify-expr-not");
  inline const auto TermSet = TokenDef("term-set");
  inline const auto Empty = TokenDef("empty");
  inline const auto SimpleRef = TokenDef("simple-ref");
  inline const auto Binding = TokenDef("binding");
  inline const auto DefaultTerm = TokenDef("default-term");
  inline const auto Body = TokenDef("body");
  inline const auto Import =
    TokenDef("import", flag::lookdown | flag::lookup | flag::shadowing);
  inline const auto Keyword =
    TokenDef("keyword", flag::lookdown | flag::lookup);
  inline const auto Idx = TokenDef("idx");
  inline const auto Idx1 = TokenDef("idx-1");
  inline const auto Skip = TokenDef("skip", flag::lookup);
  inline const auto NestedBody = TokenDef("nested-body", flag::symtab);
  inline const auto BuiltInHook = TokenDef("builtin-hook", flag::lookup);
  inline const auto RuleRef = TokenDef("rule-ref");
  inline const auto Item = TokenDef("item");
  inline const auto Item1 = TokenDef("item-1");
  inline const auto Enumerate = TokenDef("enumerate");
  inline const auto Compr = TokenDef("compr");
  inline const auto Merge = TokenDef("merge");
  inline const auto ToValues = TokenDef("to-values");
  inline const auto ImportRef = TokenDef("import-ref");
  inline const auto WithRef = TokenDef("with-ref");
  inline const auto WithExpr = TokenDef("with-expr");
  inline const auto EverySeq = TokenDef("every-seq");
  inline const auto ErrorCode = TokenDef("error-code", flag::print);
  inline const auto ErrorSeq = TokenDef("error-seq");

  // lists
  inline const auto List = TokenDef("list");

  // parser
  inline const auto Brace = TokenDef("brace");
  inline const auto Dot = TokenDef("dot");
  inline const auto Colon = TokenDef("colon");
  inline const auto Square = TokenDef("square");
  inline const auto Paren = TokenDef("paren");
  inline const auto Not = TokenDef("not");
  inline const auto Placeholder = TokenDef("_");

  inline const std::set<std::string> Keywords(
    {"if", "in", "contains", "every"});

  inline const std::set<Token> RuleTypes(
    {RuleComp, RuleFunc, RuleSet, RuleObj, DefaultRule});

  Node err(
    NodeRange& r,
    const std::string& msg,
    const std::string& code = "runtime_error");

  Node err(
    Node node,
    const std::string& msg,
    const std::string& code = "runtime_error");

  Parse parser();
  Driver& driver(const BuiltIns& builtins);
  using PassCheck = std::tuple<std::string, Pass, const wf::Wellformed*>;
  std::vector<PassCheck> passes(const BuiltIns& builtins);
  std::string to_json(const Node& node, bool sort = false);
  bool contains_local(const Node& node);
  bool contains_ref(const Node& node);
  bool is_in(const Node& node, const std::set<Token>& token);
  bool in_query(const Node& node);
  bool is_constant(const Node& node);
  std::string strip_quotes(const std::string& str);
}