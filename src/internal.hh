// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "rego/rego.hh"

#include <chrono>

namespace rego
{
  using namespace trieste;

  class Resolver
  {
  public:
    static Node scalar(BigInt value);
    static Node scalar(double value);
    static Node scalar(bool value);
    static Node scalar(const char* value);
    static Node scalar(const std::string& value);
    static Node scalar();
    static Node term(BigInt value);
    static Node term(double value);
    static Node term(bool value);
    static Node term(const char* value);
    static Node term(const std::string& value);
    static Node term();
    static Node arithinfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node bininfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node boolinfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node object(const Node& object_items);
    static Node array(const Node& array_members);
    static Node array(const Nodes& array_members);
    static Node set(const Node& set_members);
    static Node set_intersection(const Node& lhs, const Node& rhs);
    static Node set_union(const Node& lhs, const Node& rhs);
    static Node set_difference(const Node& lhs, const Node& rhs);
    static Nodes object_lookdown(const Node& object, const Node& query);
    static Nodes object_lookdown(
      const Node& object, const std::string_view& query);
    static Node membership(
      const Node& index, const Node& item, const Node& itemseq);
    static Node membership(const Node& item, const Node& itemseq);
    static std::vector<std::string> array_find(
      const Node& array, const std::string& search);
    static std::vector<std::string> object_find(
      const Node& object, const std::string& search);
    static Node to_term(const Node& value);
  };

  inline const auto NewLine = TokenDef("rego-newline");
  inline const auto Brace = TokenDef("rego-brace");
  inline const auto Dot = TokenDef("rego-dot");
  inline const auto Colon = TokenDef("rego-colon");
  inline const auto Square = TokenDef("rego-square");
  inline const auto Paren = TokenDef("rego-paren");
  inline const auto Comma = TokenDef("rego-comma");
  inline const auto EmptySet = TokenDef("rego-emptyset");

  inline const auto wf_json = JSONString | Int | Float | True | False | Null;
  inline const auto wf_parse_tokens = Query | Module | wf_json | wf_arith_op |
    wf_bool_op | wf_bin_op | Package | Var | Brace | Square | Dot | Paren |
    Assign | Unify | EmptySet | Colon | RawString | Default | Some | Import |
    Else | As | With | NewLine | Comma | If | IsIn | Contains | Every;

  // clang-format off
  inline const auto wf_parser =
    (Top <<= File)
    | (File <<= Group)
    | (Module <<= Group++[1])
    | (Query <<= Group)
    | (Brace <<= Group++)
    | (Square <<= Group++)
    | (Paren <<= Group++)
    | (Group <<= wf_parse_tokens++)
    ;
  // clang-format on

  Parse parser();

  // Utility tokens
  inline const auto Op = TokenDef("rego-op");
  inline const auto Id = TokenDef("rego-id");
  inline const auto Head = TokenDef("rego-head");
  inline const auto Tail = TokenDef("rego-tail");

  inline const auto Input = TokenDef("rego-input", flag::lookup);
  inline const auto DataSeq = TokenDef("rego-dataseq");
  inline const auto ModuleSeq = TokenDef("rego-moduleseq");
  inline const auto DataModule = TokenDef("rego-datamodule", flag::lookup);
  inline const auto DataTerm = TokenDef("rego-dataterm");
  inline const auto DataObject = TokenDef("rego-dataobject");
  inline const auto DataObjectItem = TokenDef("rego-dataobjectitem");
  inline const auto DataArray = TokenDef("rego-dataarray");
  inline const auto DataSet = TokenDef("rego-dataset");
  inline const auto DataRule = TokenDef("rego-datarule", flag::lookup);

  inline const auto BaseDocument =
    TokenDef("rego-basedocument", flag::lookup | flag::symtab);
  inline const auto VirtualDocument = TokenDef(
    "rego-virtualdocument", flag::lookup | flag::lookdown | flag::symtab);
  inline const auto BaseObject = TokenDef("rego-baseobject", flag::symtab);
  inline const auto BaseObjectItem =
    TokenDef("rego-baseobjectitem", flag::lookup | flag::lookdown);
  inline const auto DocumentSeq = TokenDef("rego-documentseq");
  inline const auto RuleSeq = TokenDef("rego-ruleseq");
  inline const auto RuleHeadObjDynamic = TokenDef("rego-ruleheadobjdynamic");
  inline const auto RuleHeadSetDynamic = TokenDef("rego-ruleheadsetdynamic");
  inline const auto ArgVal = TokenDef("rego-argval");

