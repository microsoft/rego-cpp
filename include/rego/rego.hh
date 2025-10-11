// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "rego_c.h"
#include "trieste/logging.h"
#include "trieste/token.h"

#include <initializer_list>
#include <trieste/trieste.h>

/**
 *  This namespace provides the C++ API for the library.
 *  It includes all the token types for nodes in the AST, the well-formedness
 *  definitions for each pass, the built-in system and custom types for
 *  handling various kinds of data (e.g. the BigInt class).
 */
namespace rego
{
  using namespace trieste;
  using namespace wf::ops;

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

  namespace builtins
  {
    struct BuiltInDef;
  };

  /** A pointer to a BuiltInDef.*/
  using BuiltIn = std::shared_ptr<builtins::BuiltInDef>;

  /** The function pointer to the behavior of the built-in. */
  using BuiltInBehavior = std::function<Node(const Nodes&)>;

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
    BigInt abs() const;
    BigInt negate() const;

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
   * specified. However, if Float was specified, the result would be an
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
   * Attempts to unwrap an argument to a specified type.
   *
   * @param term The term to unwrap.
   * @param type The acceptable type.
   * @return An unwrap result.
   */
  UnwrapResult unwrap(const Node& term, const Token& type);

  /**
   * Attempts to unwrap an argument to one of the specified types.
   *
   * @param term The term to unwrap.
   * @param types The acceptable types.
   * @return An unwrap result.
   */
  UnwrapResult unwrap(const Node& term, const std::vector<Token>& types);

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
   * Attempts to extract the value of a node as an integer.
   * In the case that the node is a double, it will check if the
   * double is integral, i.e. 5.0 will be considered the integer 5.
   *
   * @param node The node to extract from.
   * @return The integer value of the node
   */
  std::optional<BigInt> try_get_int(const Node& node);

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
   * Tries to get an item within an object using its key.
   *
   * @param node The object to search.
   * @param key The key to search for.
   */
  std::optional<Node> try_get_item(
    const Node& node, const std::string_view& key);

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
  Node object(const std::initializer_list<Node>& object_items);

  /**
   * Converts the value to an array node.
   *
   * @param array_members The members of the array.
   * @return The array node (array (term) (term) ...)
   */
  Node array(const std::initializer_list<Node>& array_members);

  /**
   * Converts the value to a set node.
   *
   * @param set_members The members of the set.
   * @return The set node (set (term) (term) ...)
   */
  Node set(const std::initializer_list<Node>& set_members);

  /** This constant indicates that a built-in can receive any number of
   * arguments. */
  const std::size_t AnyArity = std::numeric_limits<std::size_t>::max();

  namespace builtins
  {
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

    /**
     * Struct which defines a built-in function.
     *
     * You can extend Rego by registering your own built-ins. A built-in is a
     * function which is called by Rego during evaluation. Built-ins are called
     * with a vector of Nodes, and return a Node. The vector of Nodes contains
     * the arguments passed to the built-in. The Node returned by the built-in
     * is the result of the built-in's evaluation.
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
      /** The name used to match against expression calls in the rego program.
       */
      Location name;

      Node decl;
      /**
       * The number of expected arguments.
       *
       * If any number of arguments can be provided, use the constant AnyArity.
       */
      std::size_t arity;

      /** The function which will be called when the built-in is evaluated. */
      BuiltInBehavior behavior;

      /** Whether the builtin is available. */
      bool available;

      /**
       * Constructor.
       */
      BuiltInDef(
        Location name_, Node decl_, BuiltInBehavior behavior_, bool available_);

      virtual ~BuiltInDef() = default;

      /**
       * Called to clear any persistent state or caching.
       */
      virtual void clear();

      /**
       * Creates a new built-in.
       *
       * The `decl` node adheres to the `builtins::wf_decl` well-formedness
       * definition. BuiltIn is a pointer to a BuiltInDef.
       *
       * @param name The name of the built-in.
       * @param decl Metadata and documentation for the built-in.
       * @param behavior The function which will be called when the built-in is
       * evaluated.
       * @return The built-in.
       */
      static BuiltIn create(
        const Location& name, Node decl, BuiltInBehavior behavior);

      /**
       * Creates a placeholder for a built-in which is not available on this
       * platform.
       *
       * The `decl` node adheres to the `builtins::wf_decl` well-formedness
       * definition. BuiltIn is a pointer to a BuiltInDef.
       *
       * @param name The name of the built-in.
       * @param decl Metadata and documentation for the built-in.
       * @return The built-in.
       */
      static BuiltIn placeholder(
        const Location& name, Node decl, const std::string& message);
    };

