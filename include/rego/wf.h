#pragma once

#include "lang.h"
#include "trieste/token.h"

namespace rego
{
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
    | (Group <<= wf_parse_tokens++[1])
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
    wf_parse_tokens - (Package | Colon | Import);

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
    | (Group <<= wf_modules_tokens++[1])
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
    | (With <<= WithRef * WithExpr)
    | (WithRef <<= Group)
    | (WithExpr <<= Group)
    | (Group <<= wf_imports_tokens++[1])
    ;
  // clang-format off    

  inline const auto wf_keywords_tokens = wf_imports_tokens | IsIn | Contains | Every | If;

  // clang-format off
  inline const auto wf_pass_keywords =
    wf_pass_imports
    | (Group <<= wf_keywords_tokens++[1])
    ;
  // clang-format on

  inline const auto wf_lists_tokens = (wf_keywords_tokens - (Some | Every)) |
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
    | (Group <<= wf_lists_tokens++[1])
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
    | (Else <<= (Val >>= Group) * UnifyBody)
    ;
  // clang-format on

  inline const auto wf_rules_tokens = wf_lists_tokens - If;

  // clang-format off
  inline const auto wf_pass_rules =
    wf_pass_elses
    | (Policy <<= Rule++)
    | (Rule <<= (Default >>= JSONTrue | JSONFalse) * RuleHead * (Body >>= UnifyBody | Empty) * ElseSeq)
    | (RuleHead <<= RuleRef * (RuleHeadType >>= RuleHeadComp | RuleHeadFunc | RuleHeadSet | RuleHeadObj))
    | (RuleRef <<= (Var | Array | Dot)++[1])
    | (ElseSeq <<= Else++)
    | (Else <<= (Val >>= Group) * UnifyBody)
    | (RuleHeadComp <<= AssignOperator * Group)
    | (RuleHeadFunc <<= RuleArgs * AssignOperator * Group)
    | (RuleHeadSet <<= Group)
    | (RuleHeadObj <<= Group * AssignOperator * Group)
    | (RuleArgs <<= Group++[1])
    | (AssignOperator <<= wf_assign_op)
    | (Group <<= wf_rules_tokens++[1])
    ;
  // clang-format on

  inline const auto wf_call_tokens = wf_rules_tokens | ExprCall;

  // clang-format off
  inline const auto wf_pass_build_calls =
    wf_pass_rules
    | (ExprCall <<= RuleRef * ArgSeq)
    | (ArgSeq <<= Group++[1])
    | (Group <<= wf_call_tokens++[1])
    ;
  // clang-format on

  inline const auto wf_membership_tokens = (wf_call_tokens - Comma) | Membership;

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
    | (Else <<= Expr * UnifyBody)
    | (RuleHeadComp <<= AssignOperator * Expr)
    | (RuleHeadFunc <<= RuleArgs * AssignOperator * Expr)
    | (RuleHeadSet <<= Expr)
    | (RuleHeadObj <<= Expr * AssignOperator * Expr)
    | (RuleArgs <<= Term++[1])
    | (UnifyBody <<= (Literal | LiteralWith)++[1])
    | (Literal <<= Expr | SomeDecl | SomeExpr)
    | (LiteralWith <<= UnifyBody * WithSeq)
    | (WithSeq <<= With++)
    | (With <<= VarSeq * Expr)
    | (SomeDecl <<= VarSeq)
    | (SomeExpr <<= (Idx >>= Expr | Undefined) * (Item >>= Expr) * IsIn)
    | (VarSeq <<= Var++)
    | (IsIn <<= Expr | Undefined)
    | (Expr <<= wf_structure_exprs++[1])
    | (ExprCall <<= RuleRef * ArgSeq)
    | (ExprEvery <<= VarSeq * UnifyBody * IsIn)
    | (ArgSeq <<= Expr++[1])
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
    | (RuleArgs <<= (ArgVar | ArgVal)++[1])
    | (ArgVar <<= Var * Undefined)[Var]
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
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * JSONInt)[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * JSONInt)[Var]
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= Expr | Term))[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Key >>= Expr | Term) * (Val >>= Expr | Term))[Var]
    | (UnifyBody <<= (Local | Literal | LiteralWith | LiteralEnum)++[1])
    | (LiteralEnum <<= (Item >>= Var) * (ItemSeq >>= Expr))
    | (Query <<= (Body >>= UnifyBody))
    | (Local <<= Var * Undefined)[Var]
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
    | (RuleArgs <<= ArgVar++[1])
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
  inline const auto wf_pass_constants =
    wf_pass_lift_query
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | DataTerm) * JSONInt)[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | DataTerm) * JSONInt)[Var]
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
  inline const auto wf_pass_compr =
    wf_pass_locals
    | (ObjectCompr <<= Var * NestedBody)
    | (ArrayCompr <<= Var * NestedBody)
    | (SetCompr <<= Var * NestedBody)
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= (UnifyBody | DataTerm)))[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= (UnifyBody | DataTerm)))[Var]
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
  inline const auto wf_pass_skips = 
    wf_pass_merge_modules
    | (DataModule <<= (RuleComp | RuleFunc | RuleSet | RuleObj | Submodule)++)
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
    | (With <<= VarSeq * Var)
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
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * JSONInt)[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * JSONInt)[Var]
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
}