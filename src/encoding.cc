#include "internal.hh"
#include "trieste/json.h"

#include <sstream>
#include <string_view>

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

      key = to_key(node);
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
  std::string to_key(
    const Node& node,
    SetFormat set_format,
    bool sort_arrays,
    const char* list_delim)
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
      buf << add_quotes(node->location().view());
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
      buf << add_quotes(node->location().view());
    }
    else if (node->in({Array, DataArray}))
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
            return to_key(key.node, set_format, sort_arrays, list_delim);
          });
      }
      else
      {
        for (const auto& child : *node)
        {
          items.push_back(to_key(child, set_format, sort_arrays, list_delim));
        }
      }

      buf << "[";
      join(
        buf,
        items.begin(),
        items.end(),
        list_delim,
        [](std::ostream& stream, const std::string& item) {
          stream << item;
          return true;
        });
      buf << "]";
    }
    else if (node == Set)
    {
      std::vector<NodeKey> node_keys;
      for (const auto& child : *node)
      {
        node_keys.push_back(child);
      }

      std::sort(node_keys.begin(), node_keys.end());

      switch (set_format)
      {
        case SetFormat::Square:
          buf << "[";
          break;
        case SetFormat::Rego:
          if (node_keys.empty())
          {
            buf << "set()";
            return buf.str();
          }
          buf << "{";
          break;
        case SetFormat::Angle:
          buf << "<";
          break;
      }

      join(
        buf,
        node_keys.begin(),
        node_keys.end(),
        list_delim,
        [set_format, sort_arrays, list_delim](
          std::ostream& stream, const NodeKey& node_key) {
          stream << to_key(node_key.node, set_format, sort_arrays, list_delim);
          return true;
        });

      switch (set_format)
      {
        case SetFormat::Square:
          buf << "]";
          break;
        case SetFormat::Rego:
          buf << "}";
          break;
        case SetFormat::Angle:
          buf << ">";
          break;
      }
    }
    else if (node->in({Object, DataObject, Bindings}))
    {
      std::map<std::string, std::string> items;
      for (const auto& child : *node)
      {
        auto key = child / Key;
        auto value = child / Val;
        std::string key_str = to_key(key, set_format, sort_arrays, list_delim);
        if (!is_quoted(key_str))
        {
          key_str = add_quotes(json::escape(key_str));
        }
        items.insert(
          {key_str, to_key(value, set_format, sort_arrays, list_delim)});
      }

      buf << "{";
      join(
        buf,
        items.begin(),
        items.end(),
        ", ",
        [list_delim](std::ostream& stream, const auto& item) {
          stream << item.first << ":";
          // Use spaced separator when callers request spaced list delimiters
          // (i.e. display format like sprintf %v and template strings).
          if (list_delim[0] == ',' && list_delim[1] == ' ')
          {
            stream << " ";
          }
          stream << item.second;
          return true;
        });
      buf << "}";
    }
    else if (node->in({Scalar, Term, DataTerm}))
    {
      return to_key(node->front(), set_format, sort_arrays, list_delim);
    }
    else if (node == TemplateString)
    {
      // TemplateString should be lowered to an opblock call before reaching
      // encoding. If we get here, something went wrong in the pipeline.
      buf << "<TemplateString:unreachable>";
    }
    else if (node == Result)
    {
      Node terms = node / Terms;
      Node bindings = node / Bindings;
      buf << "{";
      if (!terms->empty())
      {
        buf << '"' << "expressions" << '"' << ":"
            << to_key(terms, set_format, sort_arrays, list_delim);
        if (!bindings->empty())
        {
          buf << ", ";
        }
      }

      if (!bindings->empty())
      {
        buf << '"' << "bindings" << '"' << ":"
            << to_key(bindings, set_format, sort_arrays, list_delim);
      }

      buf << "}";
    }
    else if (node == Terms)
    {
      buf << '[';
      join(
        buf,
        node->begin(),
        node->end(),
        ", ",
        [set_format, sort_arrays, list_delim](
          std::ostream& stream, const Node& n) {
          stream << to_key(n, set_format, sort_arrays, list_delim);
          return true;
        });
      buf << ']';
    }
    else if (node->type() == Var)
    {
      buf << node->location().view();
    }
    else if (node->type() == Error)
    {
      buf << node;
    }
    else if (node == Results)
    {
      if (node->size() == 1)
      {
        return to_key(node->front(), set_format, sort_arrays, list_delim);
      }

      buf << '[';
      join(
        buf,
        node->begin(),
        node->end(),
        ", ",
        [set_format, sort_arrays, list_delim](
          std::ostream& stream, const Node& n) {
          stream << to_key(n, set_format, sort_arrays, list_delim);
          return true;
        });
      buf << ']';
    }
    else
    {
      buf << node->type().str() << "(" << static_cast<void*>(node.get()) << ")";
    }

    return buf.str();
  }
}