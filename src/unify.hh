#pragma once

#include "internal.hh"
#include "rego.hh"

namespace rego
{
  inline const auto Rego = TokenDef("rego", flag::symtab);
  inline const auto Data = TokenDef("rego-data", flag::lookup);
  inline const auto DataSeq = TokenDef("rego-dataseq");
  inline const auto ModuleSeq = TokenDef("rego-moduleseq");
  inline const auto Submodule =
    TokenDef("rego-submodule", flag::lookdown | flag::lookup);

  inline const auto RuleComp = TokenDef(
    "rego-rulecomp",
    flag::symtab | flag::lookup | flag::lookdown | flag::defbeforeuse);
  inline const auto RuleFunc = TokenDef(
    "rego-rulefunc",
    flag::symtab | flag::lookup | flag::lookdown | flag::defbeforeuse);
  inline const auto RuleSet = TokenDef(
    "rego-ruleset",
    flag::symtab | flag::lookup | flag::lookdown | flag::defbeforeuse);
  inline const auto RuleObj = TokenDef(
    "rego-ruleobj",
    flag::symtab | flag::lookup | flag::lookdown | flag::defbeforeuse);
  inline const auto DefaultRule =
    TokenDef("rego-defaultrule", flag::lookup | flag::lookdown);
  inline const auto Local =
    TokenDef("rego-local", flag::lookup | flag::shadowing);
  inline const auto ArithInfix = TokenDef("rego-arithinfix");
  inline const auto BinInfix = TokenDef("rego-bininfix");
  inline const auto BoolInfix = TokenDef("rego-boolinfix");
  inline const auto AssignInfix = TokenDef("rego-assigninfix");
  inline const auto ArgVar = TokenDef("rego-argvar", flag::lookup);
  inline const auto ArgVal = TokenDef("rego-argval");
  inline const auto LiteralWith = TokenDef("rego-literalwith");
  inline const auto LiteralEnum = TokenDef("rego-literalenum");
  inline const auto LiteralInit = TokenDef("rego-literalinit");
  inline const auto LiteralNot = TokenDef("rego-literalnot");
  inline const auto Enumerate = TokenDef("rego-enumerate");
  inline const auto Merge = TokenDef("rego-merge");
  inline const auto UnifyBody = TokenDef("rego-unifybody");
  inline const auto Arg = TokenDef("rego-arg");
  inline const auto Body = TokenDef("rego-body");
  inline const auto Empty = TokenDef("rego-empty");
  inline const auto NestedBody = TokenDef("rego-nestedbody", flag::symtab);
  inline const auto Item = TokenDef("rego-item");
  inline const auto ItemSeq = TokenDef("rego-itemseq");
  inline const auto SimpleRef = TokenDef("rego-simpleref");
  inline const auto Function = TokenDef("rego-function");
  inline const auto UnifyExpr = TokenDef("rego-unifyexpr");
  inline const auto UnifyExprNot = TokenDef("rego-unifyexprnot");
  inline const auto UnifyExprWith = TokenDef("rego-unifyexprwith");
  inline const auto UnifyExprEnum = TokenDef("rego-unifyexrenum");
  inline const auto UnifyExprCompr = TokenDef("rego-unifyexprcompr");
  inline const auto Compr = TokenDef("rego-compr");
  inline const auto SkipSeq = TokenDef("rego-skipseq");
  inline const auto Skip = TokenDef("rego-skip", flag::lookup);
  inline const auto BuiltInHook = TokenDef("rego-builtinhook", flag::lookup);
  inline const auto ElseSeq = TokenDef("rego-elseseq");
  inline const auto TermSet = TokenDef("rego-termset");

  const std::set<Token> RuleTypes(
    {RuleComp, RuleFunc, RuleSet, RuleObj, DefaultRule});

  const inline auto ArithInfixArg =
    T(Expr, NumTerm, Ref, UnaryExpr, ArithInfix, RefTerm, ExprCall);
  const inline auto BinInfixArg =
    T(Expr, Ref, RefTerm, ExprCall, Set, SetCompr, BinInfix);

  // clang-format off
  inline const auto wf_unify_input =
    rego::wf
    | (Top <<= Rego)
    | (Rego <<= Query * Input * DataSeq * ModuleSeq)
    | (ModuleSeq <<= Module++)
    | (Input <<= DataTerm | Undefined)
    | (DataSeq <<= Data++)
    | (Data <<= DataTerm)
    | (DataObject <<= DataObjectItem++)
    | (DataObjectItem <<= (Key >>= DataTerm) * (Val >>= DataTerm))
    | (DataTerm <<= Scalar | DataArray | DataObject | DataSet)
    | (DataArray <<= DataTerm++)
    | (DataSet <<= DataTerm++)
    | (Import <<= Ref * Var)[Var]
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_strings =
    wf_unify_input
    | (Scalar <<= JSONString | Int | Float | True | False | Null)
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

