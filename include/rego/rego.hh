// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <trieste/driver.h>
#include "rego_c.h"

/** 
 *  This namespace provides the C++ API for the library.
 *  It includes all the token types for nodes in the AST, the well-formedness
 *  definitions for each pass, the built-in system and custom types for
 *  handling various kinds of data (e.g. the BigInt class). 
 */
namespace rego
{
  using namespace trieste;

  // grammar tokens
  inline const auto Module = TokenDef("module", flag::symtab);
  inline const auto Package = TokenDef("package");
  inline const auto Policy = TokenDef("policy");
  inline const auto Rule = TokenDef("rule", flag::symtab);
  inline const auto RuleHead = TokenDef("rule-head");
  inline const auto RuleHeadComp = TokenDef("rule-head-comp");
  inline const auto RuleHeadFunc = TokenDef("rule-head-func");
  inline const auto RuleHeadSet = TokenDef("rule-head-set");
  inline const auto RuleHeadObj = TokenDef("rule-head-obj");
  inline const auto RuleArgs = TokenDef("rule-args");
  inline const auto Query = TokenDef("query");
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
  inline const auto Object = TokenDef("object");
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
  inline const auto RuleRef = TokenDef("rule-ref");
  inline const auto RefHead = TokenDef("ref-head");
  inline const auto RefArgSeq = TokenDef("ref-arg-seq");
  inline const auto ModuleSeq = TokenDef("module-seq");
  inline const auto DataSeq = TokenDef("data-seq");
  inline const auto DataItemSeq = TokenDef("data-item-seq");
  inline const auto DataItem = TokenDef("data-item", flag::lookup);
  inline const auto DataRule = TokenDef("data-rule", flag::lookup);
  inline const auto DataObject = TokenDef("data-object");
  inline const auto DataObjectItem = TokenDef("data-object-item");
  inline const auto DataArray = TokenDef("data-array");
  inline const auto DataSet = TokenDef("data-set");
  inline const auto DataModule = TokenDef("data-module", flag::lookup);
  inline const auto Submodule =
    TokenDef("submodule", flag::lookdown | flag::lookup);
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
  inline const auto Input = TokenDef("input", flag::lookup);
  inline const auto Data = TokenDef("data", flag::lookup);

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
  inline const auto LhsVars = TokenDef("lhs-vars");
  inline const auto RhsVars = TokenDef("rhs-vars");
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
  inline const auto Item = TokenDef("item");
  inline const auto Item1 = TokenDef("item-1");
  inline const auto Enumerate = TokenDef("enumerate");
  inline const auto Compr = TokenDef("compr");
  inline const auto Merge = TokenDef("merge");
  inline const auto ImportRef = TokenDef("import-ref");
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
  inline const auto Comma = TokenDef("comma");
  inline const auto Placeholder = TokenDef("_");

  using namespace wf::ops;

  inline const auto wf_json =
    JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull;

  inline const auto wf_arith_op = Add | Subtract | Multiply | Divide | Modulo;

  inline const auto wf_bin_op = And | Or | Subtract;

  inline const auto wf_bool_op = Equals | NotEquals | LessThan |
    LessThanOrEquals | GreaterThan | GreaterThanOrEquals | Not;

  inline const auto wf_assign_op = Assign | Unify;

  inline const auto wf_parse_tokens = wf_json | wf_arith_op | wf_bool_op |
    wf_bin_op | Package | Var | Brace | Square | Dot | Paren | Assign | Unify |
    EmptySet | Colon | RawString | Default | Some | Import | Else | As | With |
    Placeholder;

  // clang-format off
  inline const auto wf_parser =
      (Top <<= Rego)
    | (Rego <<= Query * Input * DataSeq * ModuleSeq)
    | (Query <<= Group++)
    | (Input <<= File | Undefined)
    | (ModuleSeq <<= File++)
    | (DataSeq <<= File++)
    | (File <<= Group++)
    | (Brace <<= (List | Group)++)
    | (Paren <<= (Group | List))
    | (Square <<= (Group | List)++)
    | (List <<= Group++)
    | (Group <<= wf_parse_tokens++)
    | (Some <<= (List | Group)++)
    | (With <<= Group * Group)
    | (Error <<= ErrorMsg * ErrorAst * ErrorCode)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_input_data =
    wf_parser
    | (DataSeq <<= Data++)
    | (Input <<= Key * (Val >>= Group | Undefined))[Key]
    | (Data <<= Brace)
    ;
  // clang-format on

  inline const auto wf_modules_tokens =
    wf_parse_tokens - (Package | Colon | Import | Placeholder);

  // clang-format off
  inline const auto wf_pass_modules =
    wf_pass_input_data
    | (ModuleSeq <<= Module++)
    | (Module <<= Package * ImportSeq * Policy)
    | (Package <<= Group)
    | (ImportSeq <<= Import++)
    | (Import <<= Group)
    | (Keyword <<= Var)
    | (Policy <<= Group++)
    | (List <<= (Group | ObjectItem)++)
    | (Brace <<= (List | Group)++)
    | (ObjectItem <<= Group * Group)
    | (Group <<= wf_modules_tokens++)
    | (Square <<= (Group | List)++)
    ;
  // clang-format on