  // internal nodes
  inline const auto LocalSeq = TokenDef("rego-localseq");
  inline const auto Local =
    TokenDef("rego-local", flag::lookup | flag::shadowing);
  inline const auto EveryLocal =
    TokenDef("rego-everylocal", flag::lookup | flag::shadowing);
  inline const auto Ident = TokenDef("rego-ident", flag::print);
  inline const auto ExprAssign = TokenDef("rego-exprassign", flag::lookup);
  inline const auto ExprIsArray = TokenDef("rego-exprisarray");
  inline const auto ExprIsObject = TokenDef("rego-exprisobject");
  inline const auto ExprScan = TokenDef("rego-exprscan");
  inline const auto ExprUnify = TokenDef("rego-exprunify");
  inline const auto ExprAssignFromArray = TokenDef("rego-exprassignfromarray");
  inline const auto ExprAssignFromObject =
    TokenDef("rego-exprassignfromobject");
  inline const auto LocalRef = TokenDef("rego-localref", flag::print);
  inline const auto OpBlock = TokenDef("rego-opblock");
  inline const auto OpBlockSeq = TokenDef("rego-opblockseq");
  inline const auto AssignBlock = TokenDef("rego-assignblock");
  inline const auto WithCallStmt = TokenDef("rego-withcallstmt");
  inline const auto BuiltInCallStmt = TokenDef("rego-builtincallstmt");

  inline const auto AssignVar = TokenDef("rego-assignvar", flag::print);
  inline const auto FuncArgVar = TokenDef("rego-funcargvar", flag::print);
  inline const auto UnifyVar = TokenDef("rego-unifyvar", flag::print);
  inline const auto Alias = TokenDef("rego-alias");

  // clang-format off
  inline const auto wf_bundle_input =
    wf
    | (Top <<= RegoBundle)
    | (RegoBundle <<= EntryPointSeq * DataSeq * ModuleSeq * Query)
    | (EntryPointSeq <<= IRString++)
    | (ModuleSeq <<= Module++)
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

  template <typename I, typename S, typename F>
  S& join(S& stream, const I& begin, const I& end, const char* sep, F&& writer)
  {
    if (begin == end)
    {
      return stream;
    }

    bool write_sep = false;
    for (auto it = begin; it != end; ++it)
    {
      if (write_sep)
      {
        stream << sep;
      }

      write_sep = writer(stream, *it);
    }

    return stream;
  }

  class DependencyGraph
  {
    struct LocalNode
    {
      std::string name;
      bool captured;
      std::optional<size_t> in_edge;
      std::vector<size_t> out_edges;
    };

    struct LiteralNode
    {
      size_t index;
      Node literal;
      std::set<std::string> out_edges;
      std::set<std::string> in_edges;
      std::string_view str() const;
    };

    struct Subgraph
    {
      std::set<size_t> indices;
      std::set<std::string> in_edges;
      std::set<std::string> out_edges;
    };

    std::vector<LiteralNode> m_literals;
    std::deque<Node> m_unify_literals;
    std::map<std::string, LocalNode> m_locals;
    std::vector<Subgraph> m_graphs;
    std::vector<std::string> m_captures;
    Nodes m_nodes;
    Node m_scope;
    Node m_error;
    Node m_orderedseq;
    bool m_needs_sort;

    bool is_assigned(const std::string& name);
    bool any_unassigned(const Nodes& nodes);
    void add_literal(Node literal, Location location);
    void add_assign(Node var, Node expr);
    void add_equals(Node lhs, Node rhs);
    bool add_unifyvars(
      Node lhs_var, std::string& lhs_name, Node rhs_var, std::string& rhs_name);
    bool add_unifyvar_term(Node lhs_var, std::string& lhs_name, Node rhs_term);
    bool add_array_var(Node lhs_array, Node rhs_var);
    bool add_object_var(Node lhs_object, Node rhs_var);
    bool add_array_array(Node lhs_array, Node rhs_array);
    bool add_object_object(Node lhs_object, Node rhs_object);
    bool add_terms(Node lhs_term, Node rhs_term);
    bool add_term_var(Node lhs_term, Node rhs_var);
    bool add_expr_var(Node lhs_expr, Node rhs_rvar);
    bool add_term_expr(Node lhs_term, Node rhs_expr);
    bool add_exprs(Node lhs_expr, Node rhs_expr);
    void update_edges(LiteralNode& literal);
    void add_locals(std::set<std::string>& names, Node node);
    void resolve_unify_literals();
    void add_rule_locals();
    void add_unify_literals();
    void add_captures();
    void find_graphs();
    void topological_sort(const std::set<size_t>& graph);

