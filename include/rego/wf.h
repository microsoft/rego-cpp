#pragma once

#include "lang.h"

namespace rego
{
  using namespace wf::ops;

  inline const auto wf_json =
    JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull;

  inline const auto wf_arith_op = Add | Subtract | Multiply | Divide | Modulo;

  inline const auto wf_bool_op = Equals | NotEquals | LessThan |
    LessThanOrEquals | GreaterThan | GreaterThanOrEquals | Not;

  inline const auto wf_assign_op = Assign | Unify;

  inline const auto wf_parse_tokens = wf_json | wf_arith_op | wf_bool_op |
    Package | Var | Brace | Square | Dot | Paren | Assign | Unify | EmptySet |
    Colon | RawString | Default | Some;

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
    | (Square <<= (Group | List))
    | (List <<= Group++)
    | (Group <<= wf_parse_tokens++[1])
    | (Some <<= (List | Group)++)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_input_data =
    wf_parser
    | (DataSeq <<= Data++)
    | (Input <<= Var * Brace)[Var]
    | (Data <<= Brace)
    ;
  // clang-format on

  inline const auto wf_modules_tokens = wf_json | wf_arith_op | wf_bool_op |
    Paren | Var | Brace | Square | Dot | Assign | Unify | EmptySet | RawString |
    Default | Some;

  // clang-format off
  inline const auto wf_pass_modules =
    wf_pass_input_data
    | (ModuleSeq <<= Module++)
    | (Module <<= Package * Policy)
    | (Package <<= Var)
    | (Policy <<= Group++)
    | (List <<= (Group | ObjectItem)++)
    | (ObjectItem <<= Group * Group)
    | (Group <<= wf_modules_tokens++[1])
    ;
  // clang-format on

  inline const auto wf_lists_tokens = wf_json | wf_arith_op | wf_bool_op |
    Paren | Var | Set | UnifyBody | ObjectItemSeq | Array | Dot | Assign |
    Unify | Object | RawString | Default | SomeDecl;

