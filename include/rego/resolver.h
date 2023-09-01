#pragma once

#include "bigint.h"
#include "builtins.h"

#include <optional>
#include <string>

namespace rego
{
  using namespace trieste;
  using PrintNode = std::ostream& (*)(std::ostream&, const Node&);

  class Resolver
  {
  public:
    struct NodePrinter
    {
      Node node;
      PrintNode printer;
      std::string str() const;
    };

    static BigInt get_int(const Node& node);

    static Node scalar(BigInt value);
    static Node scalar(double value);
    static Node scalar(bool value);
    static Node scalar(const std::string& value);
    static Node scalar();
    static double get_double(const Node& node);
    static std::string get_string(const Node& node);
    static bool get_bool(const Node& node);
    static Node resolve_query(const Node& query, const BuiltIns& builtins);
    static NodePrinter stmt_str(const Node& stmt);
    static NodePrinter func_str(const Node& func);
    static NodePrinter arg_str(const Node& arg);
    static NodePrinter expr_str(const Node& unifyexpr);
    static NodePrinter enum_str(const Node& unifyexprenum);
    static NodePrinter with_str(const Node& unifyexprwith);
    static NodePrinter compr_str(const Node& unifyexprcompr);
    static NodePrinter ref_str(const Node& ref);
    static NodePrinter body_str(const Node& rego);
    static NodePrinter not_str(const Node& rego);
    static NodePrinter term_str(const Node& rego);
    static NodePrinter rego_str(const Node& rego);
    static Node negate(const Node& value);
    static Node unary(const Node& value);
    static Node arithinfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node bininfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node boolinfix(const Node& op, const Node& lhs, const Node& rhs);
    static std::optional<Nodes> apply_access(
      const Node& container, const Node& index);
    static Node object(const Node& object_items, bool is_rule);
    static Node array(const Node& array_members);
    static Node set(const Node& set_members);
    static Node set_intersection(const Node& lhs, const Node& rhs);
    static Node set_union(const Node& lhs, const Node& rhs);
    static Node set_difference(const Node& lhs, const Node& rhs);
    static Nodes resolve_varseq(const Node& varseq);
    static Node unwrap(
      const Node& term,
      const Token& type,
      const std::string& error_prefix,
      const std::string& error_code,
      bool specify_number = false);
    static std::optional<Node> maybe_unwrap(
      const Node& term, const std::set<Token>& types);
    static std::optional<Node> maybe_unwrap_number(const Node& term);
    static std::optional<Node> maybe_unwrap_int(const Node& term);
    static std::optional<Node> maybe_unwrap_string(const Node& term);
    static std::optional<Node> maybe_unwrap_bool(const Node& term);
    static std::optional<Node> maybe_unwrap_array(const Node& term);
    static std::optional<Node> maybe_unwrap_set(const Node& term);
    static bool is_falsy(const Node& node);
    static bool is_truthy(const Node& node);
    static bool is_undefined(const Node& node);
    static Nodes object_lookdown(const Node& object, const Node& query);
    static Node inject_args(const Node& rulefunc, const Nodes& args);
    static Node membership(
      const Node& index, const Node& item, const Node& itemseq);
    static Node membership(const Node& item, const Node& itemseq);
    static std::vector<std::string> array_find(
      const Node& array, const std::string& search);
    static std::vector<std::string> object_find(
      const Node& object, const std::string& search);
    static std::string type_name(
      const Token& type, bool specify_number = false);
    static std::string type_name(const Node& node, bool specify_number = false);
    static Node to_term(const Node& value);
    static void flatten_terms_into(const Node& termset, Node& terms);
    static void flatten_items_into(const Node& termset, Node& terms);
    static Node reduce_termset(const Node& termset);
    static void insert_into_object(Node& object, const std::string& path, const Node& value);
  };

  std::ostream& operator<<(
    std::ostream& os, const Resolver::NodePrinter& printer);
}