    /**
     * Manages the set of builtins used by an interpreter to resolve built-in
     * calls.
     */
    class BuiltInsDef
    {
    public:
      /** Constructor. */
      BuiltInsDef() noexcept;

      /**
       * Determines whether the provided name refers to a built-in.
       *
       * @param name The name to check.
       * @return True if the name refers to a built-in, otherwise false.
       */
      bool is_builtin(const Location& name) const;

      Node decl(const Location& name) const;

      /**
       * Determines whether the provided builtin name is deprecated in the
       * provided version.
       *
       * @param version The version to check.
       * @param name The name to check.
       */
      bool is_deprecated(const Location& version, const Location& name) const;

      /**
       * Calls the built-in with the provided name and arguments.
       *
       * @param name The name of the built-in to call.
       * @param args The arguments to pass to the built-in.
       * @param version The Rego version.
       * @return The result of the built-in call.
       */
      Node call(
        const Location& name, const Location& version, const Nodes& args);

      /**
       * Called to clear any persistent state or caching.
       */
      void clear();

      /**
       * Registers a built-in.
       *
       * @param built_in The built-in to register.
       */
      BuiltInsDef& register_builtin(const BuiltIn& built_in);

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
      BuiltInsDef& strict_errors(bool strict_errors);

      /**
       * Registers a set of built-ins.
       *
       * @param built_ins The built-ins to register.
       */
      template <typename T>
      BuiltInsDef& register_builtins(const T& built_ins)
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
       * <a
       * href="https://www.openpolicyagent.org/docs/latest/policy-reference/#built-in-functions">here</a>.
       *
       * rego-cpp supports the following built-ins as standard:
       *
       */
      BuiltInsDef& register_standard_builtins();

      /**
       * Creates the standard builtin set.
       */
      static std::shared_ptr<BuiltInsDef> create();

      std::map<Location, BuiltIn>::const_iterator begin() const;
      std::map<Location, BuiltIn>::const_iterator end() const;