  // clang-format off
  inline const auto wf_pass_lift_refheads = wf_pass_merge_data;
  // clang-format on

  inline const auto wf_symbols_exprs =
    (wf_exprs - ExprEvery) | RefTerm | NumTerm;

  // clang-format off
  inline const auto wf_pass_symbols =
    wf_pass_lift_refheads
    | (Module <<= Package * Policy)
    | (Policy <<= (Import | RuleComp | RuleFunc | RuleSet | RuleObj)++)
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * (Idx >>= Int))[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * (Idx >>= Int))[Var]
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= Expr | Term) * (Idx >>= Int))[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Key >>= Expr | Term) * (Val >>= Expr | Term) * (Idx >>= Int))[Var]
    | (UnifyBody <<= (Local | Literal | LiteralWith | LiteralEnum)++[1])
    | (Local <<= Var * (Val >>= Undefined))[Var]
    | (Literal <<= Expr | NotExpr | SomeDecl)
    | (LiteralEnum <<= (Item >>= Var) * (ItemSeq >>= Expr))
    | (LiteralWith <<= UnifyBody * WithSeq)
    | (With <<= RuleRef * Expr)
    | (ExprCall <<= RuleRef * ExprSeq)
    | (Query <<= (Body >>= UnifyBody))
    | (Term <<= Scalar | Array | Object | Set | ArrayCompr | SetCompr | ObjectCompr | Membership)
    | (ObjectCompr <<= Expr * Expr * (Body >>= NestedBody))
    | (ArrayCompr <<= Expr * (Body >>= NestedBody))
    | (SetCompr <<= Expr * (Body >>= NestedBody))
    | (RefTerm <<= Ref | Var)
    | (NumTerm <<= Int | Float)
    | (RefArgBrack <<= RefTerm | Scalar | Object | Array | Set | Expr)
    | (Expr <<= wf_symbols_exprs)
    | (NestedBody <<= Key * (Val >>= UnifyBody))
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_replace_argvals =
    wf_pass_symbols
    | (RuleArgs <<= ArgVar++)
    | (Literal <<= Expr | NotExpr)
    ;
  // clang-format on

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
    wf_pass_expand_imports
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | DataTerm) * (Idx >>= Int))[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | DataTerm) * (Idx >>= Int))[Var]
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= Expr | DataTerm) * (Idx >>= Int))[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Key >>= Expr | DataTerm) * (Val >>= Expr | DataTerm) * (Idx >>= Int))[Var]
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

  inline const auto wf_infix_exprs =
    (wf_symbols_exprs | ArithInfix | BinInfix | BoolInfix);

  // clang-format off
  inline const auto wf_pass_infix =
    wf_pass_skips
    | (ArithInfix <<= Expr * (Op >>= Add | Subtract | Multiply | Divide | Modulo) * Expr)
    | (BinInfix <<= Expr * (Op >>= And | Or) * Expr)
    | (BoolInfix <<= Expr * (Op >>= Equals | NotEquals | LessThan | LessThanOrEquals | GreaterThan | GreaterThanOrEquals) * Expr)
    | (Expr <<= wf_infix_exprs)
    | (UnifyBody <<= (Local | Literal | LiteralWith | LiteralEnum | LiteralNot)++[1])
    | (LiteralNot <<= UnifyBody)
    | (Literal <<= Expr)
    ;
  // clang-format on

  inline const auto wf_assign_exprs =
    (wf_infix_exprs | AssignInfix) - ExprInfix;

  // clang-format off
  inline const auto wf_pass_assign =
    wf_pass_infix
    | (AssignInfix <<= AssignArg * AssignArg)
    | (AssignArg <<= Expr)
    | (Expr <<= wf_assign_exprs)
    ;
  // clang-format on

  inline const auto wf_pass_skip_refs = wf_pass_assign;

  // clang-format off
  inline const auto wf_pass_simple_refs =
    wf_pass_skip_refs
    | (AssignArg <<= RefTerm | NumTerm | Term | ExprCall | UnaryExpr | ArithInfix | BinInfix | BoolInfix)
    | (RefTerm <<= Var | SimpleRef)
    | (SimpleRef <<= Var * (Op >>= RefArgDot | RefArgBrack))
    | (Expr <<= wf_assign_exprs)
    | (ExprCall <<= Var * ExprSeq)
    | (RefHead <<= Var)
    | (RuleRef <<= Var)
    ;
  // clang-format on

  // clang-format off
  inline const auto wf_pass_init =
    wf_pass_simple_refs
    | (UnifyBody <<= (Local | Literal | LiteralWith | LiteralEnum | LiteralNot | LiteralInit)++[1])
    | (LiteralInit <<= (Lhs >>= VarSeq) * (Rhs >>= VarSeq) * AssignInfix)
    | (VarSeq <<= Var++)
    ;
  // clang-format on

  inline const auto wf_pass_implicit_enums = wf_pass_init;

  inline const auto wf_pass_enum_locals = wf_pass_implicit_enums;

  inline const auto wf_rulebody_exprs = (wf_assign_exprs - AssignInfix);

  // clang-format off
  inline const auto wf_pass_rulebody =
    wf_pass_enum_locals
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

  inline const auto wf_lift_to_rule_exprs =
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
    | (UnifyExpr <<= Var * (Val >>= Var | Scalar | Array | Set | Object | Function))
    | (Function <<= JSONString * ArgSeq)
    | (ArgSeq <<= (Scalar | Object | Array | Set | Var | wf_arith_op | wf_bin_op | wf_bool_op | NestedBody | VarSeq)++)
    | (Input <<= Key * (Val >>= Term | Undefined))[Key]
    | (Array <<= Term++)
    | (Set <<= Term++)
    | (Object <<= ObjectItem++)
    | (ObjectItem <<= (Key >>= Term) * (Val >>= Term))
    | (RuleComp <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * (Idx >>= Int) * Key)[Var]
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * (Idx >>= Int) * Key)[Var]
    | (RuleSet <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * Key)[Var]
    | (RuleObj <<= Var * (Body >>= UnifyBody | Empty) * (Val >>= UnifyBody | Term) * Key)[Var]
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

  PassDef strings();
  PassDef merge_data();
  PassDef lift_refheads();
  PassDef symbols();
  PassDef replace_argvals();
  PassDef lift_query();
  PassDef expand_imports(BuiltIns builtins);
  PassDef constants();
  PassDef explicit_enums();
  PassDef body_locals(BuiltIns builtins);
  PassDef value_locals(BuiltIns builtins);
  PassDef compr_locals(BuiltIns builtins);
  PassDef rules_to_compr();
  PassDef compr();
  PassDef absolute_refs();
  PassDef merge_modules();
  PassDef datarule();
  PassDef skips();
  PassDef infix();
  PassDef assign(BuiltIns builtins);
  PassDef skip_refs(BuiltIns builtins);
  PassDef simple_refs();
  PassDef init();
  PassDef implicit_enums();
  PassDef enum_locals();
  PassDef rulebody();
  PassDef lift_to_rule();
  PassDef functions();
  PassDef unifier(BuiltIns builtins);
  PassDef result();

  class ValueDef;
  using rank_t = std::size_t;
  using Value = std::shared_ptr<ValueDef>;
  using Values = std::vector<Value>;
  using RankedNode = std::pair<rank_t, Node>;

  class ValueDef
  {
  public:
    void mark_as_valid();
    void mark_as_invalid();
    void reduce_set();

    const std::string& str() const;
    const std::string& json() const;
    bool depends_on(const Value& value) const;
    bool invalid() const;
    const Location& var() const;
    const Node& node() const;
    const Values& sources() const;
    Node to_term() const;
    rank_t rank() const;
    friend std::ostream& operator<<(std::ostream& os, const Value& value);
    friend std::ostream& operator<<(std::ostream& os, const ValueDef& value);
    friend bool operator==(const Value& lhs, const Value& rhs);
    friend bool operator<(const Value& lhs, const Value& rhs);

    static Value create(const RankedNode& value);
    static Value create(const Location& var, const RankedNode& value);
    static Value create(
      const Location& var, const RankedNode& value, const Values& sources);
    static Value create(const Node& value);
    static Value create(const Location& var, const Node& value);
    static Value create(
      const Location& var, const Node& value, const Values& sources);
    static Value copy_to(const Value& value, const Location& var);
    static Values filter_by_rank(const Values& values);
    static rank_t get_rank(const Node& node);

  private:
    static void build_string(
      std::ostream& buf,
      const ValueDef& current,
      const Location& root,
      bool first);
    Location m_var;
    Node m_node;
    Values m_sources;
    bool m_invalid;
    rank_t m_rank;
    std::string m_str;
    std::string m_json;
    ValueDef(
      const Location& var,
      const Node& value,
      const Values& sources,
      rank_t rank);
    ValueDef(const Node& value);
    ValueDef(const RankedNode& value);
    ValueDef(const Location& var, const Node& value);
    ValueDef(const Location& var, const RankedNode& value);
    ValueDef(const Location& var, const Node& value, const Values& sources);
    ValueDef(
      const Location& var, const RankedNode& value, const Values& sources);
  };

  class ValueMap
  {
  public:
    ValueMap();
    ValueMap(
      const Values::const_iterator& begin, const Values::const_iterator& end);
    bool contains(const Value& value) const;
    bool intersect_with(const Values& values);
    bool remove_values_not_contained_in(const Values& values);
    void clear();
    bool insert(const Value& value);
    bool erase(const std::string& json);
    bool empty() const;
    Nodes to_terms() const;
    Values valid_values() const;
    bool remove_invalid_values();
    void mark_valid_values(bool include_falsy);
    void mark_invalid_values();

    friend std::ostream& operator<<(std::ostream& os, const ValueMap& values);

  private:
    std::multimap<std::string, Value> m_map;
    std::set<std::pair<std::string, std::string>> m_values;
    std::set<std::string> m_keys;
  };

  /**
   * The Args class provides an interable interface over the Cartesian product
   * of all sets of arguments to a function. It will contain N sets of argument
   * sources, and will produce s_0 * s_1 * ... * s_n arguments, where s_i is the
   * size of the i-th argument source.
   */
  class Args
  {
  public:
    Args();
    void push_back_source(const Values& values);
    void mark_invalid(const std::set<Value>& active) const;
    void mark_invalid_except(const std::set<Value>& active) const;
    Values at(std::size_t index) const;
    std::size_t size() const;
    std::string str() const;
    std::size_t source_size() const;
    const Values& source_at(std::size_t index) const;
    Args subargs(std::size_t start) const;
    friend std::ostream& operator<<(std::ostream& os, const Args& args);

  private:
    std::vector<Values> m_values;
    std::vector<size_t> m_stride;
    std::size_t m_size;
  };

  class Variable
  {
  public:
    Variable(const Node& local, std::size_t id);
    bool unify(const Values& others);
    std::string str() const;
    bool remove_invalid_values();
    void mark_invalid_values();
    void mark_valid_values();
    Values valid_values() const;
    Node to_term() const;
    Node bind();
    void reset();

    bool is_unify() const;
    bool is_user_var() const;
    Location name() const;
    std::size_t id() const;

    static bool is_unify(const std::string_view& name);
    static bool is_user_var(const std::string_view& name);

    friend std::ostream& operator<<(std::ostream& os, const Variable& variable);
    friend std::ostream& operator<<(
      std::ostream& os, const std::map<Location, Variable>& variables);

  private:
    bool intersect_with(const Values& others);
    bool initialize(const Values& others);

    Node m_local;
    ValueMap m_values;
    bool m_unify;
    bool m_user_var;
    bool m_initialized;
    std::size_t m_id;
  };

  enum class UnifierType
  {
    RuleBody,
    RuleValue
  };

  struct UnifierKey
  {
    Location key;
    UnifierType type;
    bool operator<(const UnifierKey& other) const;
  };

  class UnifierDef;
  using Unifier = std::shared_ptr<UnifierDef>;
  using CallStack = std::shared_ptr<std::vector<Location>>;
  using ValuesLookup = std::map<std::string, Values>;
  using WithStack = std::shared_ptr<std::vector<ValuesLookup>>;
  using UnifierCache = std::shared_ptr<std::map<UnifierKey, Unifier>>;

  class UnifierDef
  {
  public:
    UnifierDef(
      const Location& rule,
      const Node& rulebody,
      CallStack call_stack,
      WithStack with_stack,
      BuiltIns builtins,
      UnifierCache unifier_cache);
    Node unify();
    Nodes expressions() const;
    Nodes bindings() const;
    std::string str() const;
    std::string dependency_str() const;
    static Unifier create(
      const UnifierKey& key,
      const Location& rule,
      const Node& rulebody,
      const CallStack& call_stack,
      const WithStack& with_stack,
      BuiltIns builtins,
      const UnifierCache& unifier_cache);
    std::optional<Node> resolve_rule(const Nodes& defs) const;
    std::optional<Node> resolve_rulecomp(const Nodes& rulecomp) const;
    std::optional<Node> resolve_ruleset(const Nodes& ruleset) const;
    std::optional<Node> resolve_ruleobj(const Nodes& ruleobj) const;
    std::optional<RankedNode> resolve_rulefunc(
      const Node& rulefunc, const Nodes& args) const;
    Node resolve_module(const Node& module) const;

    struct Dependency
    {
      std::string name;
      std::set<std::size_t> dependencies;
      std::size_t score;
    };

    struct Statement
    {
      std::size_t id;
      Node node;
    };

  private:
    Unifier rule_unifier(
      const UnifierKey& key, const Location& rule, const Node& rulebody) const;
    void init_from_body(
      const Node& rulebody,
      std::vector<Statement>& statements,
      std::size_t root);
    std::size_t add_variable(const Node& local);
    std::size_t add_unifyexpr(const Node& unifyexpr);
    void add_withpush(const Node& withpush);
    void add_withpop(const Node& withpop);
    void reset();

    void compute_dependency_scores();
    std::size_t compute_dependency_score(
      std::size_t index, std::set<size_t>& visited);
    std::size_t dependency_score(const Variable& var) const;
    std::size_t dependency_score(const Statement& stmt) const;
    std::size_t detect_cycles() const;
    bool has_cycle(std::size_t id) const;

    Values evaluate(const Location& var, const Node& value);
    Values evaluate_function(
      const Location& var, const std::string& func_name, const Args& args);
    Values resolve_var(const Node& var, bool exclude_with = false);
    Values enumerate(const Location& var, const Node& container);
    Values resolve_skip(const Node& skip);
    Values check_with(const Node& var, bool bypass_recurse_check = false);
    Values apply_access(const Location& var, const Values& args);
    std::optional<Value> call_function(
      const Location& var, const Values& args) const;
    std::optional<Value> call_named_function(
      const Location& var, const std::string& name, const Values& args);
    bool is_local(const Node& var);
    std::size_t scan_vars(const Node& expr, std::vector<Location>& locals);
    void pass();
    void execute_statements(
      std::vector<Statement>::iterator begin,
      std::vector<Statement>::iterator end);
    void remove_invalid_values();
    void mark_invalid_values();
    Variable& get_variable(const Location& name);
    bool is_variable(const Location& name) const;
    Node bind_variables();
    Args create_args(const Node& args);
    bool would_recurse(const Node& node);

    bool push_rule(const Location& rule);
    void pop_rule(const Location& rule);

    void push_with(const Node& withseq);
    void pop_with();

    void push_not();
    void pop_not();

    Location m_rule;
    std::map<Location, Variable> m_variables;
    std::vector<Statement> m_statements;
    std::map<std::size_t, std::vector<Statement>> m_nested_statements;
    CallStack m_call_stack;
    WithStack m_with_stack;
    BuiltIns m_builtins;
    UnifierCache m_cache;
    std::size_t m_retries;
    Token m_parent_type;
    std::vector<Dependency> m_dependency_graph;
    bool m_negate;
  };

  Node expr_infix(Token op_token, Node lhs, Node rhs);
}

