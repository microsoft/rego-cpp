// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "rego_c.h"
#include "trieste/logging.h"
#include "trieste/token.h"

#include <initializer_list>
#include <trieste/trieste.h>

/// This namespace provides the C++ API for the library.
/// It includes all the token types for nodes in the AST, the well-formedness
/// definitions for each pass, the built-in system and custom types for
/// handling various kinds of data (e.g. the BigInt class).
namespace rego
{
  using namespace trieste;
  using namespace wf::ops;

  /// @cond
  inline const auto Module =
    TokenDef("rego-module", flag::lookup | flag::symtab);
  inline const auto Package = TokenDef("rego-package");
  inline const auto Policy = TokenDef("rego-policy");
  inline const auto Rule = TokenDef(
    "rego-rule",
    flag::lookup | flag::lookdown | flag::symtab | flag::defbeforeuse);
  inline const auto RuleHead = TokenDef("rego-rulehead");
  inline const auto RuleHeadComp = TokenDef("rego-ruleheadcomp");
  inline const auto RuleHeadFunc = TokenDef("rego-ruleheadfunc");
  inline const auto RuleHeadSet = TokenDef("rego-ruleheadset");
  inline const auto RuleHeadObj = TokenDef("rego-ruleheadobj");
  inline const auto RuleArgs = TokenDef("rego-ruleargs");
  inline const auto Query = TokenDef("rego-query");
  inline const auto Literal = TokenDef("rego-literal");
  inline const auto Expr = TokenDef("rego-expr");
  inline const auto ExprInfix = TokenDef("rego-exprinfix");
  inline const auto ExprCall = TokenDef("rego-exprcall");
  inline const auto ExprEvery = TokenDef("rego-exprevery", flag::symtab);
  inline const auto ExprParens = TokenDef("rego-exprparens");
  inline const auto UnaryExpr = TokenDef("rego-unaryexpr");
  inline const auto NotExpr = TokenDef("rego-not-expr");
  inline const auto Term = TokenDef("rego-term");
  inline const auto InfixOperator = TokenDef("rego-infixoperator");
  inline const auto BoolOperator = TokenDef("rego-booloperator");
  inline const auto ArithOperator = TokenDef("rego-arithoperator");
  inline const auto AssignOperator = TokenDef("rego-assignoperator");
  inline const auto BinOperator = TokenDef("rego-binoperator");
  inline const auto Ref = TokenDef("rego-ref");
  inline const auto RefArgBrack = TokenDef("rego-refargbrack");
  inline const auto RefArgDot = TokenDef("rego-refargdot");
  inline const auto Var = TokenDef("rego-var", flag::print);
  inline const auto Scalar = TokenDef("rego-scalar");
  inline const auto String = TokenDef("rego-string");
  inline const auto Array = TokenDef("rego-array");
  inline const auto Object = TokenDef("rego-object");
  inline const auto Set = TokenDef("rego-set");
  inline const auto ObjectItem = TokenDef("rego-objectitem");
  inline const auto RawString = TokenDef("rego-rawstring", flag::print);
  inline const auto JSONString = TokenDef("rego-STRING", flag::print);
  inline const auto Int = TokenDef("rego-INT", flag::print);
  inline const auto Float = TokenDef("rego-FLOAT", flag::print);
  inline const auto True = TokenDef("rego-true");
  inline const auto False = TokenDef("rego-false");
  inline const auto Null = TokenDef("rego-null");
  inline const auto Equals = TokenDef("rego-equals");
  inline const auto NotEquals = TokenDef("rego-notequals");
  inline const auto LessThan = TokenDef("rego-lessthan");
  inline const auto GreaterThan = TokenDef("rego-greaterthan");
  inline const auto LessThanOrEquals = TokenDef("rego-lessthanorequals");
  inline const auto GreaterThanOrEquals = TokenDef("rego-greaterthanorequals");
  inline const auto Add = TokenDef("rego-add");
  inline const auto Subtract = TokenDef("rego-subtract");
  inline const auto Multiply = TokenDef("rego-multiply");
  inline const auto Divide = TokenDef("rego-divide");
  inline const auto Modulo = TokenDef("rego-modulo");
  inline const auto And = TokenDef("rego-and");
  inline const auto Or = TokenDef("rego-or");
  inline const auto Assign = TokenDef("rego-assign");
  inline const auto Unify = TokenDef("rego-unify");
  inline const auto Default = TokenDef("rego-default");
  inline const auto Some = TokenDef("rego-some");
  inline const auto SomeDecl = TokenDef("rego-somedecl");
  inline const auto If = TokenDef("rego-if");
  inline const auto IsIn = TokenDef("rego-in");
  inline const auto Contains = TokenDef("rego-contains");
  inline const auto Else = TokenDef("rego-else");
  inline const auto As = TokenDef("rego-as");
  inline const auto With = TokenDef("rego-with");
  inline const auto Every = TokenDef("rego-every");
  inline const auto ArrayCompr = TokenDef("rego-arraycompr", flag::symtab);
  inline const auto ObjectCompr = TokenDef("rego-objectcompr", flag::symtab);
  inline const auto SetCompr = TokenDef("rego-setcompr", flag::symtab);
  inline const auto Membership = TokenDef("rego-membership");
  inline const auto Not = TokenDef("rego-not");
  inline const auto Import =
    TokenDef("rego-import", flag::lookdown | flag::lookup | flag::shadowing);
  inline const auto Placeholder = TokenDef("rego-placeholder");
  inline const auto Version = TokenDef("rego-version", flag::print);

  // intermediate tokens
  inline const auto RuleBodySeq = TokenDef("rego-rulebodyseq");
  inline const auto ImportSeq = TokenDef("rego-importseq");
  inline const auto RuleRef = TokenDef("rego-ruleref");
  inline const auto RuleHeadType = TokenDef("rego-ruleheadtype");
  inline const auto WithSeq = TokenDef("rego-withseq");
  inline const auto TermSeq = TokenDef("rego-termseq");
  inline const auto ExprSeq = TokenDef("rego-exprseq");
  inline const auto VarSeq = TokenDef("rego-varseq");
  inline const auto RefHead = TokenDef("rego-refhead");
  inline const auto RefArgSeq = TokenDef("rego-refargseq");
  inline const auto Key = TokenDef("rego-key", flag::print);
  inline const auto Val = TokenDef("rego-value");
  inline const auto Undefined = TokenDef("rego-undefined");
  inline const auto Name = TokenDef("rego-name");
  inline const auto Return = TokenDef("rego-return");
  inline const auto Target = TokenDef("rego-target");
  inline const auto Lhs = TokenDef("rego-lhs");
  inline const auto Rhs = TokenDef("rego-rhs");
  inline const auto Src = TokenDef("rego-src");
  inline const auto Args = TokenDef("rego-args");
  inline const auto Idx = TokenDef("rego-idx");
  inline const auto Row = TokenDef("rego-row");
  inline const auto Col = TokenDef("rego-col");

  // other tokens
  inline const auto Results = TokenDef("rego-results", flag::symtab);
  inline const auto Result = TokenDef("rego-result", flag::symtab);
  inline const auto Bindings = TokenDef("rego-bindings");
  inline const auto Terms = TokenDef("rego-terms");
  inline const auto Binding = TokenDef("rego-binding", flag::lookdown);
  inline const auto ErrorCode = TokenDef("rego-errorcode");
  inline const auto ErrorSeq = TokenDef("rego-errorseq");
  inline const auto Line = TokenDef("rego-line", flag::print);

  inline const auto wf_assign_op = Assign | Unify;
  inline const auto wf_arith_op = Add | Subtract | Multiply | Divide | Modulo;
  inline const auto wf_bin_op = And | Or | Subtract;
  inline const auto wf_bool_op = Equals | NotEquals | LessThan |
    LessThanOrEquals | GreaterThan | GreaterThanOrEquals | Not;
  inline const auto wf_exprs =
    Term | ExprCall | ExprInfix | ExprEvery | ExprParens | UnaryExpr;