  public:
    DependencyGraph(Node rule, const NodeRange& nodes);
    Node sort();
    void log() const;
    void merge_locals(std::set<std::string>& locals) const;
    Node orderedseq() const;
    const std::vector<std::string>& captures() const;
  };

  // internal
  Node to_operand(Node block, Node operand);
  Node get_inner(Node block);
  void add_literal_to_block(Node block, Node literal);
  Node to_absolute_path(Node ref);
  Node base_block(Node base_term, const Location& value_name);
  Node rule_stmt(Node rule, const Location& value_name);
  Node rule_dynamic_block(Node rule, const Location& value_name);
  Node document_stmt(
    Node document, const Location& doc_name, Node refargseq, size_t start);
  bool is_constant(const Node& node);
  bool is_instance(const Node& value, const std::vector<Token>& types);
  bool is_falsy(const Node& node);
  bool is_truthy(const Node& node);
  Node to_constant_term(Node expr);
  std::string strip_quotes(const std::string_view& str);
  std::string add_quotes(const std::string_view& str);
  std::string type_name(const Node& node, bool specify_number = false);

  inline bool is_quoted(const std::string_view& str)
  {
    return str.size() >= 2 && str.front() == str.back() && str.front() == '"';
  }

  inline std::int32_t to_int32(Node value)
  {
    std::string value_str(value->location().view());
    return std::stoi(value_str);
  }

  inline std::uint32_t to_uint32(Node value)
  {
    std::string value_str(value->location().view());
    return std::stoul(value_str);
  }

  inline std::int64_t to_int64(Node value)
  {
    std::string value_str(value->location().view());
    return std::stoll(value_str);
  }

  inline size_t to_size(Node value)
  {
    return static_cast<size_t>(to_uint32(value));
  }

  // opblocks
  Node jsonstring_to_opblock(Node term);
  Node boolean_to_opblock(Node term);
  Node number_to_opblock(Node term);
  Node null_to_opblock(Node term);
  Node scalar_to_opblock(Node scalar);
  Node object_to_opblock(Node object);
  Node array_to_opblock(Node array);
  Node set_to_opblock(Node set);
  Node membership_to_opblock(Node membership);
  Node arraycompr_to_opblock(Node arraycompr);
  Node setcompr_to_opblock(Node setcompr);
  Node objectcompr_to_opblock(Node objectcompr);
  Node call_to_opblock(Node call);
  Node var_to_opblock(Node call);
  Node ref_to_opblock(Node call);
  Node term_to_opblock(Node term);

  class ActionMetrics
  {
  public:
    ActionMetrics(const char* file, std::size_t line);
    ~ActionMetrics();
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration = clock::duration;

    static void print();

  private:
    struct key_t
    {
      std::string_view file;
      std::size_t line;

      bool operator<(const key_t& other) const;
    };

    struct info_t
    {
      std::size_t count;
      duration time_spent;
    };

    key_t m_key;
    time_point m_start;
    static std::map<key_t, info_t> s_action_info;
  };

  // bundle

  namespace bundle
  {
    Block node_to_block(Node block, std::shared_ptr<size_t> max_index);
    Statement node_to_statement(
      Node statement, std::shared_ptr<size_t> max_index);
    Function node_to_function(Node function, std::shared_ptr<size_t> max_index);
    Plan node_to_plan(Node plan, std::shared_ptr<size_t> max_index);
  }
}

#ifdef REGOCPP_ACTION_METRICS
#define ACTION() rego::ActionMetrics __action_metrics(__FILE__, __LINE__)
#define PRINT_ACTION_METRICS() rego::ActionMetrics::print()
#else
#define ACTION()
#define PRINT_ACTION_METRICS()
#endif