// Use ADL to extend Trieste logging capabilities with additional types.
namespace trieste::logging
{
  inline void append(
    Log& log, const std::vector<rego::UnifierDef::Dependency>& deps)
  {
    for (auto it = deps.begin(); it != deps.end(); ++it)
    {
      auto& dep = *it;
      log << "[" << dep.name << "](" << dep.score << ") -> {";

      logging::Sep sep{", "};
      for (auto& idx : dep.dependencies)
      {
        log << sep << deps[idx].name;
      }
      log << "}" << std::endl;
    }
  }

  template <typename T>
  inline void append(Log& log, const std::vector<T>& values)
  {
    log << "[";
    logging::Sep sep{", "};
    for (auto& value : values)
    {
      log << sep << value;
    }
    log << "]" << std::endl;
  }

  inline void append(Log& log, const Location& loc)
  {
    log << loc.view();
  }

  inline void append(Log& log, const rego::UnifierDef::Statement& statement)
  {
    rego::Resolver::stmt_str(log, statement.node);
  }

  template <typename K, typename V>
  inline void append(Log& log, const rego::MapValuesStr<K, V>& map)
  {
    log << "{" << std::endl;
    for (auto& [_, value] : map.values)
    {
      log << value << std::endl;
    }
    log << "}" << std::endl;
  }
} // trieste::logging