  // clang-format off
  inline const auto wf =
      (Top <<= Query | Module)
    | (Module <<= Package * Version * ImportSeq * Policy)
    | (ImportSeq <<= Import++)
    | (Import <<= Ref * Var)
    | (Package <<= Ref)
    | (Policy <<= Rule++)
    | (Rule <<= (Default >>= True | False) * RuleHead * RuleBodySeq)
    | (RuleHead <<= RuleRef * (RuleHeadType >>= (RuleHeadSet | RuleHeadObj | RuleHeadFunc | RuleHeadComp)))
    | (RuleRef <<= Var | Ref)
    | (RuleHeadComp <<= Expr)
    | (RuleHeadObj <<= Expr * Expr)
    | (RuleHeadFunc <<= RuleArgs * Expr)
    | (RuleHeadSet <<= Expr)
    | (RuleArgs <<= Term++)
    | (RuleBodySeq <<= (Else | Query)++)
    | (Else <<= Expr * Query)
    | (Query <<= Literal++)
    | (Literal <<= (Expr >>= SomeDecl | Expr | NotExpr) * WithSeq)
    | (WithSeq <<= With++)
    | (With <<= Term * Expr)
    | (SomeDecl <<= ExprSeq * (IsIn >>= Expr | Undefined))
    | (NotExpr <<= Expr)
    | (Expr <<= (Term | ExprCall | ExprInfix | ExprEvery | ExprParens | UnaryExpr))
    | (ExprCall <<= Ref * ExprSeq)
    | (ExprSeq <<= Expr++)
    | (ExprInfix <<= Expr * InfixOperator * Expr)
    | (ExprEvery <<= VarSeq * (IsIn >>= Term | ExprCall | ExprInfix) * Query)
    | (VarSeq <<= Var++[1])
    | (ExprParens <<= Expr)
    | (UnaryExpr <<= Expr)
    | (Membership <<= ExprSeq * Expr)
    | (Term <<= Ref | Var | Scalar | Array | Object | Set | Membership | ArrayCompr | ObjectCompr | SetCompr)
    | (ArrayCompr <<= Expr * Query)
    | (SetCompr <<= Expr * Query)
    | (ObjectCompr <<= Expr * Expr * Query)
    | (InfixOperator <<= AssignOperator | BoolOperator | ArithOperator | BinOperator)
    | (BoolOperator <<= Equals | NotEquals | LessThan | GreaterThan | LessThanOrEquals | GreaterThanOrEquals)
    | (ArithOperator <<= Add | Subtract | Multiply | Divide | Modulo)
    | (BinOperator <<= And | Or)
    | (AssignOperator <<= Assign | Unify)
    | (Ref <<= RefHead * RefArgSeq)
    | (RefHead <<= Var | Array | Object | Set | ArrayCompr | ObjectCompr | SetCompr | ExprCall)
    | (RefArgSeq <<= (RefArgDot | RefArgBrack)++)
    | (RefArgBrack <<= Expr | Placeholder)
    | (RefArgDot <<= Var)
    | (Scalar <<= String | Int | Float | True | False | Null)
    | (String <<= JSONString | RawString)
    | (Array <<= Expr++)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= (Key >>= Expr) * (Val >>= Expr))
    | (Set <<= Expr++)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_result =
    (Top <<= Results | Undefined)
    | (Results <<= Result++[1])
    | (Result <<= Terms * Bindings)
    | (Terms <<= Term++)
    | (Bindings <<= Binding++)
    | (Binding <<= (Key >>= Var) * (Val >>= Term))[Key]
    | (Term <<= Scalar | Array | Object | Set)
    | (Array <<= Term++)
    | (Set <<= Term++)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= (Key >>= Term) * (Val >>= Term))
    | (Scalar <<= JSONString | Int | Float | True | False | Null)
    | (Error <<= ErrorMsg * ErrorAst * ErrorCode)
    ;
  // clang-format on

  /// @endcond

  struct BuiltInDef;

  /// @brief A pointer to a BuiltInDef.
  using BuiltIn = std::shared_ptr<BuiltInDef>;

  /// @brief The function pointer to the behavior of the built-in.
  using BuiltInBehavior = std::function<Node(const Nodes&)>;

  /// @brief Big Integer implemention based on strings.
  /// @details
  //  This is a basic, non-optimized implementation of a big integer
  /// using strings. In most circumstances this would be considerably slower
  /// than other approaches, but given the way in which Trieste nodes store
  /// their content as Location objects into a source document, and given how
  /// this class operates over those Locations, it is actually quite efficient
  /// when compared to parsing and serializing the Location into a vector of
  /// unsigned longs.
  class BigInt
  {
  public:
    /// @brief Default constructor, initializes to zero.
    BigInt();
    /// @brief Constructs a BigInt from a Location representing a arbitrary
    /// precision integer string.
    /// @param value The Location to construct from.
    BigInt(const Location& value);
    /// @brief Constructs a BigInt from a 64-bit integer.
    /// @param value The 64-bit integer to construct from.
    BigInt(std::int64_t value);
    /// @brief Constructs a BigInt from a size_t integer.
    /// @param value The size_t integer to construct from.
    BigInt(std::size_t value);

    /// @brief Gets the Location representing the integer string.
    /// @return The Location representing the integer string.
    const Location& loc() const;
    /// @brief Attempts to convert the BigInt to a 64-bit integer.
    /// @return The 64-bit integer representation of the BigInt, or std::nullopt
    /// if out of range.
    std::optional<std::int64_t> to_int() const;
    /// @brief  Attempts to convert the BigInt to a size_t integer.
    /// @return The size_t integer representation of the BigInt, or std::nullopt
    /// if out of range.
    std::optional<std::size_t> to_size() const;

    /// @brief Checks if the BigInt is negative.
    /// @return True if the BigInt is negative, false otherwise.
    bool is_negative() const;

    /// @brief Checks if the BigInt is zero.
    /// @return True if the BigInt is zero, false otherwise.
    bool is_zero() const;

    /// @brief Increments the BigInt by one.
    /// @return A new BigInt representing the incremented value.
    BigInt increment() const;
    /// @brief Decrements the BigInt by one.
    /// @return A new BigInt representing the decremented value.
    BigInt decrement() const;
    /// @brief Gets the absolute value of the BigInt.
    /// @return A new BigInt representing the absolute value.
    BigInt abs() const;
    /// @brief Negates the BigInt.
    /// @return A new BigInt representing the negated value.
    BigInt negate() const;

    /// @brief Checks if a Location represents a valid integer string.
    /// @param loc The Location to check.
    /// @return True if the Location represents a valid integer string, false
    /// otherwise.
    static bool is_int(const Location& loc);

    /// @brief Adds two BigInts.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return The sum of the two BigInts.
    friend BigInt operator+(const BigInt& lhs, const BigInt& rhs);

    /// @brief Subtracts two BigInts.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return The difference of the two BigInts.
    friend BigInt operator-(const BigInt& lhs, const BigInt& rhs);

    /// @brief Multiplies two BigInts.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return The product of the two BigInts.
    friend BigInt operator*(const BigInt& lhs, const BigInt& rhs);

    /// @brief Divides two BigInts.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return The quotient of the two BigInts.
    friend BigInt operator/(const BigInt& lhs, const BigInt& rhs);

    /// @brief Computes the remainder of dividing two BigInts.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return The remainder of the two BigInts.
    friend BigInt operator%(const BigInt& lhs, const BigInt& rhs);

    /// @brief Compares two BigInts for greater-than.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return True if lhs is greater than rhs, false otherwise.
    friend bool operator>(const BigInt& lhs, const BigInt& rhs);

    /// @brief Compares two BigInts for less-than.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return True if lhs is less than rhs, false otherwise.
    friend bool operator<(const BigInt& lhs, const BigInt& rhs);

    /// @brief Compares two BigInts for less-than-or-equal.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return True if lhs is less than or equal to rhs, false otherwise.
    friend bool operator<=(const BigInt& lhs, const BigInt& rhs);

    /// @brief Compares two BigInts for greater-than-or-equal.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return True if lhs is greater than or equal to rhs, false otherwise.
    friend bool operator>=(const BigInt& lhs, const BigInt& rhs);

    /// @brief Compares two BigInts for equality.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return True if lhs is equal to rhs, false otherwise.
    friend bool operator==(const BigInt& lhs, const BigInt& rhs);

    /// @brief Compares two BigInts for inequality.
    /// @param lhs The left-hand side operand.
    /// @param rhs The right-hand side operand.
    /// @return True if lhs is not equal to rhs, false otherwise.
    friend bool operator!=(const BigInt& lhs, const BigInt& rhs);

    /// @brief Outputs a BigInt to a stream.
    /// @param os The output stream.
    /// @param value The BigInt value.
    /// @return The output stream.
    friend std::ostream& operator<<(std::ostream& os, const BigInt& value);

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

  /// @brief Result of unwrapping a node.
  struct UnwrapResult
  {
    /// @brief The unwrapped argument or nullptr (if unsuccessful).
    Node node;

    /// @brief True if the argument was unwrapped successfully.
    bool success;
  };

  /// @brief Options for unwrapping an argument.
  /// @details
  /// This struct provides options for unwrapping an argument and producing
  /// an error message.
  /// The act of unwrapping a node is the process of testing whether a node
  /// or one of its direct descendents is of one or more types. So, for example,
  /// the following nodes:
  ///
  /// ```
  /// (term (scalar (int 5)))
  /// (scalar (int 5))
  /// (int 5)
  /// ```
  /// Would all be successfully unwrapped as `(int 5)` if the type Int was
  /// specified. However, if Float was specified, the result would be an
  /// error node.
  class UnwrapOpt
  {
  public:
    /// @brief Construct an UnwrapOpt.
    ///
    /// @param index The index of the argument to unwrap.
    UnwrapOpt(std::size_t index);

    /// @brief Whether the statement indicating what was received instead
    /// of the expected type should be excluded.
    bool exclude_got() const;

    /// @brief Sets whether to exclude the "got" statement in the error message.
    /// @param exclude_got True to exclude the "got" statement, false to include
    /// it.
    /// @return A reference to this UnwrapOpt.
    UnwrapOpt& exclude_got(bool exclude_got);

    /// @brief Whether to specify in the error message which kind of number
    /// was received (i.e. integer or floating point)
    bool specify_number() const;

    /// @brief Sets whether to specify in the error message which kind of number
    /// was received.
    /// @param specify_number True to specify the kind of number, false to omit
    /// it.
    /// @return A reference to this UnwrapOpt.
    UnwrapOpt& specify_number(bool specify_number);

    /// @brief The error code for the error message.
    /// Default value if omitted is EvalTypeError.
    const std::string& code() const;

    /// @brief Sets the error code for the error message.
    /// @param value The error code.
    /// @return A reference to this UnwrapOpt.
    UnwrapOpt& code(const std::string& value);

    /// @brief The error preamble.
    /// If omitted, a default preamble will be constructed
    /// from the operation metadata in the form "operand <i> must be <t>".
    const std::string& pre() const;

    /// @brief Sets the error preamble.
    /// @param value The error preamble.
    /// @return A reference to this UnwrapOpt.
    UnwrapOpt& pre(const std::string& value);

    /// @brief The full error message.
    /// If this is set, no message will be generated and instead
    /// this will be returned verbatim.
    const std::string& message() const;

    /// @brief Sets the full error message.
    /// If this is set, no message will be generated and instead
    /// this will be returned verbatim.
    /// @param value The full error message.
    /// @return A reference to this UnwrapOpt.
    UnwrapOpt& message(const std::string& value);

    /// @brief The name of the function.
    /// If provide, will be a prefix on the message as "<func-name>:"
    /// @return A reference to this UnwrapOpt.
    const std::string& func() const;

    /// @brief Sets the name of the function.
    /// @param value The name of the function.
    /// @return A reference to this UnwrapOpt.
    UnwrapOpt& func(const std::string& value);

    /// @brief The types to match against.
    /// The operand must be one of the provided types or else an
    /// error node will be returned.
    /// @return A vector of require types
    const std::vector<Token>& types() const;

    /// @brief Sets the types to match against.
    /// @note If only a single type is provided, the type() interface will be
    /// used instead.
    /// @param value The types to match against.
    /// @return A reference to this UnwrapOpt.
    UnwrapOpt& types(const std::vector<Token>& value);

    /// @brief The singular type to match against.
    /// @return A reference to the type.
    const Token& type() const;

    /// @brief Sets the singular type to match against.
    /// @note Setting this will clear the UnwrapOpt::types() vector.
    /// @param value The singular type to match against.
    /// @return A reference to this UnwrapOpt.
    UnwrapOpt& type(const Token& value);

    /// @brief Unwraps an argument from the provided vector of nodes.
    /// @param args The vector of nodes to unwrap from.
    /// @return The unwrapped argument or an appropriate error node.
    Node unwrap(const Nodes& args) const;

  private:
    bool m_exclude_got;
    bool m_specify_number;
    std::string m_code;
    std::string m_prefix;
    std::string m_message;
    std::string m_func;
    std::vector<Token> m_types;
    Token m_type;
    std::size_t m_index;
  };

  /// @brief Unwraps an argument from the provided vector of nodes.
  /// @param args The vector of nodes to unwrap from.
  /// @param options The options for unwrapping.
  /// @return The unwrapped argument or an appropriate error node.
  Node unwrap_arg(const Nodes& args, const UnwrapOpt& options);

  /// @brief Attempts to unwrap a node to a specified type.
  /// @param term The term to unwrap.
  /// @param type The acceptable type.
  /// @return An unwrap result or an appropriate error node.
  UnwrapResult unwrap(const Node& term, const Token& type);

  /// @brief Attempts to unwrap an argument to a specified type.
  /// @param term The term to unwrap.
  /// @param types The acceptable types.
  /// @return An unwrap result or an appropriate error node.
  UnwrapResult unwrap(const Node& term, const std::set<Token>& types);

  /// @brief Extracts the value of a node as an integer
  /// @param node The node to extract from.
  /// @return The integer value of the node.
  BigInt get_int(const Node& node);

  /// @brief Extracts the value of a node as an double
  /// @param node The node to extract from.
  /// @return The double value of the node.
  double get_double(const Node& node);

  /// @brief Attempts to extract the value of a node as an integer.
  /// In the case that the node is a double, it will check if the
  /// double is integral, i.e. 5.0 will be considered the integer 5.
  /// @param node The node to extract from.
  /// @return The integer value of the node
  std::optional<BigInt> try_get_int(const Node& node);

  /// @brief Extracts the value of a node as a string.
  /// The resulting string will have any enclosing quotes removed.
  /// @param node The node to extract from.
  /// @return The string value of the node.
  std::string get_string(const Node& node);

  /// @brief Extracts the value of a node as a boolean.
  /// @param node The node to extract from.
  /// @return The boolean value of the node.
  bool get_bool(const Node& node);

  /// @brief Tries to get an item within an object using its key.
  /// @param node The object to search.
  /// @param key The key to search for.
  /// @return The item node if found, otherwise std::nullopt.
  std::optional<Node> try_get_item(
    const Node& node, const std::string_view& key);

  /// @brief Converts the value to a scalar node.
  /// @param value The value to convert.
  /// @return The scalar node (scalar (INT <value>)))
  Node scalar(BigInt value);

  /// @brief Converts the value to a scalar node.
  /// @param value The value to convert.
  /// @return The scalar node (scalar (FLOAT <value>)))
  Node scalar(double value);

  /// @brief Converts the value to a scalar node.
  /// @param value The value to convert.
  /// @return The scalar node (scalar (TRUE|FALSE)))
  Node scalar(bool value);

  /// @brief Converts the value to a scalar node.
  /// @param value The value to convert.
  /// @return The scalar node (scalar (STRING <value>)))
  Node scalar(const char* value);

  /// @brief Converts the value to a scalar node.
  /// @param value The value to convert.
  /// @return The scalar node (scalar (STRING <value>)))
  Node scalar(const std::string& value);

  /// @brief Creates a null scalar.
  /// @return The scalar node (scalar (NULL)))
  Node scalar();

  /// @brief Converts the key and val terms to an object item.
  /// @param key_term The key term.
  /// @param val_term The value term.
  /// @return The object item node (object_item (key_term) (val_term))
  Node object_item(const Node& key_term, const Node& val_term);

  /// @brief Converts the value to an object node.
  /// @param object_items The object items of the object.
  /// @return The object node (object (object_item) (object_item) ...)
  Node object(const std::initializer_list<Node>& object_items);

  /// @brief Converts the value to an array node.
  /// @param array_members The members of the array.
  /// @return The array node (array (term) (term) ...)
  Node array(const std::initializer_list<Node>& array_members);

  /// @brief Converts the value to a set node.
  /// @param set_members The members of the set.
  /// @return The set node (set (term) (term) ...)
  Node set(const std::initializer_list<Node>& set_members);

  namespace builtins
  {
    /// @cond
    const std::size_t AnyArity = std::numeric_limits<std::size_t>::max();

    inline const auto Decl = TokenDef("rego-builtin-decl");
    inline const auto Arg = TokenDef("rego-builtin-arg");
    inline const auto Args = TokenDef("rego-builtin-args");
    inline const auto VarArgs = TokenDef("rego-builtin-varargs");
    inline const auto Result = TokenDef("rego-builtin-result");
    inline const auto Void = TokenDef("rego-builtin-void");
    inline const auto Return = TokenDef("rego-builtins-return");
    inline const auto Name = TokenDef("rego-builtin-name", flag::print);
    inline const auto Description =
      TokenDef("rego-builtin-description", flag::print);
    inline const auto Type = TokenDef("rego-builtin-type");
    inline const auto ArgSeq = TokenDef("rego-builtin-argseq");
    inline const auto MemberType = TokenDef("rego-builtin-membertype");
    inline const auto TypeSeq = TokenDef("rego-builtin-typeseq");
    inline const auto DynamicArray = TokenDef("rego-builtin-dynamicarray");
    inline const auto StaticArray = TokenDef("rego-builtin-staticarray");
    inline const auto Set = TokenDef("rego-builtin-set");
    inline const auto DynamicObject = TokenDef("rego-builtin-dynamicobject");
    inline const auto StaticObject = TokenDef("rego-builtin-staticobject");
    inline const auto HybridObject = TokenDef("rego-builtin-hybridobject");
    inline const auto ObjectItem = TokenDef("rego-builtin-objectitem");
    inline const auto Any = TokenDef("rego-builtin-any");
    inline const auto String = TokenDef("rego-builtin-string");
    inline const auto Number = TokenDef("rego-builtin-number");
    inline const auto Boolean = TokenDef("rego-builtin-boolean");
    inline const auto Null = TokenDef("rego-builtin-null");

    // clang-format off
    inline const auto wf_decl =
      (Decl <<= (Args >>= ArgSeq | VarArgs) * (Return >>= Result | Void))
      | (ArgSeq <<= Arg++)
      | (Arg <<= Name * Description * Type)
      | (Type <<= Any | String | Number | Boolean | Null | DynamicArray | StaticArray | DynamicObject | StaticObject | HybridObject | Set | TypeSeq)
      | (TypeSeq <<= Type++)
      | (DynamicArray <<= Type)
      | (StaticArray <<= Type++)
      | (Set <<= Type)
      | (DynamicObject <<= (Key >>= Type) * (Val >>= Type))
      | (StaticObject <<= ObjectItem++)
      | (HybridObject <<= DynamicObject * StaticObject)
      | (ObjectItem <<= Name * Type)      
      | (Result <<= Name * Description * Type)
      ;
    // clang-format on
    /// @endcond
  }

  /// @brief Struct which defines a built-in function.
  /// @details
  /// You can extend Rego by registering your own built-ins. A built-in is a
  /// function which is called by Rego during evaluation. Built-ins are called
  /// with a vector of Nodes, and return a Node. The vector of Nodes contains
  /// the arguments passed to the built-in. The Node returned by the built-in
  /// is the result of the built-in's evaluation.
  ///
  /// Here is an example built-in which performs addition:
  ///
  /// ```cpp
  /// Node add(const Nodes& args)
  /// {
  ///   Node a = unwrap_arg(args, UnwrapOpt(0).types({Int, Float}));
  ///   if(a.type() == Error){
  ///     return a;
  ///   }
  ///
  ///   Node b = unwrap_arg(args, UnwrapOpt(1).types({Int, Float}));
  ///   if(b.type() == Error){
  ///     return b;
  ///   }
  ///
  ///   if(a.type() == Int && b.type() == Int)
  ///   {
  ///     return scalar(get_int(a) + get_int(b));
  ///   }
  ///
  ///   return scalar(get_double(a) + get_double(b));
  /// }
  ///
  /// Node add_decl =
  ///   bi::Decl << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "a") <<
  ///   bi::Description
  ///                                       << (bi::Type << bi::Number))
  ///                           << (bi::Arg << (bi::Name ^ "b") <<
  ///                           bi::Description
  ///                                       << (bi::Type << bi::Number)))
  ///            << (bi::Result << (bi::Name ^ "result")
  ///                           << (bi::Description ^ "The sum of `a` and `b`.")
  ///                           << (bi::Type << bi::Number));
  /// ```
  ///
  /// Note that there are several helper methods and objects to aid in
  /// writing managing nodes and wrapping/unwrapping them into basic
  /// types. Once a method like the above has been written, use
  /// BuiltInDef::create following way:
  /// ```cpp
  /// builtins.register_builtin(BuiltInDef::create(Location("myadd"), add_decl,
  /// add))
  /// ```
  ///
  /// Then, during evaluation, any call to the built-in `myadd` will be
  /// handled by the `myadd` function.
  struct BuiltInDef
  {
    /// @brief The name used to match against expression calls in the rego
    /// program.
    Location name;

    /// @brief The declaration node which adheres to the builtins::wf_decl
    /// well-formedness definition.
    Node decl;

    /// @brief The number of expected arguments.
    ///
    /// If any number of arguments can be provided, use the constant AnyArity.
    std::size_t arity;

    /// @brief The function which will be called when the built-in is evaluated.
    BuiltInBehavior behavior;

    /// @brief Whether the builtin is available.
    bool available;

    /// @brief Constructor.
    BuiltInDef(
      Location name_, Node decl_, BuiltInBehavior behavior_, bool available_);

    virtual ~BuiltInDef() = default;

    /// @brief Called to clear any persistent state or caching.
    virtual void clear();

    /// @brief Creates a new built-in.
    /// @details
    /// The `decl` node adheres to the `builtins::wf_decl` well-formedness
    /// definition. BuiltIn is a pointer to a BuiltInDef.
    /// @param name The name of the built-in.
    /// @param decl Metadata and documentation for the built-in.
    /// @param behavior The function which will be called when the built-in is
    /// evaluated.
    /// @return The built-in.
    static BuiltIn create(
      const Location& name, Node decl, BuiltInBehavior behavior);

    /// @brief Creates a new built-in.
    /// @details
    /// An empty decl node with the correct arity and a return type of `Any`
    /// will be automatically created from the arity argument.    ///
    /// @deprecated
    /// @param name The name of the built-in.
    /// @param arity Number of arguments to the builtin
    /// @param behavior The function which will be called when the built-in is
    /// evaluated.
    /// @return The built-in.
    static BuiltIn create(
      const Location& name, size_t arity, BuiltInBehavior behavior);

    /// @brief Creates a placeholder for a built-in which is not available on
    /// this platform.
    ///
    /// The `decl` node adheres to the `builtins::wf_decl` well-formedness
    /// definition. BuiltIn is a pointer to a BuiltInDef.
    ///
    /// @param name The name of the built-in.
    /// @param decl Metadata and documentation for the built-in.
    /// @param message The message to include in the error when the built-in is
    /// called.
    /// @return The built-in.
    static BuiltIn placeholder(
      const Location& name, Node decl, const std::string& message);
  };

  /// @brief Manages the set of builtins used by an interpreter to resolve
  /// built-in calls.
  class BuiltInsDef
  {
  public:
    /// @brief Constructor.
    BuiltInsDef() noexcept;

    /// @brief Determines whether the provided name refers to a built-in.
    /// @param name The name to check.
    /// @return True if the name refers to a built-in, otherwise false.
    bool is_builtin(const Location& name) const;

    /// @brief Gets the declaration node for the specified built-in.
    /// @param name The name of the built-in.
    /// @return The declaration node for the built-in, or an empty node if not
    /// found.
    Node decl(const Location& name) const;

    /// @brief Determines whether the provided builtin name is deprecated in the
    /// provided version.
    /// @param version The version to check.
    /// @param name The name to check.
    /// @return True if the builtin is deprecated in the specified version,
    /// otherwise false.
    bool is_deprecated(const Location& version, const Location& name) const;

    /// @brief Calls the built-in with the provided name and arguments.
    /// @param name The name of the built-in to call.
    /// @param version The Rego version.
    /// @param args The arguments to pass to the built-in.
    /// @return The result of the built-in call.
    Node call(const Location& name, const Location& version, const Nodes& args);

    /// @brief Called to clear any persistent state or caching.
    void clear();

    /// @brief Registers a built-in.
    /// @param built_in The built-in to register.
    /// @return A reference to this instance.
    BuiltInsDef& register_builtin(const BuiltIn& built_in);

    /// @brief Gets the built-in with the provided name.
    /// @param name The name of the built-in to retrieve.
    /// @return The built-in with the specified name.
    const BuiltIn& at(const Location& name) const;

    /// @brief Whether to throw built-in errors.
    /// @return True if built-in errors will be thrown as exceptions, false
    /// otherwise. If true, built-in errors will be thrown as exceptions. If
    /// false, built-in errors will result in Undefined nodes.
    bool strict_errors() const;

    /// @brief Sets whether to throw built-in errors.
    /// @param strict_errors True to throw built-in errors as exceptions, false
    /// to return Undefined nodes.
    /// @return A reference to this instance.
    BuiltInsDef& strict_errors(bool strict_errors);

    /// @brief Registers a set of built-ins.
    /// @param built_ins The built-ins to register.
    /// @return A reference to this instance.
    template <typename T>
    BuiltInsDef& register_builtins(const T& built_ins)
    {
      for (auto& built_in : built_ins)
      {
        register_builtin(built_in);
      }

      return *this;
    }

    /// @brief This registers the "standard library" of built-ins.
    /// @details
    /// There are a number of built-ins which are provided by default. These
    /// built-ins are those documented [in the OPA
    /// docs](https://www.openpolicyagent.org/docs/policy-reference/builtins/).
    /// @return A reference to this instance.
    BuiltInsDef& register_standard_builtins();

    /// @brief Creates the standard builtin set.
    /// @return A shared pointer to the created BuiltInsDef.
    static std::shared_ptr<BuiltInsDef> create();

    /// @brief Gets the beginning iterator for the built-ins map.
    /// @return The beginning iterator for the built-ins map.
    std::map<Location, BuiltIn>::const_iterator begin() const;

    /// @brief Gets the ending iterator for the built-ins map.
    /// @return The ending iterator for the built-ins map.
    std::map<Location, BuiltIn>::const_iterator end() const;

  private:
    std::map<Location, BuiltIn> m_builtins;
    bool m_strict_errors;
  };

  /// @cond
  using BuiltIns = std::shared_ptr<BuiltInsDef>;

  const std::string UnknownError = "unknown_error";
  const std::string EvalTypeError = "eval_type_error";
  const std::string EvalBuiltInError = "eval_builtin_error";
  const std::string RegoTypeError = "rego_type_error";
  const std::string RegoParseError = "rego_parse_error";
  const std::string RegoCompileError = "rego_compile_error";
  const std::string EvalConflictError = "eval_conflict_error";
  const std::string WellFormedError = "wellformed_error";
  const std::string RuntimeError = "runtime_error";
  const std::string RecursionError = "rego_recursion_error";
  const std::string DefaultVersion = "v1";
  /// @endcond

  /// @brief Generates an error node.
  /// @param r The range of nodes over which the error occurred.
  /// @param msg The error message.
  /// @param code The error code.
  /// @return The generated error node.
  Node err(
    NodeRange& r,
    const std::string& msg,
    const std::string& code = UnknownError);

  /// @brief Generates an error node.
  /// @param node The node for which the error occurred.
  /// @param msg The error message.
  /// @param code The error code.
  /// @return The generated error node.
  Node err(
    Node node, const std::string& msg, const std::string& code = UnknownError);

  /// @brief Returns a node representing the version of the library.
  /// @details
  ///
  /// The resulting node will be an object containing:
  /// - The Git commit hash ("commit")
  /// - The library version ("regocpp_version")
  /// - The supported OPA Rego version ("version")
  /// - The environment variables ("env")
  Node version();

  /// @brief Converts a node to a unique key representation that can be used for
  /// comparison.
  /// @param node The node to convert.
  /// @param set_as_array Whether to represent sets as arrays.
  /// @param sort_arrays Whether to sort array elements.
  /// @param list_delim The delimiter to use when joining array elements.
  /// @return The key representation of the node.
  std::string to_key(
    const trieste::Node& node,
    bool set_as_array = false,
    bool sort_arrays = false,
    const char* list_delim = ",");

  /// @brief The logging level.
  enum class LogLevel : regoEnum
  {
    None = REGO_LOG_LEVEL_NONE,
    Error = REGO_LOG_LEVEL_ERROR,
    Output = REGO_LOG_LEVEL_OUTPUT,
    Warn = REGO_LOG_LEVEL_WARN,
    Info = REGO_LOG_LEVEL_INFO,
    Debug = REGO_LOG_LEVEL_DEBUG,
    Trace = REGO_LOG_LEVEL_TRACE
  };

  /// Namespace containing types and functions related to Rego
  /// bundles.
  namespace bundle
  {
    /// @brief The type of an operand
    enum class OperandType
    {
      None,
      Local = 1,
      String = 2,
      True = 3,
      False = 4,
      Index,
      Value
    };

    /// @brief Represents an operand in the IR.
    /// @details
    /// In this implementation of the IR the operand is overloaded slightly.
    /// It is either (as in the specification) a local index, a string
    /// index, or a boolean value (true or false). However, to simplify
    /// the implementation, it can also be a raw local index
    /// (OperandType::Index) or a raw integer value (OperandType::Value). These
    /// two additional types allow for more flexibility in the IR
    /// representation. A type of None indicates that the operand is not used.
    struct Operand
    {
      /// @brief The type of the operand.
      OperandType type;
      union
      {
        /// @brief An integer value (if type is Value).
        std::int64_t value;
        /// @brief A local index (if type is Local, String, or Index).
        size_t index;
      };

      /// @brief Constructs an Operand with type None.
      Operand();

      /// @brief Constructs an Operand from the specified node.
      /// @note Will have a type of Local, String, True, or False.
      /// @param n The node to construct the operand from.
      /// @return The constructed operand.
      static Operand from_op(const Node& n);

      /// @brief Constructs an Operand from the specified node. Will have a type
      /// of Index.
      /// @note The node must be an integer scalar.
      /// @param n The node to construct the operand from.
      /// @return The constructed operand.
      static Operand from_index(const Node& n);

      /// @brief Constructs an Operand from the specified node. Will have a type
      /// of Value.
      /// @note The node must be an integer scalar.
      /// @param n The node to construct the operand from.
      /// @return The constructed operand.
      static Operand from_value(const Node& n);

      /// @brief Writes the operand to a stream (used for debugging purposes).
      /// @param stream The stream to write to.
      /// @param op The operand to write.
      /// @return The stream.
      friend std::ostream& operator<<(std::ostream& stream, const Operand& op);
    };

    /// @brief The type of an IR statement.
    /// @details
    /// See https://www.openpolicyagent.org/docs/ir for more information on
    /// these statements.
    enum class StatementType
    {
      Nop = 0,
      ArrayAppend = 1,
      AssignInt = 2,
      AssignVarOnce = 3,
      AssignVar = 4,
      Block = 5,
      Break = 6,
      CallDynamic = 7,
      Call = 8,
      Dot = 9,
      Equal = 10,
      IsArray = 11,
      IsDefined = 12,
      IsObject = 13,
      IsSet = 14,
      IsUndefined = 15,
      Len = 16,
      MakeArray = 17,
      MakeNull = 18,
      MakeNumberInt = 19,
      MakeNumberRef = 20,
      MakeObject = 21,
      MakeSet = 22,
      NotEqual = 23,
      Not = 24,
      ObjectInsert = 25,
      ObjectInsertOnce = 26,
      ObjectMerge = 27,
      ResetLocal = 28,
      ResultSetAdd = 29,
      ReturnLocal = 30,
      Scan = 31,
      SetAdd = 32,
      With = 33
    };

    struct Statement;

    /// @brief A block of statements
    typedef std::vector<Statement> Block;

    /// @brief Additional information for Call statements
    struct CallExt
    {
      /// @brief The function being called
      Location func;
      /// @brief The arguments to the function
      std::vector<Operand> ops;
    };

    /// @brief Additional information for CallDynamic statements
    struct CallDynamicExt
    {
      /// @brief The path to the function being called
      std::vector<Operand> path;
      /// @brief The arguments to the function
      std::vector<Operand> ops;
    };

    /// @brief Additional information for With statements
    struct WithExt
    {
      /// @brief The path to the value being replaced
      std::vector<size_t> path;
      /// @brief The block to execute with the replacement
      Block block;
    };

    /// @brief Additional information for Call, CallDynamic, With, Block, Not,
    /// and Scan statements
    struct StatementExt
    {
      /// @brief The contents of the extension
      std::variant<CallExt, CallDynamicExt, WithExt, std::vector<Block>, Block>
        contents;

      /// @brief Returns this extension as a CallExt
      /// @return The CallExt contents
      const CallExt& call() const;
      /// @brief Returns this extension as a CallDynamicExt
      /// @return The CallDynamicExt contents
      const CallDynamicExt& call_dynamic() const;
      /// @brief Returns this extension as a WithExt
      /// @return The WithExt contents
      const WithExt& with() const;
      /// @brief Returns this extension as a vector of Blocks
      /// @return The vector of Blocks contents
      const std::vector<Block>& blocks() const;
      /// @brief Returns this extension as a Block
      /// @return The Block contents
      const Block& block() const;

      /// @brief Constructs a StatementExt from a CallExt
      StatementExt(CallExt&& ext);
      /// @brief Constructs a StatementExt from a CallDynamicExt
      StatementExt(CallDynamicExt&& ext);
      /// @brief Constructs a StatementExt from a WithExt
      StatementExt(WithExt&& ext);
      /// @brief Constructs a StatementExt from a vector of Blocks
      StatementExt(std::vector<Block>&& blocks);
      /// @brief Constructs a StatementExt from a Block
      StatementExt(Block&& block);
    };

    /// @brief Represents a single IR statement.
    /// @details
    /// See https://www.openpolicyagent.org/docs/ir for more information on
    /// all statement types. Most statements can be represented using this
    /// simple struct, however there are a few statements (Block, Call,
    /// CallDynamic, Scan, and With) which require additional information.
    /// For these statements, the `ext` member will contain a pointer to
    /// a struct containing the additional information.
    struct Statement
    {
      /// @brief The type of statement
      StatementType type;
      /// @brief The location of the statement in the source document
      Location location;

      /// @brief The first operand of the statement
      Operand op0;

      /// @brief The second operand of the statement
      Operand op1;

      /// @brief The target of the statement
      std::int32_t target;

      /// @brief The (optional) extended information for the statement
      std::shared_ptr<const StatementExt> ext;

      /// @brief Default constructor
      Statement();

      /// @brief Writes the statement to a stream (used for debugging purposes)
      /// @param stream The stream to write to
      /// @param stmt The statement to write
      /// @return The stream
      friend std::ostream& operator<<(
        std::ostream& stream, const Statement& stmt);
    };

    /// @brief Represents a plan in the IR.
    struct Plan
    {
      /// @brief The name of the plan
      Location name;
      /// @brief The blocks which make up the plan
      std::vector<Block> blocks;
    };

    /// @brief Represents a function in the IR.
    struct Function
    {
      /// @brief The name of the function
      Location name;
      /// @brief The arity of the function
      size_t arity;
      /// @brief The path of the function (the name broken into segments)
      std::vector<Location> path;
      /// @brief The local indices of the function parameters
      std::vector<size_t> parameters;
      /// @brief The index of the result local
      size_t result;
      /// @brief The blocks which make up the function
      std::vector<Block> blocks;
      /// @brief Whether the function result can be cached
      bool cacheable;
    };
  }

  struct BundleDef;

  /// @brief A pointer to a BundleDef
  typedef std::shared_ptr<BundleDef> Bundle;

  /// @brief Represents a compiled Rego bundle.
  struct BundleDef
  {
    /// @brief The merged base data document
    Node data;
    /// @brief The built-in functions required by the bundle
    std::map<Location, Node> builtin_functions;
    /// @brief Map from plan names to their indices
    std::map<Location, size_t> name_to_func;
    /// @brief Map from function names to their indices
    std::map<Location, size_t> name_to_plan;
    /// @brief The plans in the bundle
    std::vector<bundle::Plan> plans;
    /// @brief The functions in the bundle
    std::vector<bundle::Function> functions;
    /// @brief The string table for the bundle
    std::vector<Location> strings;
    /// @brief The module source files which were compiled into the bundle
    std::vector<Source> files;
    /// @brief The number of local variables required by the bundle
    size_t local_count;
    /// @brief The index of the query plan, if one was included
    std::optional<size_t> query_plan;

    /// @brief The query, if one was included
    Source query;

    /// @brief Finds a plan by name.
    /// @param name The name of the plan to find.
    /// @return The index of the plan if found, otherwise std::nullopt.
    std::optional<size_t> find_plan(const Location& name) const;

    /// @brief Finds a function by name.
    /// @param name The name of the function to find.
    /// @return The index of the function if found, otherwise std::nullopt.
    std::optional<size_t> find_function(const Location& name) const;

    /// @brief Determines whether the provided name refers to a function.
    /// @param name The name to check.
    /// @return True if the name refers to a function, otherwise false.
    bool is_function(const Location& name) const;

    /// @brief Saves the bundle to a stream.
    /// @details
    /// The bundle is saved in Rego Bundle Binary format. To learn
    /// more about this format, see [the specification](../../binary.md).
    /// @param stream The stream to save to.
    void save(std::ostream& stream) const;

    /// @brief Saves the bundle to a file.
    /// @details
    /// The bundle is saved in Rego Bundle Binary format. To learn
    /// more about this format, see [the specification](../../binary.md).
    /// @param path The path to the file to save to.
    void save(const std::filesystem::path& path) const;

    /// @brief Constructs a bundle from an AST node.
    /// @details
    /// The node provided must adhere to the `wf_bundle` well-formedness
    /// definition. Typically, this node is produced by the Interpreter
    /// method `build`.
    /// @param bundle The AST node representing the bundle.
    /// @return The bundle.
    static Bundle from_node(Node bundle);

    /// @brief Loads a bundle from a stream.
    /// @details
    /// The bundle is expected to be in Rego Bundle Binary format. To learn
    /// more about this format, see [the specification](../../binary.md).
    /// @param stream The stream to load from.
    /// @return The bundle, or null if the bundle could not be loaded.
    static Bundle load(std::istream& stream);

    /// @brief Loads a bundle from a file.
    /// @details
    /// The bundle is expected to be in Rego Bundle Binary format. To learn
    /// more about this format, see [the specification](../../binary.md).
    /// @param path The path to the file to load from.
    /// @return The bundle, or null if the bundle could not be loaded.
    static Bundle load(const std::filesystem::path& path);
  };

  /// @brief This class implements a virtual machine that can execute compiled
  /// Rego bundles.
  /// @details
  /// For more information on the IR used by the virtual machine, see
  /// https://www.openpolicyagent.org/docs/ir.
  class VirtualMachine
  {
  public:
    VirtualMachine();

    /// @brief Executes the entrypoint plan in the bundle with the provided
    /// input.
    /// @details
    /// The entrypoint must have been specified when the bundle was built
    /// otherwise an error node will be returned.
    /// @param entrypoint The name of the entrypoint plan to execute.
    /// @param input The input to the plan.
    /// @return The result of executing the plan.
    Node run_entrypoint(const Location& entrypoint, Node input) const;

    /// @brief Executes the query plan in the bundle with the provided input.
    /// @details
    /// The bundle must have been built with a query plan, otherwise
    /// an error node will be returned.
    /// @param input The input to the query.
    /// @return The result of executing the query.
    Node run_query(Node input) const;

    /// @brief Sets the built-in functions to use during execution.
    /// @param builtins The built-in functions to use.
    /// @return A reference to this virtual machine.
    VirtualMachine& builtins(BuiltIns builtins);

    /// @brief Gets the built-in functions used during execution.
    ///@return The built-in functions used.
    BuiltIns builtins() const;

    /// @brief Sets the bundle to use during execution.
    /// @param bundle The bundle to use.
    /// @return A reference to this virtual machine.
    VirtualMachine& bundle(Bundle bundle);

    /// @brief Gets the bundle used during execution.
    Bundle bundle() const;

  private:
    typedef std::vector<Node> Frame;

    enum class Code
    {
      Break,
      Continue,
      Undefined,
      MultipleOutputs,
      Return,
      Error
    };

    class State
    {
    public:
      State(Node input, Node data, size_t num_locals);
      Node read_local(size_t index) const;
      void write_local(size_t index, Node value);
      bool is_defined(size_t key) const;
      void reset_local(size_t key);
      void add_result(Node node);
      const Nodes& result_set() const;
      bool is_in_call_stack(const Location& func_name) const;
      std::string_view root_function_name() const;
      void push_function(const Location& func_name, size_t num_args);
      void pop_function(const Location& func_name);
      const Nodes& errors() const;
      void add_error(Node error);
      void add_error_multiple_output(Node inst);
      void add_error_object_insert(Node inst);
      void put_function_result(const Location& func_name, Node result);
      Node get_function_result(const Location& func_name) const;
      bool in_with() const;
      void push_with();
      void pop_with();
      bool in_break() const;
      void push_break(size_t levels);
      void pop_break();

    private:
      Frame m_frame;
      Nodes m_errors;
      BuiltIns m_builtins;
      std::vector<Location> m_call_stack;
      std::vector<size_t> m_num_args;
      std::map<Location, Node> m_function_cache;
      Nodes m_result_set;
      size_t m_with_count;
      size_t m_break_count;
    };

    void run_plan(const bundle::Plan& plan, State& state) const;
    Code run_block(State& state, const bundle::Block& block) const;
    Code run_stmt(
      State& state, size_t index, const bundle::Statement& stmt) const;
    Code run_scan(State& state, const bundle::Statement& stmt) const;
    Code run_with(State& state, const bundle::Statement& stmt) const;
    Code run_call(
      State& state,
      const Location& func,
      const std::vector<bundle::Operand>& args,
      size_t target) const;
    Node dot(const Node& source, const Node& key) const;
    Node merge_objects(const Node& a, const Node& b) const;
    Node merge_sets(const Node& a, const Node& b) const;
    bool insert_into_object(
      const Node& a, const Node& key, const Node& value, bool once) const;
    Node to_term(const Node& value) const;
    Node unpack_operand(
      const State& state, const bundle::Operand& operand) const;
    Node write_and_swap(
      State& state,
      size_t key,
      const std::vector<size_t>& path,
      Node value) const;

    Bundle m_bundle;
    BuiltIns m_builtins;
    RE2 m_int_regex;
  };

  /// @brief This class forms the main interface to the Rego library.
  /// @details
  /// You can use it to assemble and then execute queries, for example:
  /// ```cpp
  /// Interpreter rego;
  /// rego.add_module_file("objects.rego");
  /// rego.add_data_json_file("data0.json");
  /// rego.add_data_json_file("data1.json");
  /// rego.set_input_json_file("input0.json");
  /// std::cout << rego.query("[data.one, input.b, data.objects.sites[1]]") <<
  /// std::endl;
  /// ```
  class Interpreter
  {
  public:
    /// @brief Constructor.
    Interpreter();

    /// @brief Adds a module (i.e. virtual document) file to the interpreter.
    /// @details
    /// This is the same as calling Interpreter::add_module with the contents of
    /// the file.
    /// @param path The path to the module file.
    /// @returns either an error node or a nullptr if the module is valid.
    Node add_module_file(const std::filesystem::path& path);

    /// @brief Adds a module (i.e. virtual document) to the interpreter.
    /// @details
    /// The module will be parsed and added to the interpreter's module
    /// sequence.
    /// @param name The name of the module.
    /// @param contents The contents of the module.
    /// @returns either an error node or a nullptr if the module is valid.
    Node add_module(const std::string& name, const std::string& contents);

    /// @brief Adds a base document to the interpreter.
    /// @details
    /// This is the same as calling Interpreter::add_data_json with the contents
    /// of the file.
    /// @param path The path to the data JSON file.
    /// @returns either an error node or a nullptr if the JSON is valid.
    Node add_data_json_file(const std::filesystem::path& path);

    /// @brief Adds a base document to the interpreter.
    /// @details
    /// The document must contain a single JSON-encoded object, and will be
    /// parsed and added to the interpreter's data sequence.
    /// @param json The contents of the document.
    /// @returns either an error node or a nullptr if the JSON is valid.
    Node add_data_json(const std::string& json);

    /// @brief Adds a base document to the interpreter.
    /// @details
    /// Adds a JSON AST node to the interpreter's data sequence.
    /// @param node The contents of the document.
    /// @returns either an error node or a nullptr if the node is valid.
    Node add_data(const Node& node);

    /// @brief Sets the input document to the interpreter.
    /// @details
    /// This is the same as calling Interpreter::set_input_json with the
    /// contents of the file.
    /// @param path The path to the input file.
    /// @returns either an error node or a nullptr if the input document is
    /// valid.
    Node set_input_json_file(const std::filesystem::path& path);

    /// @brief Sets the input document to the interpreter.
    /// @details
    /// The document must contain a single JSON-encoded object, and will be
    /// parsed and set as the interpreter's input.
    /// @param json The contents of the document.
    /// @returns either an error node or a nullptr if the input document is
    /// valid.
    Node set_input_json(const std::string& json);

    /// @brief Sets the input term of the interpreter.
    /// @details
    /// The string must contain a single valid Rego data term.
    /// @param term The contents of the term.
    /// @returns either an error node or a nullptr if the input term is valid.
    Node set_input_term(const std::string& term);

    /// @brief Sets the input document to the interpreter.
    /// @details
    /// Sets an AST node directly as the interpreter's input. Use with
    /// caution. The node can either be a JSON AST (which will be converted) or
    /// a Rego AST.
    /// @param node The contents of the document.
    Node set_input(const Node& node);

    /// @brief Sets the query expression of the interpreter.
    /// @details
    /// This query will be included when building a bundle.
    /// @param query The query expression.
    /// @returns either an error node or a nullptr if the query expression is
    /// valid.
    Node set_query(const std::string& query);

    /// @brief Sets the entrypoints to include when building a bundle.
    /// @param entrypoints The entrypoints.
    /// @returns A reference to this interpreter.
    Interpreter& entrypoints(
      const std::initializer_list<std::string>& entrypoints);

    /// @brief Sets the entrypoints to include when building a bundle.
    /// @param entrypoints The entrypoints.
    /// @returns A reference to this interpreter.
    Interpreter& entrypoints(const std::vector<std::string>& entrypoints);

    /// @brief Gets the entrypoints to include when building a bundle.
    /// @returns The entrypoints.
    const std::vector<std::string>& entrypoints() const;

    /// @brief Gets the entrypoints to include when building a bundle.
    /// @returns The entrypoints.
    std::vector<std::string>& entrypoints();

    /// @brief Executes the documents against the interpreter.
    /// @details
    /// This method calls Interpreter::query_node and then converts it into a
    /// human-readable string using Interpeter::output_to_string.
    /// @return The result of the query.
    std::string query();

    /// @brief Executes a query against the interpreter.
    /// @details
    /// This method calls Interpreter::query_node and then converts it into a
    /// human-readable string using Interpreter::output_to_string.
    /// @param query_expr The query expression.
    /// @return The result of the query.
    std::string query(const std::string& query_expr);

    /// @brief Executes a query against the interpreter.
    /// @details
    /// The query expression must be a valid Rego query expression. The result
    /// will be an AST node representing the result, which will either be a
    /// list of bindings and terms, or an error sequence.
    /// @param query_expr The query expression.
    /// @return The result of the query.
    Node query_node(const std::string& query_expr);

    /// @brief Executes a query against the interpreter.
    /// @details
    /// The query executed will be the one provided to Interpreter::set_query.
    /// The result will be an AST node representing the result, which will
    /// either be a list of bindings and terms, or an error sequence.
    /// @return The result of the query.
    Node query_node();

    // clang-format off

    /// @brief Builds a bundle from the current state of the interpreter.
    /// @details
    /// The bundle will include all base and virtual documents, and will include
    /// execution plans for a query (if set) and zero or more entrypoints (if
    /// set). If no query or entrypoints are set, or if there is an error during
    /// bundle creation, an error node will be returned. The resulting bundle
    /// node can be used as an input to Bundle::from_node.
    /// ```cpp
    /// rego.set_query("[data.one, input.b, data.objects.sites[1]] = x");
    /// rego.entrypoints({"objects/sites", "objects/rect"});
    /// rego::Node bundle_node = rego.build();
    /// rego::Bundle bundle = rego::BundleDef::from_node(bundle_node);
    /// // we can assemble the input term manually
    /// rego::Node input = rego::object({
    ///   rego::object_item(rego::scalar("a"), rego::scalar(rego::BigInt(10L))),
    ///   rego::object_item(rego::scalar("b"), rego::scalar("20")),
    ///   rego::object_item(rego::scalar("c"), rego::scalar(30.0)),
    ///   rego::object_item(rego::scalar("d"), rego::scalar(true)),
    /// });
    /// rego.set_input(input);
    /// std::cout << rego.output_to_string(rego.query_bundle(bundle)) << std::endl;
    /// // we can also query bundle entrypoints directly
    /// std::cout << rego.output_to_string(rego.query_bundle(bundle, "objects/sites"))
    ///           << std::endl;
    /// ```
    /// @return The bundle, or an error node.
    Node build();

    // clang-format on

    /// @brief Saves a bundle to a directory in JSON format.
    /// @details
    /// The bundle will be saved in a format compatible with OPA. The directory
    /// will be created if it does not exist. There will be a `plan.json` file,
    /// a `data.json` file, and zero or more `.rego` files (from the modules
    /// used during compilation). If there is an error during saving, an error
    /// node will be returned.
    /// @param dir The directory to save the bundle to.
    /// @param bundle The bundle to save (as produced by Interpreter::build).
    /// @return Either an error node, or a null node if the bundle was saved
    ///         successfully.
    Node save_bundle(const std::filesystem::path& dir, const Node& bundle);

    /// @brief Loads a bundle in JSON format from a direction.
    /// @details
    /// The directory is expected to contain a `plan.json` file, a `data.json`
    /// file, and zero or more `.rego` files. If there is an error when loading,
    /// an error node will be returned.
    /// @param dir The direction to use when loading the bundle
    /// @return Either an bundle node, or an error node if there was a problem
    /// during loading.
    Node load_bundle(const std::filesystem::path& dir);

    /// @brief Performs a query against a bundle.
    /// @details
    /// The interpreter will load this bundle into its virtual machine and then
    /// execute the stored query plan. If no query was compiled into the bundle,
    /// this will result in an error. Otherwise, the result will be an AST node
    /// representing the result, which will either be a list of bindings and
    /// terms, or an error sequence.
    /// @param bundle The bundle to query
    /// @return The result of the query
    Node query_bundle(const Bundle& bundle);

    /// @brief Performs a query against a bundle.
    /// @details
    /// The interpreter will load this bundle into its virtual machine and then
    /// execute the stored entrypoint plan. If this entrypoint was not compiled
    /// into the bundle, this will result in an error. Otherwise, the result
    /// will be an AST node representing the result, which will either be a list
    /// of bindings and terms, or an error sequence.
    /// @param bundle The bundle to query
    /// @param endpoint The entrypoint to execute
    /// @return The result of the query
    Node query_bundle(const Bundle& bundle, const std::string& endpoint);

    /// @brief The path to the debug directory.
    /// @details
    /// If set, then (when in debug mode) the interpreter will output
    /// intermediary ASTs after each compiler pass to the debug directory. If
    /// the directory does not exist, it will be created.
    Interpreter& debug_path(const std::filesystem::path& prefix);

    /// @brief Gets the debug path.
    /// @return The debug path.
    const std::filesystem::path& debug_path() const;

    /// @brief Sets whether debug mode is enabled.
    /// @details
    /// If true, then the interpreter will output intermediary ASTs after each
    /// compiler pass to the debug directory set via Interpreter::debug_path.
    /// @param enabled whether debug is enabled
    /// @return a reference to this Interpreter
    Interpreter& debug_enabled(bool enabled);

    /// @brief Checks if debug mode is enabled.
    /// @return True if debug mode is enabled, false otherwise.
    bool debug_enabled() const;

    /// @brief Sets whether well-formedness checks are enabled.
    /// @details
    /// If true, then the interpreter will perform well-formedness checks after
    /// each compiler pass using the well-formedness definitions.
    /// @param enabled Whether well-formedness checks are enabled
    /// @return a reference to this Interpreter
    Interpreter& wf_check_enabled(bool enabled);

    /// @brief Checks if well-formedness checks are enabled.
    /// @return True if well-formedness checks are enabled, false otherwise.
    bool wf_check_enabled() const;

    /// @brief The built-ins used by the interpreter.
    /// @details
    /// This object can be used to register custom built-ins created using
    /// BuiltInDef::create.
    /// @return the builtins used by the interpreter
    BuiltIns builtins() const;

    /// @brief Converts an output node into a human-readable string.
    /// @param output An output node returned from Interpreter::query_node or
    /// Interpreter::query_bundle.
    /// @return A human-readable string representing the output
    std::string output_to_string(const Node& output) const;

    /// @brief Returns the current log level of this interpreter.
    /// @return The log level
    LogLevel log_level() const;

    /// @brief Sets the log level of the interpreter.
    /// @param level the log level
    /// @return A reference to the interpreter.
    Interpreter& log_level(LogLevel level);

    /// @brief Sets the logging level for the interpreter from a string.
    /// @param level The logging level. One of "None", "Error", "Output",
    /// "Warn",
    ///              "Info", "Debug", or "Trace".
    /// @return A reference to the interpreter.
    Interpreter& log_level(const std::string& level);

    /// @brief Returns a string representing the most recent error message.
    /// @note This is intended for use by the C API.
    /// @return An error string
    const std::string& c_error() const;

    /// @brief Sets the most recent error message.
    /// @note This is intended for use by the C API.
    /// @param error the error message
    /// @return A reference to the interpreter.
    Interpreter& c_error(const std::string& error);

  private:
    Reader& reader();
    Reader& json();
    Rewriter& data_from_json();
    Rewriter& input_from_json();
    Rewriter& to_input();
    Rewriter& bundle();
    Rewriter& write_bundle();
    Rewriter& read_bundle();

    void merge(const Node& ast);

    Node m_dataseq;
    Node m_moduleseq;
    Node m_input;
    Node m_query;
    std::vector<std::string> m_entrypoints;
    std::filesystem::path m_debug_path;
    bool m_debug_enabled;
    bool m_wf_check_enabled;
    LogLevel m_log_level;

    BuiltIns m_builtins;
    std::unique_ptr<Reader> m_reader;
    std::unique_ptr<Reader> m_json;
    std::unique_ptr<Rewriter> m_data_from_json;
    std::unique_ptr<Rewriter> m_input_from_json;
    std::unique_ptr<Rewriter> m_to_input;
    std::unique_ptr<Rewriter> m_bundle;
    std::unique_ptr<Rewriter> m_write_bundle;
    std::unique_ptr<Rewriter> m_read_bundle;
    VirtualMachine m_vm;
    std::size_t m_data_count;
    std::map<std::string, Node> m_cache;

    std::string m_c_error;
  };

  /// @brief Converts a string to a log level (case insensitive).
  /// @param value One of `Error`, `Warn`, `Output`, `Info`, `Debug`, `Trace`.
  /// @return the corresponding log level, or LogLevel::Unsupported.
  LogLevel log_level_from_string(const std::string& value);

  /// Bundle
  /// @cond
  inline const auto RegoBundle = TokenDef("rego-bundle", flag::symtab);
  inline const auto Data = TokenDef("rego-data");
  inline const auto ModuleFile = TokenDef("rego-modulefile");
  inline const auto Static = TokenDef("rego-static");
  inline const auto BuiltInFunction = TokenDef("rego-builtinfunction");
  inline const auto Plan = TokenDef("rego-plan", flag::lookup | flag::symtab);
  inline const auto Block = TokenDef("rego-block");
  inline const auto Function =
    TokenDef("rego-function", flag::lookup | flag::symtab);
  inline const auto LocalIndex = TokenDef("rego-localindex", flag::print);
  inline const auto Operand = TokenDef("rego-operand");
  inline const auto Int32 = TokenDef("rego-int32", flag::print);
  inline const auto Int64 = TokenDef("rego-int64", flag::print);
  inline const auto UInt32 = TokenDef("rego-uint32", flag::print);
  inline const auto StringIndex = TokenDef("rego-stringindex", flag::print);
  inline const auto Boolean = TokenDef("rego-boolean", flag::print);
  inline const auto IRString = TokenDef("rego-irstring", flag::print);
  inline const auto IRQuery = TokenDef("rego-irquery");
  inline const auto IRPath = TokenDef("rego-irpath");

  // sequences
  inline const auto BlockSeq = TokenDef("rego-blockseq");
  inline const auto ParameterSeq = TokenDef("rego-parameterseq");
  inline const auto PlanSeq = TokenDef("rego-planseq");
  inline const auto FunctionSeq = TokenDef("rego-functionseq");
  inline const auto BuiltInFunctionSeq = TokenDef("rego-builtinfunctionseq");
  inline const auto StringSeq = TokenDef("rego-stringseq");
  inline const auto PathSeq = TokenDef("rego-pathseq");
  inline const auto OperandSeq = TokenDef("rego-operandseq");
  inline const auto Int32Seq = TokenDef("rego-int32seq");
  inline const auto EntryPointSeq = TokenDef("rego-entrypointseq");
  inline const auto ModuleFileSeq = TokenDef("rego-modulefileseq");

  // Statements
  inline const auto ArrayAppendStmt = TokenDef("rego-arrayappendstmt");
  inline const auto AssignIntStmt = TokenDef("rego-assignintstmt");
  inline const auto AssignVarOnceStmt = TokenDef("rego-assignvaroncestmt");
  inline const auto AssignVarStmt = TokenDef("rego-assignvarstmt");
  inline const auto BlockStmt = TokenDef("rego-blockstmt");
  inline const auto BreakStmt = TokenDef("rego-breakstmt");
  inline const auto CallDynamicStmt = TokenDef("rego-calldynamicstmt");
  inline const auto CallStmt = TokenDef("rego-callstmt");
  inline const auto DotStmt = TokenDef("rego-dotstmt");
  inline const auto EqualStmt = TokenDef("rego-equalstmt");
  inline const auto IsArrayStmt = TokenDef("rego-isarraystmt");
  inline const auto IsDefinedStmt = TokenDef("rego-isdefinedstmt");
  inline const auto IsObjectStmt = TokenDef("rego-isobjectstmt");
  inline const auto IsSetStmt = TokenDef("rego-issetstmt");
  inline const auto IsUndefinedStmt = TokenDef("rego-isundefinedstmt");
  inline const auto LenStmt = TokenDef("rego-lenstmt");
  inline const auto MakeArrayStmt = TokenDef("rego-makearraystmt");
  inline const auto MakeNullStmt = TokenDef("rego-makenullstmt");
  inline const auto MakeNumberIntStmt = TokenDef("rego-makenumberintstmt");
  inline const auto MakeNumberRefStmt = TokenDef("rego-makenumberrefstmt");
  inline const auto MakeObjectStmt = TokenDef("rego-makeobjectstmt");
  inline const auto MakeSetStmt = TokenDef("rego-makesetstmt");
  inline const auto NotEqualStmt = TokenDef("rego-notequalstmt");
  inline const auto NotStmt = TokenDef("rego-notstmt");
  inline const auto ObjectInsertOnceStmt =
    TokenDef("rego-objectinsertoncestmt");
  inline const auto ObjectInsertStmt = TokenDef("rego-objectinsertstmt");
  inline const auto ObjectMergeStmt = TokenDef("rego-objectmergestmt");
  inline const auto ResetLocalStmt = TokenDef("rego-resetlocalstmt");
  inline const auto ResultSetAddStmt = TokenDef("rego-resultsetaddstmt");
  inline const auto ReturnLocalStmt = TokenDef("rego-returnlocalstmt");
  inline const auto ScanStmt = TokenDef("rego-scanstmt");
  inline const auto SetAddStmt = TokenDef("rego-setaddstmt");
  inline const auto WithStmt = TokenDef("rego-withstmt");

  // other tokens
  inline const auto Blocks = TokenDef("rego-blocks");
  inline const auto Func = TokenDef("rego-func");
  inline const auto Capacity = TokenDef("rego-capacity");
  inline const auto Stmt = TokenDef("rego-stmt");

  inline const auto wf_ir_statements = ArrayAppendStmt | AssignIntStmt |
    AssignVarOnceStmt | AssignVarStmt | BlockStmt | BreakStmt |
    CallDynamicStmt | CallStmt | DotStmt | EqualStmt | IsArrayStmt |
    IsDefinedStmt | IsObjectStmt | IsSetStmt | IsUndefinedStmt | LenStmt |
    MakeArrayStmt | MakeNullStmt | MakeNumberIntStmt | MakeNumberRefStmt |
    MakeObjectStmt | MakeSetStmt | NotEqualStmt | NotStmt |
    ObjectInsertOnceStmt | ObjectInsertStmt | ObjectMergeStmt | ResetLocalStmt |
    ResultSetAddStmt | ReturnLocalStmt | ScanStmt | SetAddStmt | WithStmt;

  // clang-format off
  inline const auto wf_bundle =
    builtins::wf_decl
    | wf_result
    | (Top <<= RegoBundle)
    | (RegoBundle <<= Data * Policy * ModuleFileSeq)
    | (ModuleFileSeq <<= ModuleFile++)
    | (ModuleFile <<= (Name >>= IRString) * (Contents >>= IRString))
    | (Data <<= rego::Object)
    | (Policy <<= Static * PlanSeq * IRQuery * FunctionSeq)
    | (IRQuery <<= (IRString|Undefined))
    | (Static <<= StringSeq * PathSeq * BuiltInFunctionSeq)
    | (StringSeq <<= IRString++)
    | (BuiltInFunctionSeq <<= BuiltInFunction++)
    | (BuiltInFunction <<= (Name >>= IRString) * builtins::Decl)
    | (PathSeq <<= IRString++)
    | (PlanSeq <<= Plan++[1])
    | (Plan <<= (Name >>= IRString) * BlockSeq)
    | (BlockSeq <<= Block++)
    | (Block <<= wf_ir_statements++)
    | (FunctionSeq <<= Function++)
    | (Function <<= (Name >>= IRString) * IRPath * ParameterSeq * (Return >>= LocalIndex) * BlockSeq)[Name]
    | (IRPath <<= IRString++[1])
    | (ParameterSeq <<= LocalIndex++)
    | (ArrayAppendStmt <<= (Array >>= LocalIndex) * (Val >>= Operand))
    | (AssignIntStmt <<= (Val >>= Int64) * (Target >>= LocalIndex))
    | (AssignVarOnceStmt <<= (Src >>= Operand) * (Target >>= LocalIndex))
    | (AssignVarStmt <<= (Src >>= Operand) * (Target >>= LocalIndex))
    | (BlockStmt <<= (Blocks >>= BlockSeq))
    | (BreakStmt <<= (Idx >>= UInt32))
    | (CallDynamicStmt <<= (Func >>= OperandSeq) * (Args >>= OperandSeq) * (Result >>= LocalIndex))
    | (CallStmt <<= (Func >>= IRString) * (Args >>= OperandSeq) * (Result >>= LocalIndex))
    | (DotStmt <<= (Src >>= Operand) * (Key >>= Operand) * (Target >>= LocalIndex))
    | (EqualStmt <<= (Lhs >>= Operand) * (Rhs >>= Operand))
    | (IsArrayStmt <<= (Src >>= Operand))
    | (IsDefinedStmt <<= (Src >>= LocalIndex))
    | (IsObjectStmt <<= (Src >>= Operand))
    | (IsSetStmt <<= (Src >>= Operand))
    | (IsUndefinedStmt <<= (Src >>= LocalIndex))
    | (LenStmt <<= (Src >>= Operand) * (Target >>= LocalIndex))
    | (MakeArrayStmt <<= (Capacity >>= Int32) * (Target >>= LocalIndex))
    | (MakeNullStmt <<= (Target >>= LocalIndex))
    | (MakeNumberIntStmt <<= (Val >>= Int64) * (Target >>= LocalIndex))
    | (MakeNumberRefStmt <<= (Idx >>= Int32) * (Target >>= LocalIndex))
    | (MakeObjectStmt <<= (Target >>= LocalIndex))
    | (MakeSetStmt <<= (Target >>= LocalIndex))
    | (NotEqualStmt <<= (Lhs >>= Operand) * (Rhs >>= Operand))
    | (NotStmt <<= Block)
    | (ObjectInsertOnceStmt <<= (Key >>= Operand) * (Val >>= Operand) * (Object >>= LocalIndex))
    | (ObjectInsertStmt <<= (Key >>= Operand) * (Val >>= Operand) * (Object >>= LocalIndex))
    | (ObjectMergeStmt <<= (Lhs >>= LocalIndex) * (Rhs >>= LocalIndex) * (Target >>= LocalIndex))
    | (ResetLocalStmt <<= (Target >>= LocalIndex))
    | (ResultSetAddStmt <<= (Val >>= LocalIndex))
    | (ReturnLocalStmt <<= (Src >>= LocalIndex))
    | (ScanStmt <<= (Src >>= LocalIndex) * (Key >>= LocalIndex) * (Val >>= LocalIndex) * Block)
    | (SetAddStmt <<= (Val >>= Operand) * (Set >>= LocalIndex))
    | (WithStmt <<= LocalIndex * (Path >>= Int32Seq) * (Val >>= Operand) * Block)
    | (Operand <<= LocalIndex | Boolean | StringIndex)
    | (OperandSeq <<= Operand++)
    | (Int32Seq <<= Int32++)
    ;
  // clang-format on
  /// @endcond

  /// @brief Parses Rego queries and virtual documents.
  Reader file_to_rego();

  /// @brief Rewrites a Query AST to an input term.
  Rewriter rego_to_input();

  /// @brief Rewrites a JSON AST to a Rego data input AST.
  Rewriter json_to_rego(bool as_term = false);

  /// @brief Rewrites a Rego binding term to a JSON AST.
  Rewriter rego_to_json();

  /// @brief Rewrites a Rego binding term to a YAML AST.
  Rewriter rego_to_yaml();

  /// @brief Rewrites an OPA bundle JSON to a Bundle AST.
  Rewriter json_to_bundle();

  /// @brief Rewrites a Bundle AST to a JSON AST in OPA bundle JSON format.
  Rewriter bundle_to_json();

  /// @brief Rewrites a Rego AST to a Bundle AST.
  Rewriter rego_to_bundle(BuiltIns builtins = BuiltInsDef::create());
}