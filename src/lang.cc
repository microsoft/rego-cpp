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
    | (ObjectItem <<= Key * Term)
    | (Term <<= Scalar | Array | Object | Set | Undefined)
    | (Scalar <<= JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    ;
  // clang-format on
}

namespace rego
{
  std::stringstream log_stream;
  std::vector<PassCheck> passes()
  {
    return {
      {"input_data", input_data(), wf_pass_input_data},
      {"modules", modules(), wf_pass_modules},
      {"lists", lists(), wf_pass_lists},
      {"structure", structure(), wf_pass_structure},
      {"strings", strings(), wf_pass_strings},
      {"symbols", symbols(), wf_pass_symbols},
      {"locals", locals(), wf_pass_locals},
      {"multiply_divide", multiply_divide(), wf_pass_multiply_divide},
      {"add_subtract", add_subtract(), wf_pass_add_subtract},
      {"comparison", comparison(), wf_pass_comparison},
      {"assign", assign(), wf_pass_assign},
      {"refs", refs(), wf_pass_refs},
      {"rulebody", rulebody(), wf_pass_rulebody},
      {"functions", functions(), wf_pass_functions},
      {"merge_data", merge_data(), wf_pass_merge_data},
      {"merge_modules", merge_modules(), wf_pass_merge_modules},
      {"rules", rules(), wf_pass_rules},
      {"query", query(), wf_pass_query},
    };
  }

  Driver& driver()
  {
    auto passdefs = passes();
    static Driver d(
      "rego",
      parser(),
      wf_parser,
      {
        {"input_data", input_data(), wf_pass_input_data},
        {"modules", modules(), wf_pass_modules},
        {"lists", lists(), wf_pass_lists},
        {"structure", structure(), wf_pass_structure},
        {"strings", strings(), wf_pass_strings},
        {"symbols", symbols(), wf_pass_symbols},
        {"locals", locals(), wf_pass_locals},
        {"multiply_divide", multiply_divide(), wf_pass_multiply_divide},
        {"add_subtract", add_subtract(), wf_pass_add_subtract},
        {"comparison", comparison(), wf_pass_comparison},
        {"assign", assign(), wf_pass_assign},
        {"refs", refs(), wf_pass_refs},
        {"rulebody", rulebody(), wf_pass_rulebody},
        {"functions", functions(), wf_pass_functions},
        {"merge_data", merge_data(), wf_pass_merge_data},
        {"merge_modules", merge_modules(), wf_pass_merge_modules},
        {"rules", rules(), wf_pass_rules},
        {"query", query(), wf_pass_query},
      });
    return d;
  }

  std::string to_json(const Node& node)
  {
    std::stringstream buf;
    if (node->type() == JSONInt)
    {
      buf << node->location().view();
    }
    else if (node->type() == JSONFloat)
    {
      buf << node->location().view();
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
    else if (node->type() == Array)
    {
      buf << "[";
      std::string sep = "";
      for (const auto& child : *node)
      {
        buf << sep << to_json(child);
        sep = ", ";
      }
      buf << "]";
    }
    else if (node->type() == Set)
    {
      std::set<std::string> items;
      for (const auto& child : *node)
      {
        items.insert(to_json(child));
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
    else if (node->type() == Object)
    {
      std::map<std::string, std::string> items;
      for (const auto& child : *node)
      {
        auto key = child->at(wfi / ObjectItem / Key);
        auto value = child->at(wfi / ObjectItem / Term);
        items[to_json(key)] = to_json(value);
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
    else if (node->type() == Scalar || node->type() == Term)
    {
      return to_json(node->at(wfi / Scalar / Scalar, wfi / Term / Term));
    }
    else if (node->type() == Binding)
    {
      buf << node->at(wfi / Binding / Var)->location().view() << " = "
          << to_json(node->at(wfi / Binding / Term));
    }
    else if (node->type() == TermSet)
    {
      buf << "termset{";
      std::string sep = "";
      for (const auto& child : *node)
      {
        buf << sep << to_json(child);
        sep = ", ";
      }
      buf << "}";
    }
    else if (node->type() == Error)
    {
      buf << node->str();
    }
    else
    {
      buf << node->type().str() << "(" << node.get() << ")";
    }

    return buf.str();
  }

  bool contains_local(const Node& node)
  {
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

  bool is_in(const Node& node, const Token& type)
  {
    if (node->type() == type)
    {
      return true;
    }

    if (node->type() == Rego)
    {
      return false;
    }

    return is_in(node->parent()->shared_from_this(), type);
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
      os << sep << loc.view();
      sep = ", ";
    }
    os << "]";
    return os;
  }
}