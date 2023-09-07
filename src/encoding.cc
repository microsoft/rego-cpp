#include "encoding.h"

#include "helpers.h"

namespace rego
{
  std::string to_json(const Node& node, bool sort, bool rego_set)
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
        items.push_back(to_json(child, sort, rego_set));
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
      std::vector<std::string> items;
      for (const auto& child : *node)
      {
        items.push_back(to_json(child, sort, rego_set));
      }

      if (sort)
      {
        std::sort(items.begin(), items.end());
      }

      if (rego_set)
      {
        buf << "<";
      }
      else
      {
        buf << "[";
      }
      std::string sep = "";
      for (const auto& item : items)
      {
        buf << sep << item;
        sep = ", ";
      }

      if (rego_set)
      {
        buf << ">";
      }
      else
      {
        buf << "]";
      }
    }
    else if (node->type() == Object || node->type() == DataObject)
    {
      std::map<std::string, std::string> items;
      for (const auto& child : *node)
      {
        auto key = child / Key;
        auto value = child / Val;
        std::string key_str = to_json(key, sort, rego_set);
        if (!key_str.starts_with('"') || !key_str.ends_with('"'))
        {
          key_str = '"' + key_str + '"';
        }
        items[key_str] = to_json(value, sort, rego_set);
      }

      buf << "{";
      std::string sep = "";
      for (const auto& [key, value] : items)
      {
        buf << sep << key << ":" << value;
        sep = ", ";
      }

      buf << "}";
    }
    else if (
      node->type() == Scalar || node->type() == Term ||
      node->type() == DataTerm)
    {
      return to_json(node->front(), sort, rego_set);
    }
    else if (node->type() == Binding)
    {
      buf << (node / Var)->location().view() << " = "
          << to_json(node / Term, sort, rego_set);
    }
    else if (node->type() == TermSet)
    {
      buf << "termset{";
      std::string sep = "";
      for (const auto& child : *node)
      {
        buf << sep << to_json(child, sort, rego_set);
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
    else if (RuleTypes.contains(node->type()))
    {
      buf << node->type().str() << "(" << (node / Var)->location().view() << ":"
          << static_cast<void*>(node.get()) << ")";
    }
    else
    {
      buf << node->type().str() << "(" << static_cast<void*>(node.get()) << ")";
    }

    return buf.str();
  }
}