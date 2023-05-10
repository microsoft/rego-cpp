#pragma once

#include "lang.h"

namespace rego
{
  using namespace wf::ops;

  inline const auto wf_json =
    JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull;

  inline const auto wf_arith_op = Add | Subtract | Multiply | Divide;

  inline const auto wf_bool_op = Equals | NotEquals | LessThan |
    LessThanOrEquals | GreaterThan | GreaterThanOrEquals;

  inline const auto wf_parse_tokens = wf_json | wf_arith_op | wf_bool_op |
    Package | Var | Brace | Square | Dot | Paren | Assign;

  // clang-format off
  inline const auto wf_parser =
      (Top <<= Rego)
    | (Rego <<= Query * Input * DataSeq * ModuleSeq)
    | (Query <<= Group)
    | (Input <<= File | Undefined)
    | (ModuleSeq <<= File++)
    | (DataSeq <<= File++)
    | (File <<= Group++)
    | (Brace <<= (Group | ObjectItem)++)
    | (Paren <<= Group)
    | (Square <<= Group++)
    | (ObjectItem <<= Group * Group)
    | (Group <<= wf_parse_tokens++[1])
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
    Paren | Var | Brace | Square | Dot | Assign;

  // clang-format off
  inline const auto wf_pass_modules =
    wf_pass_input_data
    | (ModuleSeq <<= Module++)
    | (Module <<= Package * Policy)
    | (Package <<= Var)
    | (Policy <<= Group++)
    | (Group <<= wf_modules_tokens++[1])
    ;
  // clang-format on

  inline const auto wf_lists_tokens = wf_json | wf_arith_op | wf_bool_op |
    Paren | Var | BraceList | ObjectItemList | SquareList | Dot | Assign;

  // clang-format off
  inline const auto wf_pass_lists =
    wf_pass_modules
    | (ObjectItemList <<= ObjectItem++)
    | (ObjectItem <<= Group * Group)
    | (SquareList <<= Group++)
    | (BraceList <<= Group++)
    | (Input <<= Var * ObjectItemList)[Var]
    | (Data <<= ObjectItemList)
    | (Group <<= wf_lists_tokens++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_structure =
      (Top <<= Rego)
    | (Rego <<= Query * Input * DataSeq * ModuleSeq)
    | (Input <<= Var * ObjectItemList)[Var]
    | (DataSeq <<= Data++)
    | (Data <<= ObjectItemList)
    | (ObjectItemList <<= ObjectItem++)
    | (ModuleSeq <<= Module++)
    | (Query <<= Literal++[1])
    // Below this point is the grammar of the version of Rego we support
    | (Module <<= Package * Policy)
    | (Package <<= Var)
    | (Policy <<= Rule++)
    | (Rule <<= RuleHead * RuleBodySeq)
    | (RuleHead <<= Var * RuleHeadComp)
    | (RuleBodySeq <<= RuleBody)
    | (RuleHeadComp <<= AssignOperator * Expr)
    | (RuleBody <<= (Expr | Rule)++)
    | (Literal <<= Expr)
    | (Expr <<= (Term | wf_arith_op | wf_bool_op | Expr)++[1])
    | (AssignOperator <<= Assign)
    | (Term <<= Ref | Var | Scalar | Array | Object)
    | (Ref <<= Var * RefArgSeq)
    | (RefArgSeq <<= (RefArgDot | RefArgBrack)++)
    | (RefArgDot <<= Var)
    | (RefArgBrack <<= Scalar | Var)
    | (Scalar <<= String | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    | (String <<= JSONString)
    | (Array <<= Expr++)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= Scalar * Expr)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_symbols =
    wf_pass_structure
    | (Module <<= Var * Policy)[Var]
    | (RuleComp <<= Var * Expr * RuleBodySeq)[Var]
    | (Policy <<= RuleComp++)
    | (RuleBody <<= (RuleComp | Expr)++)
    | (ObjectItem <<= Key * Expr)[Key]
    | (Term <<= Scalar | Array | Object)
    | (RefTerm <<= Ref | Var)
    | (NumTerm <<= JSONInt | JSONFloat)
    | (RefArgBrack <<= Scalar | RefTerm)
    | (Expr <<= (RefTerm | NumTerm | Term | wf_arith_op | wf_bool_op | Expr)++[1])
    ;
  // clang-format-off

  inline const auto wf_math_tokens = RefTerm | NumTerm | UnaryExpr | ArithInfix;

  // clang-format off
  inline const auto wf_pass_multiply_divide =
    wf_pass_symbols
    | (ArithInfix <<= ArithArg * (Op >>= Multiply | Divide) * ArithArg)
    | (ArithArg <<= (Add | Subtract | Expr | wf_math_tokens)++[1])
    | (UnaryExpr <<= ArithArg)
    | (Expr <<= (NumTerm | RefTerm | Term | Add | Subtract | wf_bool_op | Expr | ArithInfix)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_add_subtract =
    wf_pass_multiply_divide
    | (ArithInfix <<= ArithArg * (Op >>= wf_arith_op) * ArithArg)
    | (ArithArg <<= Expr | wf_math_tokens)
    | (Expr <<= (NumTerm | RefTerm | Term | wf_bool_op | Expr | UnaryExpr | ArithInfix)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_comparison =
    wf_pass_add_subtract
    | (BoolInfix <<= ArithArg * (Op >>= wf_bool_op) * ArithArg)
    | (ArithArg <<= wf_math_tokens)
    | (Expr <<= (NumTerm | RefTerm | Term | UnaryExpr | ArithInfix | BoolInfix)++[1])
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_merge_data =
    wf_pass_comparison
    | (Rego <<= Query * Input * Data * ModuleSeq)
    | (Data <<= Var * ObjectItemList)[Var]
    | (Expr <<= NumTerm | RefTerm | Term | UnaryExpr | ArithInfix | BoolInfix)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_merge_modules =
    wf_pass_merge_data
    | (Rego <<= Query * Input * Data)
    | (Module <<= RuleComp++)
    | (Data <<= Var * ObjectItemList * DataModuleSeq)[Var]
    | (DataModuleSeq <<= DataModule++)
    | (DataModule <<= Var * Module)[Var]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_rules =
    wf_pass_merge_modules
    | (Query <<= Term++[1])
    | (Module <<= RuleComp++)
    | (RuleComp <<= Var * Term * (Value >>= JSONTrue | JSONFalse))[Var]
    | (Term <<= Scalar | Array | Object | Undefined)
    | (Array <<= Term++)
    | (ObjectItem <<= Key * Term)[Key]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_query =
    wf_pass_rules
    | (Top <<= Term++[1])
    ;
  // clang-format on
}