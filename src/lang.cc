#include "lang.h"

#include "log.h"
#include "passes.h"
#include "wf.h"

#include <charconv>

namespace
{
  using namespace rego;

  // clang-format off
  inline const auto wfi =
      (Binding <<= Var * Term)
    | (ObjectItem <<= (Key >>= Term) * (Val >>= Term))
    | (DataItem <<= Key * (Val >>= DataTerm))
    | (Term <<= Scalar | Array | Object | Set | Undefined)
    | (Scalar <<= JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    ;
  // clang-format on
}

namespace rego
{
  bool Logger::enabled = false;
  std::string Logger::indent = "";

  std::vector<PassCheck> passes(const BuiltIns& builtins)
  {
    return {
      {"input_data", input_data(), &wf_pass_input_data},
      {"modules", modules(), &wf_pass_modules},
      {"imports", imports(), &wf_pass_imports},
      {"keywords", keywords(), &wf_pass_keywords},
      {"lists", lists(), &wf_pass_lists},
      {"ifs", ifs(), &wf_pass_ifs},
      {"elses", elses(), &wf_pass_elses},
      {"rules", rules(), &wf_pass_rules},
      {"build_calls", build_calls(), &wf_pass_build_calls},
      {"membership", membership(), &wf_pass_membership},
      {"build_refs", build_refs(), &wf_pass_build_refs},
      {"structure", structure(), &wf_pass_structure},
      {"strings", strings(), &wf_pass_strings},
      {"merge_data", merge_data(), &wf_pass_merge_data},
      {"symbols", symbols(), &wf_pass_symbols},
      {"replace_argvals", replace_argvals(), &wf_pass_replace_argvals},
      {"lift_query", lift_query(), &wf_pass_lift_query},
      {"constants", constants(), &wf_pass_constants},
      {"explicit_enums", explicit_enums(), &wf_pass_explicit_enums},
      {"body_locals", body_locals(), &wf_pass_locals},
      {"value_locals", value_locals(), &wf_pass_locals},
      {"compr", compr(), &wf_pass_compr},
      {"absolute_refs", absolute_refs(), &wf_pass_absolute_refs},
      {"merge_modules", merge_modules(), &wf_pass_merge_modules},
      {"skips", skips(), &wf_pass_skips},
      {"unary", unary(), &wf_pass_unary},
      {"multiply_divide", multiply_divide(), &wf_pass_multiply_divide},
      {"add_subtract", add_subtract(), &wf_pass_add_subtract},
      {"comparison", comparison(), &wf_pass_comparison},
      {"assign", assign(builtins), &wf_pass_assign},
      {"skip_refs", skip_refs(builtins), &wf_pass_skip_refs},
      {"simple_refs", simple_refs(), &wf_pass_simple_refs},
      {"implicit_enums", implicit_enums(), &wf_pass_implicit_enums},
      {"init", init(), &wf_pass_init},
      {"rulebody", rulebody(), &wf_pass_rulebody},
      {"lift_to_rule", lift_to_rule(), &wf_pass_lift_to_rule},
      {"functions", functions(), &wf_pass_functions},
      {"unify", unify(builtins), &wf_pass_unify},
      {"query", query(), &wf_pass_query},
    };
  }