  inline const auto wf_imports_tokens = wf_modules_tokens - As;

  // clang-format off
  inline const auto wf_pass_imports =
    wf_pass_modules
    | (ImportSeq <<= (Import | Keyword)++)
    | (Keyword <<= Var)[Var]
    | (Import <<= ImportRef * As * (Val >>= Var | Undefined))
    | (ImportRef <<= Group)
    | (With <<= RuleRef * WithExpr)
    | (RuleRef <<= Group)
    | (WithExpr <<= Group)
    | (Group <<= wf_imports_tokens++)
    ;
  // clang-format off    

  inline const auto wf_keywords_tokens = wf_imports_tokens | IsIn | Contains | Every | If;

  // clang-format off
  inline const auto wf_pass_keywords =
    wf_pass_imports
    | (Group <<= wf_keywords_tokens++)
    ;
  // clang-format on

  inline const auto wf_lists_tokens =
    (wf_keywords_tokens - (Some | Every | EmptySet | Brace | Square)) |
    UnifyBody | ObjectItemSeq | Array | Object | Set | ExprEvery | SomeDecl |
    ObjectCompr | ArrayCompr | SetCompr | Comma | Undefined;

  // clang-format off
  inline const auto wf_pass_lists =
    wf_pass_keywords
    | (Object <<= ObjectItem++)
    | (ObjectItemSeq <<= ObjectItem++)
    | (ObjectItem <<= Group * Group)
    | (Array <<= Group++)
    | (Set <<= Group++)
    | (UnifyBody <<= (SomeDecl | Group)++)
    | (Input <<= Key * (Val >>= Group | Undefined))[Key]
    | (Data <<= ObjectItemSeq)
    | (Group <<= wf_lists_tokens++)
    | (List <<= Group++)
    | (SomeDecl <<= VarSeq * Group)
    | (ExprEvery <<= VarSeq * UnifyBody * EverySeq)
    | (EverySeq <<= Group)
    | (VarSeq <<= Group++)
    | (ObjectCompr <<= Group * Group * UnifyBody)
    | (ArrayCompr <<= Group * UnifyBody)
    | (SetCompr <<= Group * UnifyBody)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_ifs = wf_pass_lists;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_elses =
    wf_pass_ifs
    | (Else <<= (Val >>= Group) * (Body >>= UnifyBody | Empty))
    ;
  // clang-format on

  inline const auto wf_rules_tokens = wf_lists_tokens - (If | Else | Default);

  // clang-format off
  inline const auto wf_pass_rules =
    wf_pass_elses
    | (Policy <<= Rule++)
    | (Rule <<= (Default >>= JSONTrue | JSONFalse) * RuleHead * (Body >>= UnifyBody | Empty) * ElseSeq)
    | (RuleHead <<= RuleRef * (RuleHeadType >>= RuleHeadComp | RuleHeadFunc | RuleHeadSet | RuleHeadObj))
    | (RuleRef <<= (Var | Array | Dot)++[1])
    | (ElseSeq <<= Else++)
    | (Else <<= (Val >>= Group) * (Body >>= UnifyBody | Empty))
    | (RuleHeadComp <<= AssignOperator * Group)
    | (RuleHeadFunc <<= RuleArgs * AssignOperator * Group)
    | (RuleHeadSet <<= Group)
    | (RuleHeadObj <<= Group * AssignOperator * Group)
    | (RuleArgs <<= Group++)
    | (AssignOperator <<= wf_assign_op)
    | (Group <<= wf_rules_tokens++)
    ;
  // clang-format on

  inline const auto wf_call_tokens = wf_rules_tokens | ExprCall;

  // clang-format off
  inline const auto wf_pass_build_calls =
    wf_pass_rules
    | (ExprCall <<= RuleRef * ArgSeq)
    | (ArgSeq <<= Group++)
    | (Group <<= wf_call_tokens++[1])
    ;
  // clang-format on

  inline const auto wf_membership_tokens =
    (wf_call_tokens - Comma) | Membership;

  // clang-format off
  inline const auto wf_pass_membership =
    wf_pass_build_calls
    | (Membership <<= (Idx >>= Group | Undefined) * (Item >>= Group) * Group)
    | (Group <<= wf_membership_tokens++[1])
    ;
  // clang-format off

  inline const auto wf_refs_tokens = wf_membership_tokens | Ref;

  // clang-format off
  inline const auto wf_pass_build_refs =
    wf_pass_membership
    | (Ref <<= RefHead * RefArgSeq)
    | (RefHead <<= Var | Array | Object | Set | ArrayCompr | ObjectCompr | SetCompr | ExprCall)
    | (RefArgSeq <<= (RefArgDot | RefArgBrack)++)
    | (RefArgDot <<= Var)
    | (RefArgBrack <<= Group)
    | (RuleRef <<= (Var | Array | Dot | Ref)++[1])
    | (Group <<= wf_refs_tokens++[1])
    ;
  // clang-format on

  inline const auto wf_structure_exprs = Term | wf_arith_op | wf_bin_op |
    wf_bool_op | wf_assign_op | Dot | ExprCall | ExprEvery | Membership | Expr;

