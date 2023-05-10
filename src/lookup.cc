#include "lookup.h"

#include "math.h"

namespace rego
{
  Node singleton_value(const Nodes& defs)
  {
    if (defs.size() == 0)
    {
      return Undefined;
    }
    else if (defs.size() > 1)
    {
      return err("multiple references");
    }

    auto result = defs[0];
    if (result->type() == ObjectItem || result->type() == DataModule)
    {
      return result->back();
    }
    else if (result->type() == Data || result->type() == Input)
    {
      return result;
    }
    else if (result->type() == RuleComp)
    {
      auto resolved = result->at(2);
      if (resolved->type() == RuleBodySeq)
      {
        return err("rule not resolved");
      }
      else if (resolved->type() == JSONTrue)
      {
        return result->at(1);
      }
      else
      {
        return Undefined;
      }
    }
    return err("invalid search result");
  }

  Node search(const Node& ref)
  {
    Node refArgs = ref->back();

    // std::cout << "ref: " << ref->front() << std::endl;

    Node result = singleton_value(ref->front()->lookup());
    // std::cout << "result: " << result << std::endl;

    for (auto refArg : *refArgs)
    {
      if (result->type() == Error || result->type() == Undefined)
      {
        break;
      }

      if (result->type() == Term)
      {
        result = result->front();
      }

      // std::cout << "ref-arg: " << refArg << std::endl;

      if (refArg->type() == RefArgDot)
      {
        Nodes nodes = result->lookdown(refArg->front()->location());
        result = singleton_value(nodes);
      }
      else if (refArg->type() == RefArgBrack)
      {
        Node key = refArg->front();
        if (key->type() == Term)
        {
          key = key->front();
        }
        if (key->type() == Scalar)
        {
          Node value = key->front();
          if (value->type() == JSONInt)
          {
            if (result->type() != Array)
            {
              return err(result, "Not an array");
            }
            size_t index = static_cast<size_t>(get_int(value));
            if (index < result->size())
            {
              result = result->at(index);
            }
            else
            {
              return err("Index out of range");
            }
          }
          else if (value->type() == String)
          {
            result = singleton_value(result->lookdown(value->location()));
          }
          else
          {
            return err("Invalid index");
          }
        }
        else
        {
          return err("Unsupported ref-brack type");
        }
      }
      else
      {
        return err(refArg, "Unsupported ref arg");
      }

      // std::cout << "result: " << result << std::endl;
    }

    if (result->type() == Undefined)
    {
      result = Term << Undefined;
    }

    return result;
  }

  bool any_refs(const Node& n)
  {
    if (n->type() == Scalar)
    {
      return false;
    }

    if (n->type() == Ref || n->type() == Var)
    {
      return true;
    }

    if (n->size() == 0)
    {
      return false;
    }

    for (auto& child : *n)
    {
      if (any_refs(child))
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
    if (node->type() == Var)
    {
      auto defs = node->lookup();
      if (defs.size() == 0)
      {
        return true;
      }

      auto rule = defs.front();
      value = rule->at(1);
    }
    else if (node->type() == Ref)
    {
      value = search(node);
    }
    else
    {
      return false;
    }

    if (value->type() == Expr || value->type() == Error)
    {
      return false;
    }

    if (any_refs(value))
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

  int count_refs(const Node& node, const Location& name)
  {
    if (node->type() == RefTerm)
    {
      if (node->front()->location() == name)
      {
        return 1;
      }
      return 0;
    }

    if (node->size() == 0)
    {
      return 0;
    }

    int sum = 0;
    for (auto child : *node)
    {
      sum += count_refs(child, name);
    }
    return sum;
  }

  bool can_remove(const NodeRange& n)
  {
    Node rule = *n.first;
    if (rule->parent()->type() != RuleBody)
    {
      return false;
    }

    auto root = rule->parent()->parent()->parent()->shared_from_this();
    int ref_count = count_refs(root, rule->front()->location());

    return ref_count == 0;
  }
}