    private:
      std::map<Location, BuiltIn> m_builtins;
      bool m_strict_errors;
    };
  }

  using BuiltIns = std::shared_ptr<builtins::BuiltInsDef>;

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

  /**
   * Generates an error node.
   *
   * @param r The range of nodes over which the error occurred.
   * @param msg The error message.
   * @param code The error code.
   */
  Node err(
    NodeRange& r,
    const std::string& msg,
    const std::string& code = UnknownError);

  /**
   * Generates an error node.
   *
   * @param node The node for which the error occurred.
   * @param msg The error message.
   * @param code The error code.
   */
  Node err(
    Node node, const std::string& msg, const std::string& code = UnknownError);

  /**
   * Returns a node representing the version of the library.
   *
   * The resulting node will be an object containing:
   * - The Git commit hash ("commit")
   * - The library version ("regocpp_version")
   * - The supported OPA Rego version ("version")
   * - The environment variables ("env")
   */
  Node version();

  /** Converts a node to a unique key representation.
   *
   * @param node The node to convert.
   * @param set_as_array Whether to represent sets as arrays.
   */
  std::string to_key(
    const trieste::Node& node,
    bool set_as_array = false,
    bool sort_arrays = false,
    const char* list_delim = ",");

  /**
   * The logging level.
   */
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

  namespace bundle
  {
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

    struct Operand
    {
      OperandType type;
      union
      {
        std::int64_t value;
        size_t index;
      };

      Operand();

      static Operand from_op(const Node& n);
      static Operand from_index(const Node& n);
      static Operand from_value(const Node& n);

      friend std::ostream& operator<<(std::ostream& stream, const Operand& op);
    };

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

    typedef std::vector<Statement> Block;

    struct CallExt
    {
      Location func;
      std::vector<Operand> ops;
    };

    struct CallDynamicExt
    {
      std::vector<Operand> path;
      std::vector<Operand> ops;
    };

    struct WithExt
    {
      std::vector<size_t> path;
      Block block;
    };

    struct StatementExt
    {
      std::variant<CallExt, CallDynamicExt, WithExt, std::vector<Block>, Block>
        contents;

      const CallExt& call() const;
      const CallDynamicExt& call_dynamic() const;
      const WithExt& with() const;
      const std::vector<Block>& blocks() const;
      const Block& block() const;

      StatementExt(CallExt&& ext);
      StatementExt(CallDynamicExt&& ext);
      StatementExt(WithExt&& ext);
      StatementExt(std::vector<Block>&& blocks);
      StatementExt(Block&& block);
    };

    struct Statement
    {
      StatementType type;
      Location location;
      Operand op0;
      Operand op1;
      std::int32_t target;
      std::shared_ptr<const StatementExt> ext;

      Statement();

      friend std::ostream& operator<<(
        std::ostream& stream, const Statement& stmt);
    };

    struct Plan
    {
      Location name;
      std::vector<Block> blocks;
    };

    struct Function
    {
      Location name;
      size_t arity;
      std::vector<Location> path;
      std::vector<size_t> parameters;
      size_t result;
      std::vector<Block> blocks;
      bool cacheable;
    };
  }

  struct BundleDef;

  typedef std::shared_ptr<BundleDef> Bundle;

  struct BundleDef
  {
    Node data;
    std::map<Location, Node> builtin_functions;
    std::map<Location, size_t> name_to_func;
    std::map<Location, size_t> name_to_plan;
    std::vector<bundle::Plan> plans;
    std::vector<bundle::Function> functions;
    std::vector<Location> strings;
    std::vector<Source> files;
    size_t local_count;
    std::optional<size_t> query_plan;
    Source query;

    std::optional<size_t> find_plan(const Location& name) const;
    std::optional<size_t> find_function(const Location& name) const;
    bool is_function(const Location& name) const;
    void save(std::ostream& stream) const;
    void save(const std::filesystem::path& path) const;

    static Bundle from_node(Node bundle);
    static Bundle load(std::istream& stream);
    static Bundle load(const std::filesystem::path& path);
  };

  class VirtualMachine
  {
  public:
    VirtualMachine();
    Node run_entrypoint(const Location& entrypoint, Node input) const;
    Node run_query(Node input) const;

    VirtualMachine& builtins(BuiltIns builtins);
    BuiltIns builtins() const;

    VirtualMachine& bundle(Bundle bundle);
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

  /**
   * This class forms the main interface to the Rego library.
   *
   * You can use it to assemble and then execute queries, for example:
   *
   * ```cpp
   * Interpreter rego;
   * rego.add_module_file("objects.rego");
   * rego.add_data_json_file("data0.json");
   * rego.add_data_json_file("data1.json");
   * rego.set_input_json_file("input0.json");
   * std::cout << rego.query("[data.one, input.b, data.objects.sites[1]]") <<
   * std::endl;
   * ```
   */
  class Interpreter
  {
  public:
    /**
     * Constructor.
     */
    Interpreter();

    /**
     * Adds a module (i.e. virtual document) file to the interpreter.
     *
     * This is the same as calling Interpreter::add_module with the contents of
     * the file.
     *
     * @param path The path to the module file.
     * @returns either an error node or a nullptr if the module is valid.
     */
    Node add_module_file(const std::filesystem::path& path);

    /**
     * Adds a module (i.e. virtual document) to the interpreter.
     *
     * The module will be parsed and added to the interpreter's module sequence.
     *
     * @param name The name of the module.
     * @param contents The contents of the module.
     * @returns either an error node or a nullptr if the module is valid.
     */
    Node add_module(const std::string& name, const std::string& contents);

    /**
     * Adds a base document to the interpreter.
     *
     * This is the same as calling Interpreter::add_data_json with the contents
     * of the file.
     *
     * @param module The module to add.
     * @returns either an error node or a nullptr if the JSON is valid.
     */
    Node add_data_json_file(const std::filesystem::path& path);

    /**
     * Adds a base document to the interpreter.
     *
     * The document must contain a single JSON-encoded object, and will be
     * parsed and added to the interpreter's data sequence.
     *
     * @param json The contents of the document.
     * @returns either an error node or a nullptr if the JSON is valid.
     */
    Node add_data_json(const std::string& json);

    /**
     * Adds a base document to the interpreter.
     *
     * Adds an AST node directly to the interpreter's data sequence. Use with
     * caution.
     *
     * @param node The contents of the document.
     */
    Node add_data(const Node& node);

    /**
     * Sets the input document to the interpreter.
     *
     * This is the same as calling Interpreter::set_input_json with the contents
     * of the file.
     *
     * @param path The path to the input file.
     * @returns either an error node or a nullptr if the input document is
     * valid.
     */
    Node set_input_json_file(const std::filesystem::path& path);

    /**
     * Sets the input document to the interpreter.
     *
     * The document must contain a single JSON-encoded object, and will be
     * parsed and set as the interpreter's input.
     *
     * @param json The contents of the document.
     * @returns either an error node or a nullptr if the input document is
     * valid.
     */
    Node set_input_json(const std::string& json);

    /**
     * Sets the input term of the interpreter.
     *
     * The string must contain a single valid Rego data term.
     *
     * @param term The contents of the term.
     * @returns either an error node or a nullptr if the input term is valid.
     */
    Node set_input_term(const std::string& term);

    /**
     * Sets the input document to the interpreter.
     *
     * Sets an AST node directly as the interpreter's input. Use with
     * caution.
     *
     * @param node The contents of the document.
     */
    Node set_input(const Node& node);

    Node set_query(const std::string& query);

    Interpreter& entrypoints(
      const std::initializer_list<std::string>& entrypoints);

    Interpreter& entrypoints(const std::vector<std::string>& entrypoints);

    const std::vector<std::string>& entrypoints() const;

    std::vector<std::string>& entrypoints();

    /**
     * Executes the documents against the interpreter.
     *
     * This method calls Interpreter::raw_query and then converts it into a
     * human-readable string.
     *
     * @return The result of the query.
     */
    std::string query();

    /**
     * Executes a query against the interpreter.
     *
     * This method calls Interpreter::raw_query and then converts it into a
     * human-readable string.
     *
     * @param query_expr The query expression.
     * @return The result of the query.
     */
    std::string query(const std::string& query_expr);

    /**
     * Executes a query against the interpreter.
     *
     * The query expression must be a valid Rego query expression. The result
     * will be an AST node representing the result, which will either be a
     * list of bindings and terms, or an error sequence.
     *
     * @param query_expr The query expression.
     * @return The result of the query.
     */
    Node raw_query(const std::string& query_expr);

    Node raw_query();

    Node build();

    Node save_bundle(const std::filesystem::path& dir, const Node& bundle);

    Node load_bundle(const std::filesystem::path& dir);

    Node query_bundle(const Bundle& bundle);

    Node query_bundle(const Bundle& bundle, const std::string& endpoint);

    /**
     * The path to the debug directory.
     *
     * If set, then (when in debug mode) the interpreter will output
     * intermediary ASTs after each compiler pass to the debug directory. If the
     * directory does not exist, it will be created.
     */
    Interpreter& debug_path(const std::filesystem::path& prefix);
    const std::filesystem::path& debug_path() const;

    /**
     * Whether debug mode is enabled.
     *
     * If true, then the interpreter will output intermediary ASTs after each
     * compiler pass to the debug directory set via Interpreter::debug_path.
     */
    Interpreter& debug_enabled(bool enabled);
    bool debug_enabled() const;

    /**
     * Whether well-formed checks are enabled.
     *
     * If true, then the interpreter will perform well-formedness checks after
     * each compiler pass using the well-formedness definitions.
     */
    Interpreter& wf_check_enabled(bool enabled);
    bool wf_check_enabled() const;

    /**
     * The built-ins used by the interpreter.
     *
     * This object can be used to register custom built-ins created using
     * BuiltInDef::create.
     */
    BuiltIns builtins() const;

    std::string output_to_string(const Node& output) const;

    LogLevel log_level() const;

    Interpreter& log_level(LogLevel level);

    /**
     * Sets the logging level for the interpreter.
     *
     * @param level The logging level. One of "None", "Error", "Output", "Warn",
     *              "Info", "Debug", or "Trace".
     * @return Error message if the level is invalid, otherwise empty string.
     */
    Interpreter& log_level(const std::string& level);

    const std::string& c_error() const;

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

  LogLevel log_level_from_string(const std::string& value);

  /** Bundle */

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

  /**
   * Parses Rego queries and virtual documents.
   */
  Reader file_to_rego();

  /**
   * Rewrites a Query AST to an input term.
   */
  Rewriter rego_to_input();

  /**
   * Rewrites a JSON AST to a Rego data input AST.
   */
  Rewriter json_to_rego(bool as_term = false);

  /**
   * Rewrites a Rego binding term to a JSON AST.
   */
  Rewriter rego_to_json();

  /**
   * Rewrites a Rego binding term to a YAML AST.
   */
  Rewriter rego_to_yaml();

  Rewriter json_to_bundle();

  Rewriter bundle_to_json();

  Rewriter rego_to_bundle(BuiltIns builtins = builtins::BuiltInsDef::create());
}