// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "rego_c.h"

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

  inline const auto Module = TokenDef("rego-module", flag::symtab);
  inline const auto Package = TokenDef("rego-package");
  inline const auto Policy = TokenDef("rego-policy");
  inline const auto Rule = TokenDef("rego-rule", flag::symtab);
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
  inline const auto ExprEvery = TokenDef("rego-exprevery");
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
  inline const auto ArrayCompr = TokenDef("rego-arraycompr");
  inline const auto ObjectCompr = TokenDef("rego-objectcompr");
  inline const auto SetCompr = TokenDef("rego-setcompr");
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

  // other tokens
  inline const auto Results = TokenDef("rego-results", flag::symtab);
  inline const auto Result = TokenDef("rego-result", flag::symtab);
  inline const auto Bindings = TokenDef("rego-bindings");
  inline const auto Terms = TokenDef("rego-terms");
  inline const auto Binding = TokenDef("rego-binding", flag::lookdown);
  inline const auto ErrorCode = TokenDef("rego-errorcode");
  inline const auto ErrorSeq = TokenDef("rego-errorseq");

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

  struct BuiltInDef;
  /** A pointer to a BuiltInDef.*/
  using BuiltIn = std::shared_ptr<BuiltInDef>;

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

  /**
   * Sets the location where rego-cpp will look for timezone database
   * information.
   *
   * The timezone database will be interpreted as one obtained from the IANA
   * (https://www.iana.org/time-zones) which has been downloaded and unpacked
   * into at the path provided. If the library was built without manual tzdata
   * support, this function will throw an exception.
   *
   * @param path The path to the timezone database.
   */
  void set_tzdata_path(const std::filesystem::path& path);

  /** This constant indicates that a built-in can receive any number of
   * arguments. */
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
     * Constructor.
     */
    BuiltInDef(Location name_, std::size_t arity_, BuiltInBehavior behavior_);

    virtual ~BuiltInDef() = default;

    /**
     * Called to clear any persistent state or caching.
     */
    virtual void clear();

    /**
     * Creates a new built-in.
     *
     * BuiltIn is a pointer to a BuiltInDef.
     *
     * @param name The name of the built-in.
     * @param arity The number of arguments expected by the built-in.
     * @param behavior The function which will be called when the built-in is
     * evaluated.
     * @return The built-in.
     */
    static BuiltIn create(
      const Location& name, std::size_t arity, BuiltInBehavior behavior);
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
    Node call(const Location& name, const Location& version, const Nodes& args);

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
  const std::string DefaultVersion = "v0";

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
    bool sort_arrays = false);

  /**
   * The logging level.
   */
  enum class LogLevel : char
  {
    None = REGO_LOG_LEVEL_NONE,
    Error = REGO_LOG_LEVEL_ERROR,
    Output = REGO_LOG_LEVEL_OUTPUT,
    Warn = REGO_LOG_LEVEL_WARN,
    Info = REGO_LOG_LEVEL_INFO,
    Debug = REGO_LOG_LEVEL_DEBUG,
    Trace = REGO_LOG_LEVEL_TRACE,
  };

  /**
   * Sets the logging level for the library.
   *
   * @param level The logging level.
   */
  void set_log_level(LogLevel level);

  /**
   * Sets the logging level for the library.
   *
   * @param level The logging level. One of "None", "Error", "Output", "Warn",
   *              "Info", "Debug", or "Trace".
   * @return Error message if the level is invalid, otherwise empty string.
   */
  std::string set_log_level_from_string(const std::string& level);

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
   * rego.add_input_json_file("input0.json");
   * std::cout << rego.query("[data.one, input.b, data.objects.sites[1]]") <<
   * std::endl;
   * ```
   */
  class Interpreter
  {
  public:
    /**
     * Constructor.
     *
     * @param v1_compatible whether the Interpreter should run in rego-v1 mode.
     */
    Interpreter(bool v1_compatible = false);

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
     * This is the same as calling Interpreter::add_input_json with the contents
     * of the file.
     *
     * @param path The path to the input file.
     * @returns either an error node or a nullptr if the input document is
     * valid.
     */
    Node set_input_json_file(const std::filesystem::path& path);

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
    BuiltInsDef& builtins() const;

  private:
    friend const char* ::regoGetError(regoInterpreter* rego);
    friend void setError(regoInterpreter* rego, const std::string& error);
    friend regoOutput* ::regoQuery(
      regoInterpreter* rego, const char* query_expr);

    void merge(const Node& ast);
    std::string output_to_string(const Node& output) const;
    Reader m_reader;
    Node m_ast;
    std::filesystem::path m_debug_path;
    BuiltIns m_builtins;
    Rewriter m_unify;
    Reader m_json;
    Rewriter m_from_json;
    Rewriter m_to_input;
    std::size_t m_data_count;

    std::string m_c_error;
  };

  /**
   * Parses Rego queries and virtual documents.
   */
  Reader reader(bool v1_compatible = false);

  /**
   * Rewrites a Query AST to an input term.
   */
  Rewriter to_input();

  /**
   * Unifies a set of base and virtual documents, an input term, and a query.
   */
  Rewriter unify(BuiltIns builtins = BuiltInsDef::create());

  /**
   * Rewrites a JSON AST to a Rego data input AST.
   */
  Rewriter from_json(bool as_term = false);

  /**
   * Rewrites a Rego binding term to a JSON AST.
   */
  Rewriter to_json();

  /**
   * Rewrites a Rego binding term to a YAML AST.
   */
  Rewriter to_yaml();
}