  Driver& driver(const BuiltIns& builtins)
  {
    static Driver d(
      "rego",
      nullptr,
      parser(),
      wf_parser,
      {
        {"input_data", input_data(), wf_pass_input_data},
        {"modules", modules(), wf_pass_modules},
        {"imports", imports(), wf_pass_imports},
        {"keywords", keywords(), wf_pass_keywords},
        {"lists", lists(), wf_pass_lists},
        {"ifs", ifs(), wf_pass_ifs},
        {"elses", elses(), wf_pass_elses},
        {"rules", rules(), wf_pass_rules},
        {"build_calls", build_calls(), wf_pass_build_calls},
        {"membership", membership(), wf_pass_membership},
        {"build_refs", build_refs(), wf_pass_build_refs},
        {"structure", structure(), wf_pass_structure},
        {"strings", strings(), wf_pass_strings},
        {"merge_data", merge_data(), wf_pass_merge_data},
        {"symbols", symbols(), wf_pass_symbols},
        {"replace_argvals", replace_argvals(), wf_pass_replace_argvals},
        {"lift_query", lift_query(), wf_pass_lift_query},
        {"constants", constants(), wf_pass_constants},
        {"explicit_enums", explicit_enums(), wf_pass_explicit_enums},
        {"body_locals", body_locals(), wf_pass_locals},
        {"value_locals", value_locals(), wf_pass_locals},
        {"compr", compr(), wf_pass_compr},
        {"absolute_refs", absolute_refs(), wf_pass_absolute_refs},
        {"merge_modules", merge_modules(), wf_pass_merge_modules},
        {"skips", skips(), wf_pass_skips},
        {"unary", unary(), wf_pass_unary},
        {"multiply_divide", multiply_divide(), wf_pass_multiply_divide},
        {"add_subtract", add_subtract(), wf_pass_add_subtract},
        {"comparison", comparison(), wf_pass_comparison},
        {"assign", assign(builtins), wf_pass_assign},
        {"skip_refs", skip_refs(builtins), wf_pass_skip_refs},
        {"simple_refs", simple_refs(), wf_pass_simple_refs},
        {"implicit_enums", implicit_enums(), wf_pass_implicit_enums},
        {"init", init(), wf_pass_init},
        {"rulebody", rulebody(), wf_pass_rulebody},
        {"lift_to_rule", lift_to_rule(), wf_pass_lift_to_rule},
        {"functions", functions(), wf_pass_functions},
        {"unify", unify(builtins), wf_pass_unify},
        {"query", query(), wf_pass_query},
      });
    return d;
  }

  std::string to_json(const Node& node, bool sort)
  {
    std::ostringstream buf;
    if (node->type() == JSONInt)
    {
      buf << node->location().view();
    }
    else if (node->type() == JSONFloat)
    {
      try
      {
        double value = std::stod(std::string(node->location().view()));
        buf << std::setprecision(std::numeric_limits<double>::max_digits10 - 1)
            << std::noshowpoint << value;
      }
      catch (...)
      {
        buf << node->location().view();
      }
    }
    else if (node->type() == JSONString)
    {
      std::string str = std::string(node->location().view());
      if (!str.starts_with('"'))
      {
        buf << '"' << str << '"';
      }
      else
      {
        buf << str;
      }
    }
    else if (node->type() == JSONTrue)
    {
      buf << "true";
    }
    else if (node->type() == JSONFalse)
    {
      buf << "false";
    }
    else if (node->type() == JSONNull)
    {
      buf << "null";
    }
    else if (node->type() == Undefined)
    {
      buf << "undefined";
    }
    else if (node->type() == Key)
    {
      buf << '"' << node->location().view() << '"';
    }
    else if (node->type() == Array || node->type() == DataArray)
    {
      std::vector<std::string> items;
      for (const auto& child : *node)
      {
        items.push_back(to_json(child, sort));
      }

      if (sort)
      {
        std::sort(items.begin(), items.end());
      }
      buf << "[";
      std::string sep = "";
      for (const auto& item : items)
      {
        buf << sep << item;
        sep = ", ";
      }
      buf << "]";
    }
    else if (node->type() == Set || node->type() == DataSet)
    {
      std::set<std::string> items;
      for (const auto& child : *node)
      {
        items.insert(to_json(child, sort));
      }

      buf << "[";
      std::string sep = "";
      for (const auto& item : items)
      {
        buf << sep << item;
        sep = ", ";
      }

      buf << "]";
    }
    else if (node->type() == Object || node->type() == DataObject)
    {
      std::map<std::string, std::string> items;
      for (const auto& child : *node)
      {
        auto key = wfi / child / Key;
        auto value = wfi / child / Val;
        items[to_json(key, sort)] = to_json(value, sort);
      }

      buf << "{";
      std::string sep = "";
      for (const auto& [key, value] : items)
      {
        buf << sep << key << ": " << value;
        sep = ", ";
      }

      buf << "}";
    }
    else if (
      node->type() == Scalar || node->type() == Term ||
      node->type() == DataTerm)
    {
      return to_json(node->front(), sort);
    }
    else if (node->type() == Binding)
    {
      buf << (wfi / node / Var)->location().view() << " = "
          << to_json(wfi / node / Term, sort);
    }
    else if (node->type() == TermSet)
    {
      buf << "termset{";
      std::string sep = "";
      for (const auto& child : *node)
      {
        buf << sep << to_json(child, sort);
        sep = ", ";
      }
      buf << "}";
    }
    else if (node->type() == BuiltInHook)
    {
      buf << "builtin(" << node->location().view() << ")";
    }
    else if (node->type() == Error)
    {
      buf << node;
    }
    else
    {
      buf << node->type().str() << "(" << static_cast<void*>(node.get()) << ")";
    }

    return buf.str();
  }

