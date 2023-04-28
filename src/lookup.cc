#include "lookup.h"

#include "math.h"

namespace rego
{
  Node singleton_value(const Nodes& defs)
  {
    if (defs.size() == 0)
    {
      return Node(Undefined);
    }
    else if (defs.size() > 1)
    {
      return err("multiple references");
    }

    auto result = defs[0];
    if (result->type() == KeyValue || result->type() == TopKeyValue)
    {
      return result->back();
    }
    else if (result->type() == Data || result->type() == Input)
    {
      return result;
    }
    else if (result->type() == Rule)
    {
      auto value = result->at(1);
      auto body = result->at(2);
      if (body->type() == RuleSeq)
      {
        return value;
      }
      else if (body->type() == Undefined)
      {
        return Node(Undefined);
      }
      else
      {
        return err("invalid rule body");
      }
    }
    return err("invalid search result");
  }

  Node search(const Node& lookup)
  {
    const Node lhs = lookup->front();
    const Node rhs = lookup->back();

    //std::cerr << "lhs: " << lhs << "rhs: " << rhs << std::endl;

    Node container;
    if (lhs->type() == Lookup)
    {
      container = search(lhs);
    }
    else
    {
      container = singleton_value(lhs->lookup());
    }

    //std::cerr << "container: " << container << std::endl;

    if (
      container->type() == Error || container->type() == Undefined ||
      container->type() == RuleValue)
    {
      return container;
    }

    Node result;
    if (rhs->type() == Index)
    {
      Node value = rhs->front();
      if (value->type() == Int)
      {
        size_t index = static_cast<size_t>(get_int(value));
        if (index < container->size())
        {
          result = container->at(index);
        }
        else
        {
          result = err("Index out of range");
        }
      }
      else if (value->type() == String)
      {
        result = singleton_value(container->lookdown(rhs->location()));
      }
      else
      {
        result = err("Invalid index");
      }
    }
    else
    {
      result = singleton_value(container->lookdown(rhs->location()));
    }

    //std::cerr << "result: " << result->type() << std::endl;

    return result;
  }

  bool any_refs(NodeDef& n)
  {
    if (n.size() == 0)
    {
      return false;
    }

    if (n.type() == Ref)
    {
      return true;
    }

    for (auto& child : n)
    {
      if (any_refs(*child))
      {
        return true;
      }
    }

    return false;
  }

  bool can_replace(const NodeRange& n)
  {
    Node node = *n.first;

    Node value;
    if (node->type() == Local)
    {
      auto defs = node->lookup();
      if (defs.size() == 0)
      {
        return false;
      }

      auto rule = defs.front();
      value = rule->at(1);
      if (value->type() == Undefined)
      {
        std::cout << node << " is undefined" << std::endl;
      }
    }
    else if (node->type() == Lookup)
    {
      value = search(node);
    }
    else
    {
      return false;
    }

    if (
      value->type() == Expression || value->type() == RuleValue ||
      value->type() == Error)
    {
      return false;
    }

    if (any_refs(*value))
    {
      return false;
    }

    return true;
  }

  bool exists(const NodeRange& n)
  {
    Node node = *n.first;
    auto defs = node->lookup();
    return defs.size() > 0;
  }
}