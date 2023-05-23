#include "lang.h"

#include "math.h"
#include "passes.h"
#include "wf.h"

#include <charconv>

namespace rego
{
  std::vector<PassCheck> passes()
  {
    return {
      {"input_data", input_data(), wf_pass_input_data},
      {"modules", modules(), wf_pass_modules},
      {"lists", lists(), wf_pass_lists},
      {"structure", structure(), wf_pass_structure},
      {"strings", strings(), wf_pass_strings},
      {"symbols", symbols(), wf_pass_symbols},
      {"multiply_divide", multiply_divide(), wf_pass_multiply_divide},
      {"add_subtract", add_subtract(), wf_pass_add_subtract},
      {"comparison", comparison(), wf_pass_comparison},
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
        {"multiply_divide", multiply_divide(), wf_pass_multiply_divide},
        {"add_subtract", add_subtract(), wf_pass_add_subtract},
        {"comparison", comparison(), wf_pass_comparison},
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
      buf << node->location().view();
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
    else if (node->type() == Array || node->type() == Set)
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
    else if (node->type() == ObjectItem)
    {
      auto key = node->front();
      auto value = node->back();
      buf << to_json(key) << ": " << to_json(value);
    }
    else if (node->type() == Object)
    {
      buf << "{";
      std::string sep = "";
      for (const auto& child : *node)
      {
        buf << sep << to_json(child);
        sep = ", ";
      }
      buf << "}";
    }
    else if (
      node->type() == String || node->type() == Scalar || node->type() == Term)
    {
      return to_json(node->front());
    }
    else
    {
      return "unsupported node type";
    }

    return buf.str();
  }
}