// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "rego/rego.hh"

#include <chrono>

namespace rego
{
  inline const std::set<std::string> Keywords(
    {"if", "in", "contains", "every"});

  bool all_alnum(const std::string_view& str);
  bool contains_local(const Node& node);
  bool contains_ref(const Node& node);
  bool refargs_contain_varref(const Node& node);
  Node concat_refs(const Node& lhs, const Node& rhs);
  std::string flatten_ref(const Node& ref);
  bool is_in(const Node& node, const std::set<Token>& token);
  bool in_query(const Node& node);
  bool is_constant(const Node& node);
  bool is_instance(const Node& value, const std::set<Token>& types);
  bool is_falsy(const Node& node);
  bool is_truthy(const Node& node);
  bool is_undefined(const Node& node);
  bool is_ref_to_type(const Node& var, const std::set<Token>& types);
  bool is_module(const Node& var);
  std::string strip_quotes(const std::string_view& str);
  std::string add_quotes(const std::string_view& str);
  std::string type_name(const Token& type, bool specify_number = false);
  std::string type_name(const Node& node, bool specify_number = false);

  inline bool is_quoted(const std::string_view& str)
  {
    return str.size() >= 2 && str.front() == str.back() && str.front() == '"';
  }

  using namespace trieste;
  using PrintNode = std::ostream& (*)(std::ostream&, const Node&);

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
    static Nodes resolve_query(const Node& query, BuiltIns builtins);
    static void stmt_str(logging::Log&, const Node& stmt);
    static void func_str(logging::Log&, const Node& func);
    static void arg_str(logging::Log&, const Node& arg);
    static void expr_str(logging::Log&, const Node& unifyexpr);
    static void enum_str(logging::Log&, const Node& unifyexprenum);
    static void with_str(logging::Log&, const Node& unifyexprwith);
    static void compr_str(logging::Log&, const Node& unifyexprcompr);
    static void ref_str(logging::Log&, const Node& ref);
    static void body_str(logging::Log&, const Node& rego);
    static void not_str(logging::Log&, const Node& rego);
    static void term_str(logging::Log&, const Node& rego);
    static void rego_str(logging::Log&, const Node& rego);
    static Node negate(const Node& value);
    static Node unary(const Node& value);
    static Node arithinfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node bininfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node boolinfix(const Node& op, const Node& lhs, const Node& rhs);
    static std::optional<Nodes> apply_access(
      const Node& container, const Node& index);
    static Node merge_sets(const Node& lhs, const Node& rhs, bool unwrapped);
    static Node merge_objects(const Node& lhs, const Node& rhs, bool unwrapped);
    static Node object(const Node& object_items, bool is_dynamic);
    static Node array(const Node& array_members);
    static Node set(const Node& set_members, bool is_dynamic);
    static Node set_intersection(const Node& lhs, const Node& rhs);
    static Node set_union(const Node& lhs, const Node& rhs);
    static Node set_difference(const Node& lhs, const Node& rhs);
    static Nodes resolve_varseq(const Node& varseq);
    static Nodes object_lookdown(const Node& object, const Node& query);
    static Nodes object_lookdown(
      const Node& object, const std::string_view& query);
    static Nodes module_lookdown(
      const Node& module, const std::string_view& name);
    static Node inject_args(const Node& rulefunc, const Nodes& args);
    static Node membership(
      const Node& index, const Node& item, const Node& itemseq);
    static Node membership(const Node& item, const Node& itemseq);
    static std::vector<std::string> array_find(
      const Node& array, const std::string& search);
    static std::vector<std::string> object_find(
      const Node& object, const std::string& search);
    static Node to_term(const Node& value);
    static void flatten_terms_into(const Node& termset, Node& terms);
    static void flatten_items_into(const Node& termset, Node& terms);
    static Node reduce_termset(const Node& termset);
    static void insert_into_object(
      Node& object, const std::string& path, const Node& value);
    static Nodes walk(Node x);
  };

  inline const auto NewLine = TokenDef("rego-newline");
  inline const auto Brace = TokenDef("rego-brace");
  inline const auto Dot = TokenDef("rego-dot");
  inline const auto Colon = TokenDef("rego-colon");
  inline const auto Square = TokenDef("rego-square");
  inline const auto Paren = TokenDef("rego-paren");
  inline const auto Comma = TokenDef("rego-comma");
  inline const auto EmptySet = TokenDef("rego-emptyset");

  // Utility tokens
  inline const auto Op = TokenDef("rego-op");
  inline const auto Id = TokenDef("rego-id");
  inline const auto Head = TokenDef("rego-head");
  inline const auto Tail = TokenDef("rego-tail");
  inline const auto Lhs = TokenDef("rego-lhs");
  inline const auto Rhs = TokenDef("rego-rhs");
  inline const auto LhsVars = TokenDef("rego-lhsvars");
  inline const auto RhsVars = TokenDef("rego-rhsvars");
  inline const auto RefTerm = TokenDef("rego-refterm");
  inline const auto NumTerm = TokenDef("rego-numterm");
  inline const auto AssignArg = TokenDef("rego-assignarg");
  inline const auto Idx = TokenDef("rego-idx");
  inline const auto ArgSeq = TokenDef("rego-argseq");
  inline const auto DynamicObject = TokenDef("rego-dynamicobject");
  inline const auto DynamicSet = TokenDef("rego-dynamicset");

  inline const auto wf_json = JSONString | Int | Float | True | False | Null;
  inline const auto wf_parse_tokens = Query | Module | wf_json | wf_arith_op |
    wf_bool_op | wf_bin_op | Package | Var | Brace | Square | Dot | Paren |
    Assign | Unify | EmptySet | Colon | RawString | Default | Some | Import |
    Else | As | With | NewLine | Comma;

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

  inline const auto Input = TokenDef("rego-input", flag::lookup);
  inline const auto DataModule = TokenDef("rego-datamodule", flag::lookup);
  inline const auto DataTerm = TokenDef("rego-dataterm");
  inline const auto DataObject = TokenDef("rego-dataobject");
  inline const auto DataObjectItem = TokenDef("rego-dataobjectitem");
  inline const auto DataArray = TokenDef("rego-dataarray");
  inline const auto DataSet = TokenDef("rego-dataset");
  inline const auto DataRule = TokenDef("rego-datarule", flag::lookup);

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

  // Provide types to delay pretty printing various rego node types.
  using ArgStr = logging::Lazy<Node, Resolver::arg_str>;
  using BodyStr = logging::Lazy<Node, Resolver::body_str>;
  using ExprStr = logging::Lazy<Node, Resolver::expr_str>;
  using NotStr = logging::Lazy<Node, Resolver::not_str>;
  using RefStr = logging::Lazy<Node, Resolver::ref_str>;
  using RegoStr = logging::Lazy<Node, Resolver::rego_str>;
  using StmtStr = logging::Lazy<Node, Resolver::stmt_str>;
  using TermStr = logging::Lazy<Node, Resolver::term_str>;
  using WithStr = logging::Lazy<Node, Resolver::with_str>;

  template <typename K, typename V>
  struct MapValuesStr
  {
    const std::map<K, V>& values;
    MapValuesStr(const std::map<K, V>& values_) : values(values_) {}
  };
  // Deduction guide.
  template <typename K, typename V>
  MapValuesStr(const std::map<K, V>&) -> MapValuesStr<K, V>;

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

  template <typename T, typename I>
  inline bool contains(const std::shared_ptr<T>& container, const I& item)
  {
    return container->contains(item);
  }

  template <typename T, typename I>
  inline bool contains(const T& container, const I& item)
  {
    return container.contains(item);
  }

  template <typename S, typename P>
  inline bool starts_with(const S& str, const P& prefix)
  {
    return str.starts_with(prefix);
  }

  template <typename S, typename P>
  inline bool ends_with(const S& s, const P& suffix)
  {
    return s.ends_with(suffix);
  }
}

#ifdef REGOCPP_ACTION_METRICS
#define ACTION() rego::ActionMetrics __action_metrics(__FILE__, __LINE__)
#define PRINT_ACTION_METRICS() rego::ActionMetrics::print()
#else
#define ACTION()
#define PRINT_ACTION_METRICS()
#endif