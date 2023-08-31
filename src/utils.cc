#include "utils.h"

#include "errors.h"

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
      std::set<std::string> items;
      for (const auto& child : *node)
      {
        items.insert(to_json(child, sort, rego_set));
      }

      if (rego_set)
      {
        buf << "{";
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
        buf << "}";
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

    Node node = term;
    if (node->type() == Term)
    {
      node = node->front();
    }

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

  std::string strip_quotes(const std::string_view& str)
  {
    if (str.starts_with('"') && str.ends_with('"'))
    {
      return std::string(str.substr(1, str.size() - 2));
    }

    return std::string(str);
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
      return name.find("query$") != std::string::npos;
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

  bool all_alnum(const std::string_view& str)
  {
    for (char c : str)
    {
      if (!std::isalnum(c))
      {
        return false;
      }
    }
    return true;
  }

  Node concat_refs(const Node& lhs, const Node& rhs)
  {
    Node ref;
    if (lhs->type() == Var)
    {
      ref = Ref << (RefHead << lhs->clone()) << RefArgSeq;
    }
    else if (lhs->type() == Ref)
    {
      ref = lhs->clone();
    }
    else
    {
      return err(lhs, "invalid reference");
    }

    Node refhead = (rhs / RefHead)->front();
    Node refargseq = rhs / RefArgSeq;
    if (refhead->type() != Var)
    {
      return err(rhs, "cannot concatenate non-var refhead refs");
    }

    (ref / RefArgSeq) << (RefArgDot << refhead->clone());
    for (auto& arg : *refargseq)
    {
      (ref / RefArgSeq) << arg->clone();
    }

    return ref;
  }

  std::string flatten_ref(const Node& ref)
  {
    std::ostringstream buf;
    buf << (ref / RefHead)->front()->location().view();
    for (auto& arg : *(ref / RefArgSeq))
    {
      if (arg->type() == RefArgDot)
      {
        buf << "." << arg->front()->location().view();
      }
      else
      {
        Node index = arg->front();
        if (index->type() == Scalar)
        {
          index = index->front();
        }

        Location key = index->location();
        if (index->type() == JSONString)
        {
          key.pos += 1;
          key.len -= 2;
        }

        if (all_alnum(key.view()))
        {
          buf << "." << key.view();
        }
        else
        {
          buf << "[" << index->location().view() << "]";
        }
      }
    }
    return buf.str();
  }
}