  // clang-format off
  inline const auto wf_pass_lists =
    wf_pass_modules
    | (Object <<= ObjectItem++)
    | (ObjectItemSeq <<= ObjectItem++)
    | (ObjectItem <<= Group * Group)
    | (Array <<= Group++)
    | (Set <<= Group++)
    | (UnifyBody <<= (SomeDecl | Group)++)
    | (Input <<= Var * ObjectItemSeq)[Var]
    | (Data <<= ObjectItemSeq)
    | (Group <<= wf_lists_tokens++[1])
    | (List <<= Group++)
    | (SomeDecl <<= Group++)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_structure =
      (Top <<= Rego)
    | (Rego <<= Query * Input * DataSeq * ModuleSeq)
    | (Input <<= Var * ObjectItemSeq)[Var]
    | (DataSeq <<= Data++)
    | (Data <<= ObjectItemSeq)
    | (ObjectItemSeq <<= ObjectItem++)
    | (ModuleSeq <<= Module++)
    | (Query <<= Literal++[1])
    // Below this point is the grammar of the version of Rego we support
    | (Module <<= Package * Policy)
    | (Package <<= Var)
    | (Policy <<= (Rule | DefaultRule)++)
    | (DefaultRule <<= Var * Term)
    | (Rule <<= RuleHead * RuleBodySeq)
    | (RuleHead <<= Var * (RuleHeadType >>= RuleHeadComp | RuleHeadFunc))
    | (RuleBodySeq <<= (Empty | UnifyBody)++[1])
    | (RuleHeadComp <<= AssignOperator * Expr)
    | (RuleHeadFunc <<= RuleArgs * AssignOperator * Expr)
    | (RuleArgs <<= Term++[1])
    | (UnifyBody <<= Literal++[1])
    | (Literal <<= Expr | SomeDecl)
    | (SomeDecl <<= Var++)
    | (Expr <<= (Term | wf_arith_op | wf_bool_op | wf_assign_op | Expr)++[1])
    | (AssignOperator <<= wf_assign_op)
    | (Term <<= Ref | Var | Scalar | Array | Object | Set)
    | (Ref <<= Var * RefArgSeq)
    | (RefArgSeq <<= (RefArgDot | RefArgBrack | RefArgCall)++)
    | (RefArgDot <<= Var)
    | (RefArgBrack <<= Scalar | Var | Object | Array | Set)
    | (RefArgCall <<= Expr++[1])
    | (Scalar <<= String | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    | (String <<= JSONString | RawString)
    | (Array <<= Expr++)
    | (Set <<= Expr++)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= ObjectItemHead * Expr)
    | (ObjectItemHead <<= (Var | Ref | Scalar)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_strings =
    wf_pass_structure
    | (Scalar <<= JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_symbols =
    wf_pass_strings
    | (Module <<= Var * Policy)[Var]
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * Expr)[Var]
    | (RuleFunc <<= Var * RuleArgs * UnifyBody * Expr)[Var]
    | (RuleArgs <<= (ArgVar | ArgVal)++[1])
    | (UnifyBody <<= (Local | Literal)++[1])
    | (Query <<= UnifyBody)
    | (Local <<= Var * Undefined)[Var]
    | (Literal <<= Expr | NotExpr)    
    | (ArgVar <<= Var * Undefined)[Var]
    | (ArgVal <<= Scalar | Array | Object | Set)
    | (DefaultRule <<= Var * Term)[Var]
    | (Policy <<= (DefaultRule | RuleComp | RuleFunc)++)
    | (Object <<= (ObjectItem | RefObjectItem)++)
    | (ObjectItem <<= Key * Expr)[Key]
    | (RefObjectItem <<= RefTerm * Expr)
    | (Term <<= Scalar | Array | Object | Set)
    | (RefTerm <<= Ref | Var)
    | (NumTerm <<= JSONInt | JSONFloat)
    | (RefArgBrack <<= RefTerm | Scalar | Object | Array | Set)
    | (Expr <<= (RefTerm | NumTerm | Term | wf_arith_op | wf_bool_op | Unify | Expr)++[1])
    ;
  // clang-format-on

  // clang-format-off
  inline const auto wf_pass_locals = 
    wf_pass_symbols
    ;
  // clang-format-on

  inline const auto wf_math_tokens = RefTerm | NumTerm | UnaryExpr | ArithInfix;

  // clang-format off
  inline const auto wf_pass_multiply_divide =
    wf_pass_locals
    | (ArithInfix <<= ArithArg * (Op >>= Multiply | Divide | Modulo) * ArithArg)
    | (ArithArg <<= (Add | Subtract | Expr | wf_math_tokens)++[1])
    | (UnaryExpr <<= ArithArg)
    | (Expr <<= (NumTerm | RefTerm | Term | Add | Subtract | wf_bool_op | Unify | Expr | ArithInfix)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_add_subtract =
    wf_pass_multiply_divide
    | (ArithInfix <<= ArithArg * (Op >>= wf_arith_op) * ArithArg)
    | (ArithArg <<= Expr | wf_math_tokens)
    | (Expr <<= (NumTerm | RefTerm | Term | wf_bool_op | Unify | Expr | UnaryExpr | ArithInfix)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_comparison =
    wf_pass_add_subtract
    | (BoolInfix <<= BoolArg * (Op >>= wf_bool_op) * BoolArg)
    | (BoolArg <<= Term | wf_math_tokens)
    | (ArithArg <<= wf_math_tokens)
    | (Literal <<= Expr | NotExpr)
    | (NotExpr <<= Expr)
    | (Expr <<= (NumTerm | RefTerm | Term | UnaryExpr | Unify | Expr | ArithInfix | BoolInfix)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_assign =
    wf_pass_comparison
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term))[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody) * (Val >>= UnifyBody | Term))[Var]
    | (AssignInfix <<= AssignArg * AssignArg)
    | (AssignArg <<= wf_math_tokens | Term | BoolInfix)
    | (Expr <<= (NumTerm | RefTerm | Term | UnaryExpr | ArithInfix | BoolInfix | AssignInfix)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_refs =
    wf_pass_assign
    | (RefTerm <<= Ref | Var | SimpleRef)
    | (SimpleRef <<= Var * (Op >>= RefArgDot | RefArgBrack | RefArgCall))
    | (Expr <<= NumTerm | RefTerm | Term | UnaryExpr | ArithInfix | BoolInfix | AssignInfix)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_rulebody =
    wf_pass_refs
    | (UnifyExpr <<= Var * (Val >>= NotExpr | Expr))
    | (Expr <<= NumTerm | RefTerm | Term | UnaryExpr | ArithInfix | BoolInfix)
    | (UnifyBody <<= (Local | UnifyExpr)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_functions =
    wf_pass_rulebody
    | (UnifyExpr <<= Var * (Val >>= Var | Scalar | Function))
    | (Function <<= JSONString * ArgSeq)
    | (ArgSeq <<= (Scalar | Var | wf_arith_op | wf_bool_op)++)
    | (Array <<= Term++)
    | (Set <<= Term++)
    | (ObjectItem <<= Key * Term)[Key]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_merge_data =
    wf_pass_functions
    | (Rego <<= Query * Input * Data * ModuleSeq)
    | (Data <<= Var * ObjectItemSeq)[Var]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_merge_modules =
    wf_pass_merge_data
    | (Rego <<= Query * Input * Data)
    | (Module <<= (RuleComp | DefaultRule | RuleFunc)++)
    | (Data <<= Var * ObjectItemSeq * DataModuleSeq)[Var]
    | (DataModuleSeq <<= DataModule++)
    | (DataModule <<= Var * Module)[Var]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_rules =
    wf_pass_merge_modules
    | (Query <<= (Term | Binding)++[1])
    | (Binding <<= Var * Term)[Var]
    | (Term <<= Scalar | Array | Object | Set | Undefined)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_query =
    wf_pass_rules
    | (Top <<= (Binding | Term)++[1])
    ;
  // clang-format on
}