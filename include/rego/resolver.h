#pragma once

#include <optional>
#include <string>
#include <trieste/driver.h>

namespace rego
{
  using namespace trieste;

  class Resolver
  {
  public:
    static Node resolve_query(const Node& query);
    static std::string func_str(const Node& func);
    static std::string arg_str(const Node& arg);
    static std::string expr_str(const Node& unifyexpr);
    static Node negate(const Node& value);
    static Node unary(const Node& value);
    static Node arithinfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node boolinfix(const Node& op, const Node& lhs, const Node& rhs);
    static std::optional<Nodes> apply_access(
      const Node& container, const Node& index);
    static Node object(const Node& object_items);
    static Node array(const Node& array_members);
    static Node set(const Node& set_members);
    static std::optional<Node> maybe_unwrap_number(const Node& term);
    static bool is_falsy(const Node& node);
    static bool is_truthy(const Node& node);
    static Nodes object_lookdown(const Node& object, const Node& query);
    static Node inject_args(const Node& rulefunc, const Nodes& args);
  };
}