  // clang-format off
  inline const auto wf_pass_structure =
      (Top <<= Rego)
    | (Rego <<= Query * Input * DataSeq * ModuleSeq)
    | (Input <<= Key * (Val >>= Term | Undefined))[Key]
    | (DataSeq <<= Data++)
    | (Data <<= ObjectItemSeq)
    | (ObjectItemSeq <<= ObjectItem++)
    | (ModuleSeq <<= Module++)
    | (Query <<= Literal++[1])
    | (Error <<= ErrorMsg * ErrorAst * ErrorCode)
    // Below this point is the grammar of the version of Rego we support
    | (Module <<= Package * ImportSeq * Policy)
    | (ImportSeq <<= (Import | Keyword)++)
    | (Import <<= Ref * As * Var)
    | (Keyword <<= Var)[Var]
    | (Package <<= Ref)
    | (Policy <<= Rule++)
    | (Rule <<= (Default >>= JSONTrue | JSONFalse) * RuleHead * (Body >>= UnifyBody | Empty) * ElseSeq)
    | (RuleHead <<= RuleRef * (RuleHeadType >>= RuleHeadComp | RuleHeadFunc | RuleHeadSet | RuleHeadObj))
    | (RuleRef <<= Ref | Var)
    | (ElseSeq <<= Else++)
    | (Else <<= (Val >>= Expr) * (Body >>= UnifyBody | Empty))
    | (RuleHeadComp <<= AssignOperator * Expr)
    | (RuleHeadFunc <<= RuleArgs * AssignOperator * Expr)
    | (RuleHeadSet <<= Expr)
    | (RuleHeadObj <<= Expr * AssignOperator * Expr)
    | (RuleArgs <<= Term++)
    | (UnifyBody <<= (Literal | LiteralWith)++[1])
    | (Literal <<= Expr | SomeDecl | SomeExpr)
    | (LiteralWith <<= UnifyBody * WithSeq)
    | (WithSeq <<= With++)
    | (With <<= RuleRef * Expr)
    | (SomeDecl <<= VarSeq)
    | (SomeExpr <<= (Idx >>= Expr | Undefined) * (Item >>= Expr) * IsIn)
    | (VarSeq <<= Var++)
    | (IsIn <<= Expr | Undefined)
    | (Expr <<= wf_structure_exprs++[1])
    | (ExprCall <<= RuleRef * ArgSeq)
    | (ExprEvery <<= VarSeq * UnifyBody * IsIn)
    | (ArgSeq <<= Expr++)
    | (AssignOperator <<= wf_assign_op)
    | (Term <<= Ref | Var | Scalar | Array | Object | Set | ArrayCompr | ObjectCompr | SetCompr)
    | (Ref <<= RefHead * RefArgSeq)
    | (RefHead <<= Var | Array | Object | Set | ArrayCompr | ObjectCompr | SetCompr | ExprCall)
    | (RefArgSeq <<= (RefArgDot | RefArgBrack)++)
    | (RefArgDot <<= Var)
    | (RefArgBrack <<= Scalar | Var | Object | Array | Set | Expr)
    | (Scalar <<= String | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    | (String <<= JSONString | RawString)
    | (Array <<= Expr++)
    | (Set <<= Expr++)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= (Key >>= Expr) * (Val >>= Expr))
    | (ObjectCompr <<= Expr * Expr * (Body >>= UnifyBody))
    | (ArrayCompr <<= Expr * (Body >>= UnifyBody))
    | (SetCompr <<= Expr * (Body >>= UnifyBody))
    | (Membership <<= (Idx >>= Expr | Undefined) * (Item >>= Expr) * (ItemSeq >>= Expr))
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_strings =
    wf_pass_structure
    | (Scalar <<= JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_merge_data =
    wf_pass_strings
    | (Rego <<= Query * Input * Data * ModuleSeq)
    | (Input <<= Key * (Val >>= DataTerm | Undefined))[Key]
    | (Data <<= Key * (Val >>= DataModule))[Key]
    | (DataModule <<= (DataRule | Submodule)++)
    | (DataRule <<= Var * (Val >>= DataTerm))
    | (Submodule <<= Key * (Val >>= DataModule))[Key]
    | (DataTerm <<= Scalar | DataArray | DataObject | DataSet)
    | (DataArray <<= DataTerm++)
    | (DataSet <<= DataTerm++)
    | (DataObject <<= DataObjectItem++)
    | (DataObjectItem <<= (Key >>= DataTerm) * (Val >>= DataTerm))
    | (RuleArgs <<= (ArgVar | ArgVal)++)
    | (ArgVar <<= Var * (Val >>= Undefined))[Var]
    | (ArgVal <<= Scalar | Array | Object | Set)
    ;
  // clang-format on

  inline const auto wf_pass_lift_refheads = wf_pass_merge_data;

  inline const auto wf_symbols_exprs =
    (wf_structure_exprs - (Assign | Dot | ExprEvery)) | RefTerm | NumTerm |
    Set | SetCompr;

  // clang-format off
  inline const auto wf_pass_symbols =
    wf_pass_lift_refheads
    | (Module <<= Package * Policy)
    | (Policy <<= (Import | RuleComp | RuleFunc | RuleSet | RuleObj)++)
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * (Idx >>= JSONInt))[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * (Idx >>= JSONInt))[Var]
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= Expr | Term))[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Key >>= Expr | Term) * (Val >>= Expr | Term))[Var]
    | (UnifyBody <<= (Local | Literal | LiteralWith | LiteralEnum)++[1])
    | (LiteralEnum <<= (Item >>= Var) * (ItemSeq >>= Expr))
    | (Query <<= (Body >>= UnifyBody))
    | (Local <<= Var * (Val >>= Undefined))[Var]
    | (Literal <<= Expr | SomeExpr)
    | (Term <<= Scalar | Array | Object | Set | ArrayCompr | SetCompr | ObjectCompr)
    | (ObjectCompr <<= Expr * Expr * (Body >>= NestedBody))
    | (ArrayCompr <<= Expr * (Body >>= NestedBody))
    | (SetCompr <<= Expr * (Body >>= NestedBody))
    | (RefTerm <<= Ref | Var)
    | (NumTerm <<= JSONInt | JSONFloat)
    | (RefArgBrack <<= RefTerm | Scalar | Object | Array | Set | Expr)
    | (Expr <<= wf_symbols_exprs++[1])
    | (Import <<= Var * Ref)[Var]
    | (NestedBody <<= Key * (Val >>= UnifyBody))
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_replace_argvals =
    wf_pass_symbols
    | (RuleArgs <<= ArgVar++)
    | (Literal <<= Expr)
    ;
  //

  // clang-format off
  inline const auto wf_pass_lift_query =
    wf_pass_replace_argvals
    | (Query <<= Var)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_expand_imports = wf_pass_lift_query;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_constants =
    wf_pass_lift_query
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | DataTerm) * (Idx >>= JSONInt))[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | DataTerm) * (Idx >>= JSONInt))[Var]
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= Expr | DataTerm))[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Key >>= Expr | DataTerm) * (Val >>= Expr | DataTerm))[Var]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_explicit_enums = 
    wf_pass_constants
    | (LiteralEnum <<= (Item >>= Var) * (ItemSeq >>= Var) * UnifyBody);
  // clang-format on

  inline const auto wf_pass_locals = wf_pass_explicit_enums;

  // clang-format off
  inline const auto wf_pass_rules_to_compr =
    wf_pass_locals
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= (UnifyBody | DataTerm)))[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= (UnifyBody | DataTerm)))[Var]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_compr =
    wf_pass_rules_to_compr
    | (ObjectCompr <<= Var * NestedBody)
    | (ArrayCompr <<= Var * NestedBody)
    | (SetCompr <<= Var * NestedBody)
    ;
  // clang-format on

  inline const auto wf_pass_absolute_refs = wf_pass_compr;

  // clang-format off
  inline const auto wf_pass_merge_modules =
    wf_pass_absolute_refs
    | (Rego <<= Query * Input * Data)
    | (DataModule <<= (DataRule | RuleComp | RuleFunc | RuleSet | RuleObj | Submodule)++)
    | (Submodule <<= Key * (Val >>= DataModule))[Key]
    | (DataItem <<= Key * (Val >>= DataModule))[Key]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_datarule = 
    wf_pass_merge_modules
    | (DataModule <<= (RuleComp | RuleFunc | RuleSet | RuleObj | Submodule)++)
    | (Rego <<= Query * Input * Data)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_skips = 
    wf_pass_datarule
    | (Rego <<= Query * Input * Data * SkipSeq)
    | (SkipSeq <<= Skip++)
    | (Skip <<= Key * (Val >>= VarSeq | BuiltInHook | Undefined))[Key]
    ;
  // clang-format on

  inline const auto wf_unary_exprs = wf_symbols_exprs | UnaryExpr;

  // clang-format off
  inline const auto wf_pass_unary =
    wf_pass_skips
    | (UnaryExpr <<= ArithArg)
    | (ArithArg <<= (Expr | RefTerm | NumTerm | UnaryExpr | ExprCall))
    | (Expr <<= wf_unary_exprs++[1])
    ;
  // clang-format on

  inline const auto wf_math_tokens =
    RefTerm | NumTerm | UnaryExpr | ArithInfix | ExprCall;

  inline const auto wf_bin_tokens =
    RefTerm | Set | SetCompr | ExprCall | BinInfix;

  inline const auto wf_multiply_divide_exprs =
    (wf_unary_exprs - (Multiply | Divide | Modulo | And)) | ArithInfix |
    BinInfix;

  // clang-format off
  inline const auto wf_pass_multiply_divide =
    wf_pass_unary
    | (ArithInfix <<= ArithArg * (Op >>= Multiply | Divide | Modulo) * ArithArg)
    | (ArithArg <<= Expr | wf_math_tokens)
    | (BinInfix <<= BinArg * (Op >>= And) * BinArg)
    | (BinArg <<= (Expr | wf_bin_tokens)++[1])
    | (UnaryExpr <<= ArithArg)
    | (Expr <<= wf_multiply_divide_exprs++[1])
    ;
  // clang-format on

  inline const auto wf_add_subtract_exprs =
    wf_multiply_divide_exprs - (Add | Subtract | Or);

  // clang-format off
  inline const auto wf_pass_add_subtract =
    wf_pass_multiply_divide
    | (ArithInfix <<= ArithArg * (Op >>= wf_arith_op) * ArithArg)
    | (ArithArg <<= Expr | wf_math_tokens)
    | (BinInfix <<= BinArg * (Op >>= wf_bin_op) * BinArg)
    | (BinArg <<= Expr | wf_bin_tokens)
    | (Expr <<= wf_add_subtract_exprs++[1])
    ;
  // clang-format on

  inline const auto wf_comparison_exprs =
    (wf_add_subtract_exprs - wf_bool_op) | BoolInfix;

  // clang-format off
  inline const auto wf_pass_comparison =
    wf_pass_add_subtract
    | (BoolInfix <<= BoolArg * (Op >>= wf_bool_op) * BoolArg)
    | (BoolArg <<= Term | BinInfix | wf_math_tokens)
    | (ArithArg <<= wf_math_tokens)
    | (BinArg <<= wf_bin_tokens)
    | (Expr <<= wf_comparison_exprs++[1])
    | (UnifyBody <<= (Local | Literal | LiteralWith | LiteralEnum | LiteralNot)++[1])
    | (LiteralNot <<= UnifyBody)
    ;
  // clang-format on

  inline const auto wf_assign_exprs =
    (wf_comparison_exprs - (Unify | Expr | Set | SetCompr)) | AssignInfix;

  // clang-format off
  inline const auto wf_pass_assign =
    wf_pass_comparison
    | (AssignInfix <<= AssignArg * AssignArg)
    | (AssignArg <<= wf_math_tokens | Term | BinInfix | BoolInfix | Membership)
    | (Expr <<= wf_assign_exprs++[1])
    ;
  // clang-format on

  inline const auto wf_pass_skip_refs = wf_pass_assign;

  // clang-format off
  inline const auto wf_pass_simple_refs =
    wf_pass_skip_refs
    | (RefTerm <<= Var | SimpleRef)
    | (SimpleRef <<= Var * (Op >>= RefArgDot | RefArgBrack))
    | (Expr <<= wf_assign_exprs)
    | (ExprCall <<= Var * ArgSeq)
    | (RefHead <<= Var)
    | (RuleRef <<= Var)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_init =
    wf_pass_simple_refs
    | (UnifyBody <<= (Local | Literal | LiteralWith | LiteralEnum | LiteralNot | LiteralInit)++[1])
    | (LiteralInit <<= VarSeq * VarSeq * AssignInfix)
    ;
  // clang-format on

  inline const auto wf_pass_implicit_enums = wf_pass_init;

  inline const auto wf_rulebody_exprs = (wf_assign_exprs - AssignInfix);

  // clang-format off
  inline const auto wf_pass_rulebody =
    wf_pass_implicit_enums
    | (Module <<= (Import | RuleComp | RuleFunc | RuleSet | RuleObj | Submodule)++)
    | (UnifyExpr <<= Var * (Val >>= Expr))
    | (Expr <<= wf_rulebody_exprs)
    | (UnifyBody <<= (Local | UnifyExpr | UnifyExprWith | UnifyExprCompr | UnifyExprEnum | UnifyExprNot)++[1])
    | (UnifyExprWith <<= UnifyBody * WithSeq)
    | (UnifyExprCompr <<= Var * (Val >>= ArrayCompr | SetCompr | ObjectCompr) * NestedBody)
    | (UnifyExprEnum <<= Var * (Item >>= Var) * (ItemSeq >>= Var) * UnifyBody)
    | (UnifyExprNot <<= UnifyBody)
    | (ArrayCompr <<= Var)
    | (SetCompr <<= Var)
    | (ObjectCompr <<= Var)
    | (With <<= RuleRef * Var)
    ;
  // clang-format on

  const inline auto wf_lift_to_rule_exprs =
    wf_rulebody_exprs | Enumerate | ArrayCompr | SetCompr | ObjectCompr | Merge;

  // clang-format off
  inline const auto wf_pass_lift_to_rule =
    wf_pass_rulebody
    | (UnifyBody <<= (Local | UnifyExpr | UnifyExprWith | UnifyExprNot)++[1])
    | (Expr <<= wf_lift_to_rule_exprs)
    | (Merge <<= Var)
    | (Enumerate <<= Expr)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_functions =
    wf_pass_lift_to_rule
    | (UnifyExpr <<= Var * (Val >>= Var | Scalar | Function))
    | (Function <<= JSONString * ArgSeq)
    | (ArgSeq <<= (Scalar | Var | wf_arith_op | wf_bin_op | wf_bool_op | NestedBody | VarSeq)++)
    | (Input <<= Key * (Val >>= Term | Undefined))[Key]
    | (Array <<= Term++)
    | (Set <<= Term++)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= (Key >>= Term) * (Val >>= Term))
    | (DataItem <<= Key * (Val >>= DataModule | Term))[Key]
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * (Idx >>= JSONInt))[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * (Idx >>= JSONInt))[Var]
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term))[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term))[Var]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_unify =
    wf_pass_functions
    | (Query <<= (Term | Binding)++)
    | (Binding <<= Var * Term)[Var]
    | (Term <<= Scalar | Array | Object | Set)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_query =
    wf_pass_unify
    | (Top <<= (Binding | Term)++)
    ;
  // clang-format on

  struct BuiltInDef;
  using BuiltIn = std::shared_ptr<BuiltInDef>;
  using BuiltInBehavior = Node (*)(const Nodes&);

  /**
   * Result of unwrapping a node. 
   */
  struct UnwrapResult
  {
    /** The unwrapped argument or nullptr (if unsuccessful). */
    Node node;

    /** True if the argument was unwrapped successfully. */
    bool success;
  };

  /**
   * This struct provides options for unwrapping an argument and producing
   * an error message.
   * 
   * The act of unwrapping a node is the process of testing whether a node
   * or one of its direct descendents is of one or more types. So, for example,
   * the following nodes:
   * 
   * ```
   * (term (scalar (int 5)))
   * (scalar (int 5))
   * (int 5)
   * ```
   * 
   * Would all be successfully unwrapped as `(int 5)` if the type JSONInt was
   * specified. However, if JSONFloat was specified, the result would be an
   * error node.
   */
  class UnwrapOpt
  {
  public:
    /**
     * Construct an UnwrapOpt.
     * 
     * @param index The index of the argument to unwrap. 
     */
    UnwrapOpt(std::size_t index);

    /**
     * Whether the statement indicating what was received instead
     * of the expect type should be excluded.
     */
    bool exclude_got() const;
    UnwrapOpt& exclude_got(bool exclude_got);

    /**
     * Whether to specify in the error message which kind of number
     * was received (i.e. integer or floating point) 
     */
    bool specify_number() const;
    UnwrapOpt& specify_number(bool specify_number);

    /**
     * The error code for the error message.
     * 
     * Default value if omitted is EvalTypeError.
     */
    const std::string& code() const;
    UnwrapOpt& code(const std::string& value);

    /**
     * The error preamble.
     * 
     * If omitted, a default preamble will be constructed
     * from the operation metadata in the form "operand <i> must be <t>". 
     */
    const std::string& pre() const;
    UnwrapOpt& pre(const std::string& value);

    /**
     * The full error message.
     * 
     * If this is set, no message will be generated and instead
     * this will be returned verbatim. 
     */
    const std::string& message() const;
    UnwrapOpt& message(const std::string& value);

    /**
     * The name of the function.
     * 
     * If provide, will be a prefix on the message as "<func-name>:"
     */
    const std::string& func() const;
    UnwrapOpt& func(const std::string& value);

    /**
     * The types to match against.
     * 
     * The operand must be one of the provided types or else an
     * error node will be return.
     */
    const std::vector<Token>& types() const;
    UnwrapOpt& types(const std::vector<Token>& value);

    /**
     * The singular type to match against.
     */
    const Token& type() const;
    UnwrapOpt& type(const Token& value);

    /**
     * Unwraps an argument from the provided vector of nodes. 
     * 
     * @param args The vector of nodes to unwrap from.
     * @return The unwrapped argument or an appropriate error node.
     */
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

  /**
   * Unwraps an argument from the provided vector of nodes.
   * 
   * @param args The vector of nodes to unwrap from.
   * @param options The options for unwrapping.
   * @return The unwrapped argument or an appropriate error node. 
   */
  Node unwrap_arg(const Nodes& args, const UnwrapOpt& options);

  /**
   * Attempts to unwrap an argument to one of the specified types.
   * 
   * @param term The term to unwrap.
   * @param types The acceptable types.
   * @return An unwrap result.
   */
  UnwrapResult unwrap(const Node& term, const std::set<Token>& types);

  /**
   * Extracts the value of a node as an integer.
   * 
   * @param node The node to extract from.
   * @return The integer value of the node.
   */ 
  BigInt get_int(const Node& node);

  /**
   * Extracts the value of a node as a double.
   * 
   * @param node The node to extract from.
   * @return The double value of the node.
   */
  double get_double(const Node& node);

  /**
   * Extracts the value of a node as a string.
   * 
   * The resulting string will have any enclosing quotes removed.
   * 
   * @param node The node to extract from.
   * @return The string value of the node.
   */
  std::string get_string(const Node& node);

  /**
   * Extracts the value of a node as a boolean.
   * 
   * @param node The node to extract from.
   * @return The boolean value of the node.
   */
  bool get_bool(const Node& node);

  /**
   * Converts the value to a scalar node.
   * 
   * @param value The value to convert.
   * @return The scalar node (scalar (INT <value>)))
   */
    Node scalar(BigInt value);

    /**
     * Converts the value to a scalar node.
     * 
     * @param value The value to convert.
     * @return The scalar node (scalar (FLOAT <value>)))
     */
    Node scalar(double value);

    /**
     * Converts the value to a scalar node.
     * 
     * @param value The value to convert.
     * @return The scalar node (scalar (TRUE|FALSE)))
     */
    Node scalar(bool value);

    /**
     * Converts the value to a scalar node.
     * 
     * @param value The value to convert.
     * @return The scalar node (scalar (STRING <value>)))
     */
    Node scalar(const char* value);

    /**
     * Converts the value to a scalar node.
     * 
     * @param value The value to convert.
     * @return The scalar node (scalar (STRING <value>)))
     */
    Node scalar(const std::string& value);

    /**
     * Converts the value to a scalar node.
     * 
     * @param value The value to convert.
     * @return The scalar node (scalar (NULL)))
     */
    Node scalar();

    /**
     * Converts the key and val terms to an object item.
     * 
     * @param key_term The key term.
     * @param val_term The value term.
     * @return The object item node (object_item (key_term) (val_term))
     */
    Node object_item(const Node& key_term, const Node& val_term);

    /**
     * Converts the value to an object node.
     * 
     * @param object_items The object items of the object
     * @return The object node (object (object_item) (object_item) ...)
     */
    Node object(const Nodes& object_items);

    /**
     * Converts the value to an array node.
     * 
     * @param array_members The members of the array.
     * @return The array node (array (term) (term) ...)
     */
    Node array(const Nodes& array_members);

    /**
     * Converts the value to a set node.
     * 
     * @param set_members The members of the set.
     * @return The set node (set (term) (term) ...)
     */
    Node set(const Nodes& set_members);


  /** This constant indicates that a built-in can receive any number of arguments. */
  const std::size_t AnyArity = std::numeric_limits<std::size_t>::max();

  /** 
   * Struct which defines a built-in function.
   * 
   * You can extend Rego by registering your own built-ins. A built-in is a
   * function which is called by Rego during evaluation. Built-ins are called
   * with a vector of Nodes, and return a Node. The vector of Nodes contains the
   * arguments passed to the built-in. The Node returned by the built-in is the
   * result of the built-in's evaluation.
   * 
   * Here is an example built-in which performs addition:
   * 
   * ```cpp
   * Node add(const Nodes& args)
   * {
   *   Node a = unwrap_arg(args, UnwrapOpt(0).types({Int, Float}));
   *   if(a.type() == Error){
   *     return a;
   *   }
   *   
   *   Node b = unwrap_arg(args, UnwrapOpt(1).types({Int, Float}));
   *   if(b.type() == Error){
   *    return b;
   *   }
   * 
   *   if(a.type() == Int && b.type() == Int)
   *   {
   *     return scalar(get_int(a) + get_int(b));
   *   }
   * 
   *   return scalar(get_double(a) + get_double(b));
   * }
   * ```
   * 
   * Note that there are several helper methods and objects to aid in
   * writing managing nodes and wrapping/unwrapping them into basic
   * types. Once a method like the above has been written, use
   * BuiltInDef::create following way:
   * ```cpp
   * builtins.register_builtin(BuiltInDef::create(Location("add"), 2, add))
   * ```
   * 
   * Then, during evaluation, any call to the built-in `add` will be
   * handled by the `add` function.
   */
  struct BuiltInDef
  {
    /** The name used to match against expression calls in the rego program. */
    Location name;
    /** 
     * The number of expected arguments.
     * 
     * If any number of arguments can be provided, use the constant AnyArity.
     */
    std::size_t arity;

    /** The function which will be called when the built-in is evaluated. */
    BuiltInBehavior behavior;

    /**
     * Creates a new built-in.
     * 
     * BuiltIn is a pointer to a BuiltInDef.
     * 
     * @param name The name of the built-in.
     * @param arity The number of arguments expected by the built-in.
     * @param behavior The function which will be called when the built-in is evaluated.
     * @return The built-in.
     */
    static BuiltIn create(
      const Location& name, std::size_t arity, BuiltInBehavior behavior);
  };

  /**
   * Manages the set of builtins used by an interpreter to resolve built-in calls.
   */
  class BuiltIns
  {
  public:
    /** Constructor. */
    BuiltIns() noexcept;

    /**
     * Determines whether the provided name refers to a built-in.
     * 
     * @param name The name to check.
     * @return True if the name refers to a built-in, otherwise false.
     */
    bool is_builtin(const Location& name) const;

    /**
     * Calls the built-in with the provided name and arguments.
     * 
     * @param name The name of the built-in to call.
     * @param args The arguments to pass to the built-in.
     * @return The result of the built-in call.
     */
    Node call(const Location& name, const Nodes& args) const;

    /**
     * Registers a built-in.
     * 
     * @param built_in The built-in to register.
     */
    BuiltIns& register_builtin(const BuiltIn& built_in);

    /** 
     * Gets the built-in with the provided name. 
     */
    const BuiltIn& at(const Location& name) const;

    /**
     * Whether to throw built-in errors.
     * 
     * If true, built-in errors will be thrown as exceptions. If false,
     * built-in errors will result in Undefined nodes.
     */
    bool strict_errors() const;
    BuiltIns& strict_errors(bool strict_errors);

    /**
     * Registers a set of built-ins.
     * 
     * @param built_ins The built-ins to register.
     */
    template <typename T>
    BuiltIns& register_builtins(const T& built_ins)
    {
      for (auto& built_in : built_ins)
      {
        register_builtin(built_in);
      }

      return *this;
    }

    /**
     * This registers the "standard library" of built-ins.
     * 
     * There are a number of built-ins which are provided by default. These
     * built-ins are those documented
     * <a href="https://www.openpolicyagent.org/docs/latest/policy-reference/#built-in-functions">here</a>.
     * 
     * rego-cpp supports the following built-ins as standard:
     * 
     */
    BuiltIns& register_standard_builtins();
    std::map<Location, BuiltIn>::const_iterator begin() const;
    std::map<Location, BuiltIn>::const_iterator end() const;

  private:
    std::map<Location, BuiltIn> m_builtins;
    bool m_strict_errors;
  };

  /**
   * This is a basic, non-optimized implementation of a big integer using
   * strings. In most circumstances this would be considerably slower than other
   * approaches, but given the way in which Trieste nodes store their content as
   * Location objects into a source document, and this class operates over those
   * Locations, it is actual quite efficient when compared to parsing and
   * serializing the Location into a vector of unsigned longs.
   */
  class BigInt
  {
  public:
    BigInt();
    BigInt(const Location& value);
    BigInt(std::int64_t value);
    BigInt(std::size_t value);

    const Location& loc() const;
    std::int64_t to_int() const;
    std::size_t to_size() const;
    bool is_negative() const;
    bool is_zero() const;
    BigInt increment() const;
    BigInt decrement() const;

    static bool is_int(const Location& loc);

    friend BigInt operator+(const BigInt& lhs, const BigInt& rhs);
    friend BigInt operator-(const BigInt& lhs, const BigInt& rhs);
    friend BigInt operator*(const BigInt& lhs, const BigInt& rhs);
    friend BigInt operator/(const BigInt& lhs, const BigInt& rhs);
    friend BigInt operator%(const BigInt& lhs, const BigInt& rhs);

    friend bool operator>(const BigInt& lhs, const BigInt& rhs);
    friend bool operator<(const BigInt& lhs, const BigInt& rhs);
    friend bool operator<=(const BigInt& lhs, const BigInt& rhs);
    friend bool operator>=(const BigInt& lhs, const BigInt& rhs);
    friend bool operator==(const BigInt& lhs, const BigInt& rhs);
    friend bool operator!=(const BigInt& lhs, const BigInt& rhs);

    friend std::ostream& operator<<(std::ostream& os, const BigInt& value);

    BigInt negate() const;

  private:
    struct DivideResult
    {
      std::string quotient;
      std::string remainder;
    };

    static bool less_than(
      const std::string_view& lhs, const std::string_view& rhs);
    static bool greater_than(
      const std::string_view& lhs, const std::string_view& rhs);
    static bool equal(const std::string_view& lhs, const std::string_view& rhs);
    static std::string add(
      const std::string_view& lhs, const std::string_view& rhs, bool negative);
    static std::string subtract(
      const std::string_view& lhs, const std::string_view& rhs, bool negative);
    static DivideResult divide(
      const std::string_view& lhs, const std::string_view& rhs);
    static std::string multiply(
      const std::string_view& lhs, const std::string_view& rhs);
    std::string_view digits() const;
    Location m_loc;
    static Location Zero;
    static Location One;
  };

  const std::string EvalTypeError = "eval_type_error";
  const std::string EvalBuiltInError = "eval_builtin_error";
  const std::string RegoTypeError = "rego_type_error";
  const std::string EvalConflictError = "eval_conflict_error";
  const std::string WellFormedError = "wellformed_error";
  const std::string RuntimeError = "runtime_error";

  Node err(
    NodeRange& r,
    const std::string& msg,
    const std::string& code = WellFormedError);

  Node err(
    Node node,
    const std::string& msg,
    const std::string& code = WellFormedError);

  Parse parser();
  Driver& driver(const BuiltIns& builtins);
  using PassCheck = std::tuple<std::string, Pass, const wf::Wellformed*>;
  std::vector<PassCheck> passes(const BuiltIns& builtins);
  Node version();


  std::string to_json(
    const trieste::Node& node, bool sort = false, bool rego_set = true);
  void set_logging_enabled(bool enabled);

class Interpreter
  {
  public:
    Interpreter();
    ~Interpreter();
    void add_module_file(const std::filesystem::path& path);
    void add_module(const std::string& name, const std::string& contents);
    void add_data_json_file(const std::filesystem::path& path);
    void add_data_json(const std::string& json);
    void add_data(const Node& node);
    void set_input_json_file(const std::filesystem::path& path);
    void set_input_json(const std::string& json);
    void set_input(const Node& node);
    std::string query(const std::string& query_expr) const;
    Node raw_query(const std::string& query_expr) const;
    Interpreter& debug_path(const std::filesystem::path& prefix);
    const std::filesystem::path& debug_path() const;
    Interpreter& debug_enabled(bool enabled);
    bool debug_enabled() const;
    Interpreter& well_formed_checks_enabled(bool enabled);
    bool well_formed_checks_enabled() const;
    Interpreter& executable(const std::filesystem::path& path);
    const std::filesystem::path& executable() const;
    BuiltIns& builtins();
    const BuiltIns& builtins() const;

  private:
    friend const char* ::regoGetError(regoInterpreter* rego);
    friend void setError(regoInterpreter* rego, const std::string& error);
    friend regoOutput* ::regoQuery(
      regoInterpreter* rego, const char* query_expr);

    void insert_module(const Node& module);
    std::string output_to_string(const Node& output) const;
    void write_ast(
      std::size_t index, const std::string& pass, const Node& ast) const;
    Node get_errors(const Node& ast) const;
    Parse m_parser;
    wf::Wellformed m_wf_parser;
    Node m_module_seq;
    Node m_data_seq;
    Node m_input;
    std::filesystem::path m_debug_path;
    bool m_debug_enabled;
    bool m_well_formed_checks_enabled;
    BuiltIns m_builtins;

    std::string m_c_error;
  };
}