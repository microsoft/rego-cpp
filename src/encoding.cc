#include "internal.hh"

namespace
{
  using namespace rego;

  struct NodeKey
  {
    std::string key;
    Node node;
    NodeKey(const Node& value)
    {
      node = value;
      if (node->type() == Term)
      {
        node = node->front();
      }

      if (node->type() == Scalar)
      {
        node = node->front();
      }

      key = to_json(node, false);
    }

    bool operator<(const NodeKey& other) const
    {
      if (node->type() == other.node->type())
      {
        if (node->type() == Int)
        {
          return get_int(node) < get_int(other.node);
        }

        if (node->type() == Float)
        {
          return get_double(node) < get_double(other.node);
        }

        return key < other.key;
      }
      else
      {
        if (
          (node->type() == Float && other.node->type() == Int) ||
          (node->type() == Int && other.node->type() == Float))
        {
          return get_double(node) < get_double(other.node);
        }

        // type ordering is Null < Bool < Number < String
        if (node->type() == Null)
        {
          return true;
        }

        if (other.node->type() == Null)
        {
          return false;
        }

        if (node->type() == False)
        {
          return true;
        }

        if (other.node->type() == False)
        {
          return false;
        }

        if (node->type() == True)
        {
          return true;
        }

        if (other.node->type() == True)
        {
          return false;
        }

        if (node->type() == Int || node->type() == Float)
        {
          return true;
        }

        if (other.node->type() == Int || other.node->type() == Float)
        {
          return false;
        }

        return key < other.key;
      }
    }
  };
}

namespace rego
{
  std::string to_json(const Node& node, bool set_as_array, bool sort_arrays)
  {
    std::ostringstream buf;
    if (node->type() == Int)
    {
      buf << node->location().view();
    }
    else if (node->type() == Float)
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
    else if (node->type() == True)
    {
      buf << "true";
    }
    else if (node->type() == False)
    {
      buf << "false";
    }
    else if (node->type() == Null)
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
      if (sort_arrays)
      {
        std::vector<NodeKey> keys;
        for (const auto& child : *node)
        {
          keys.push_back(child);
        }

        std::sort(keys.begin(), keys.end());
        std::transform(
          keys.begin(), keys.end(), std::back_inserter(items), [&](auto& key) {
            return to_json(key.node, set_as_array, sort_arrays);
          });
      }
      else
      {
        for (const auto& child : *node)
        {
          items.push_back(to_json(child, set_as_array, sort_arrays));
        }
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
      std::vector<NodeKey> node_keys;
      for (const auto& child : *node)
      {
        node_keys.push_back(child);
      }

      std::sort(node_keys.begin(), node_keys.end());

      if (set_as_array)
      {
        buf << "[";
      }
      else
      {
        buf << "<";
      }
      std::string sep = "";
      for (const auto& node_key : node_keys)
      {
        std::string key_str = to_json(node_key.node, set_as_array, sort_arrays);
        buf << sep << key_str;
        sep = ", ";
      }

      if (set_as_array)
      {
        buf << "]";
      }
      else
      {
        buf << ">";
      }
    }
    else if (node->type() == Object || node->type() == DataObject)
    {
      std::map<std::string, std::string> items;
      std::vector<NodeKey> keys;
      for (const auto& child : *node)
      {
        auto key = child / Key;
        auto value = child / Val;
        std::string key_str = to_json(key, set_as_array, sort_arrays);
        if (!key_str.starts_with('"') || !key_str.ends_with('"'))
        {
          key_str = '"' + key_str + '"';
        }
        items.insert({key_str, to_json(value, set_as_array, sort_arrays)});
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
      return to_json(node->front(), set_as_array, sort_arrays);
    }
    else if (node->type() == Binding)
    {
      buf << (node / Var)->location().view() << " = "
          << to_json(node / Term, set_as_array, sort_arrays);
    }
    else if (node->type() == TermSet)
    {
      buf << "termset{";
      std::string sep = "";
      for (const auto& child : *node)
      {
        buf << sep << to_json(child, set_as_array, sort_arrays);
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