  bool contains_ref(const Node& node)
  {
    if (node->type() == NestedBody)
    {
      return false;
    }

    if (node->type() == Ref || node->type() == Var)
    {
      return true;
    }

    for (auto& child : *node)
    {
      if (contains_ref(child))
      {
        return true;
      }
    }

    return false;
  }

  bool contains_local(const Node& node)
  {
    if (node->type() == NestedBody)
    {
      return false;
    }

    if (node->type() == Var)
    {
      Nodes defs = node->lookup();
      if (defs.size() == 0)
      {
        // check if it is a temporary variable
        // (which may not yet be in the symbol table)
        return node->location().view().find('$') != std::string::npos;
      }
      return defs.size() == 1 && defs[0]->type() == Local;
    }

    for (auto& child : *node)
    {
      if (contains_local(child))
      {
        return true;
      }
    }

    return false;
  }

  bool is_in(const Node& node, const std::set<Token>& types)
  {
    if (types.contains(node->type()))
    {
      return true;
    }

    if (node->type() == Rego)
    {
      return false;
    }

    return is_in(node->parent()->shared_from_this(), types);
  }

  bool is_constant(const Node& term)
  {
    if (term->type() == NumTerm)
    {
      return true;
    }

    if (term->type() == RefTerm)
    {
      return false;
    }

    if (term->type() != Term)
    {
      return false;
    }

    Node node = term->front();
    if (node->type() == Scalar)
    {
      return true;
    }

    if (node->type() == Array || node->type() == Set)
    {
      for (auto& child : *node)
      {
        if (!is_constant(child->front()))
        {
          return false;
        }
      }

      return true;
    }

    if (node->type() == Object)
    {
      for (auto& item : *node)
      {
        Node key = item / Key;
        if (!is_constant(key->front()))
        {
          return false;
        }

        Node val = item / Val;
        if (!is_constant(val->front()))
        {
          return false;
        }
      }

      return true;
    }

    return false;
  }

  std::ostream& operator<<(std::ostream& os, const std::set<Location>& locs)
  {
    std::string sep = "";
    os << "{";
    for (const auto& loc : locs)
    {
      os << sep << loc.view();
      sep = ", ";
    }
    os << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const std::vector<Location>& locs)
  {
    std::string sep = "";
    os << "[";
    for (const auto& loc : locs)
    {
      os << sep << loc.source->origin() << ":" << loc.view();
      sep = ", ";
    }
    os << "]";
    return os;
  }

  std::string strip_quotes(const std::string& str)
  {
    if (str.starts_with('"') && str.ends_with('"'))
    {
      return str.substr(1, str.size() - 2);
    }

    return str;
  }

  bool in_query(const Node& node)
  {
    if (node->type() == Rego)
    {
      return false;
    }

    if (node->type() == RuleComp)
    {
      std::string name = std::string((node / Var)->location().view());
      return name.starts_with("query$");
    }

    return in_query(node->parent()->shared_from_this());
  }

  Node err(NodeRange& r, const std::string& msg, const std::string& code)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << r) << (ErrorCode ^ code);
  }

  Node err(Node node, const std::string& msg, const std::string& code)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << node->clone())
                 << (ErrorCode ^ code);
  }
}