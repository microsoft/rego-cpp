#pragma once

#include "lang.h"

namespace rego
{
  using namespace wf::ops;

  inline constexpr auto wf_number = Int | Float;

  inline constexpr auto wf_literal = Int | Float | Bool | String | Null;

  inline constexpr auto wf_math_op = Add | Subtract | Multiply | Divide;

  inline constexpr auto wf_comp_op = Equals | NotEquals | LessThan |
    LessThanOrEquals | GreaterThan | GreaterThanOrEquals;

  inline constexpr auto wf_parse_tokens = wf_literal | wf_math_op | wf_comp_op |
    Package | Ident | Brace | Square | RuleSeq | Dot | Paren;

  // clang-format off
  inline constexpr auto wf_parser =
      (Top <<= Policy)
    | (Policy <<= Query * Input * DataSeq * ModuleSeq)
    | (Input <<= File | Undefined)
    | (ModuleSeq <<= File++)
    | (DataSeq <<= File++)
    | (File <<= Group)
    | (Package <<= Group)
    | (RuleSeq <<= Assign++[1])
    | (Query <<= Group)
    | (Assign <<= Group++[2])
    | (KeyValue <<= Group * Group)
    | (Brace <<= (KeyValue | Group | Assign)++)
    | (Paren <<= Group)
    | (Square <<= Group++)
    | (Group <<= wf_parse_tokens++[1])
    ;
  // clang-format on

  // clang-format off
  inline constexpr auto wf_pass_input_data =
    wf_parser
    | (DataSeq <<= Data++)
    | (Input <<= Ident * Brace)[Ident]
    | (Data <<= Brace)
    ;
  // clang-format on

  inline constexpr auto wf_modules_tokens =
    wf_literal | wf_math_op | wf_comp_op | Paren | Ident | Brace | Square | Dot;

  // clang-format off
  inline constexpr auto wf_pass_modules =
    wf_pass_input_data
    | (ModuleSeq <<= Module++)
    | (Module <<= Ident * RuleSeq)[Ident]
    | (Group <<= wf_modules_tokens++[1])
    ;
  // clang-format on

  // clang-format off
  inline constexpr auto wf_pass_lists =
    wf_pass_modules
    | (Brace <<= KeyValueList | BraceList)
    | (KeyValueList <<= KeyValue++)
    | (Square <<= SquareList)
    | (SquareList <<= Group++)
    | (BraceList <<= (Group | Assign)++)
    ;
  // clang-format on

  inline constexpr auto wf_structure_tokens =
    wf_literal | Array | Ref | Object | Expression;

  // clang-format off
  inline constexpr auto wf_pass_structure =
    wf_pass_lists
    | (Query <<= wf_structure_tokens)
    | (Input <<= Ident * TopKeyValueList)[Ident]
    | (Data <<= TopKeyValueList)
    | (TopKeyValueList <<= TopKeyValue++)
    | (TopKeyValue <<= Key * (Value >>= wf_structure_tokens))[Key]
    | (RuleSeq <<= Rule++)
    | (Rule <<= Ident * RuleValue * RuleBody)[Ident]
    | (Object <<= KeyValue++)
    | (KeyValue <<= Key * (Value >>= wf_structure_tokens))[Key]
    | (Array <<= wf_structure_tokens++)
    | (Ref <<= Lookup | Local)
    | (Lookup <<= (Lhs >>= Lookup|Ident) * (Rhs >>= Ident|Index))
    | (Index <<= Expression | Int | Ref)
    | (RuleValue <<= wf_structure_tokens)
    | (RuleBody <<= (wf_structure_tokens | Rule)++)
    | (Expression <<= (wf_math_op | wf_comp_op | wf_number | Ref | Expression)++[1])
    ;
  // clang-format on

  inline constexpr auto wf_math_tokens =
    wf_comp_op | wf_number | Ref | Math | Expression;

  // clang-format off
  inline constexpr auto wf_pass_multiply_divide =
    wf_pass_structure
    | (Math <<= (Op >>= Multiply | Divide) * Expression * Expression)
    | (Expression <<= (Add | Subtract | wf_math_tokens)++[1])
    ;
  // clang-format on

  // clang-format off
  inline constexpr auto wf_pass_add_subtract =
    wf_pass_multiply_divide
    | (Math <<= (Op >>= wf_math_op) * Expression * Expression)
    | (Expression <<= (Subtract | wf_math_tokens)++[1])
    ;
  // clang-format on

  inline constexpr auto wf_comp_tokens = wf_number | Ref | Math | Comparison;

  // clang-format off
  inline constexpr auto wf_pass_comparison =
    wf_pass_add_subtract
    | (Comparison <<= (Op >>= wf_comp_op) * Expression * Expression)
    | (Expression <<= wf_comp_tokens)
    ;
  // clang-format on

  // clang-format off
  inline constexpr auto wf_pass_merge_data =
    wf_pass_comparison
    | (Policy <<= Query * Input * Data * ModuleSeq)
    | (Data <<= Ident * TopKeyValueList)[Ident]
    ;
  // clang-format on

  // clang-format off
  inline constexpr auto wf_pass_merge_modules =
    wf_pass_merge_data
    | (Policy <<= Query * Input * Data)
    | (Module <<= Rule++)
    | (TopKeyValue <<= Key * (Value >>= wf_structure_tokens | Module))[Key]
    ;
  // clang-format on

  inline constexpr auto wf_rules_tokens =
    wf_literal | Array | Object | Undefined;

  // clang-format off
  inline constexpr auto wf_pass_rules =
    wf_pass_merge_modules
    | (Query <<= wf_rules_tokens)
    | (Rule <<= Ident * (Value >>= wf_rules_tokens) * RuleSeq)[Ident]
    | (KeyValue <<= Key * (Value >>= wf_rules_tokens))[Key]
    | (TopKeyValue <<= Key * (Value >>= wf_rules_tokens | Module))[Key]
    | (Array <<= wf_rules_tokens++)
    ;
  // clang-format on

  // clang-format off
  inline constexpr auto wf_pass_convert_modules =
    wf_pass_rules
    | (Query <<= wf_rules_tokens)
    | (TopKeyValue <<= Key * (Value >>= wf_rules_tokens))[Key]
    | (Array <<= wf_rules_tokens++)
    ;
  // clang-format on

  // clang-format off
  inline constexpr auto wf_pass_query =
    wf_pass_convert_modules
    | (Top <<= wf_rules_tokens)
    ;
  // clang-format on
}