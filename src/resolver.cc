#include "resolver.h"

#include "lang.h"
#include "math.h"
#include "unifier.h"
#include "wf.h"

namespace rego
{
  Node Resolver::arithinfix(const Node& op, const Node& lhs, const Node& rhs)
  {
    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return JSONFalse;
    }

    if (lhs->type() == Error)
    {
      return lhs;
    }

    if (rhs->type() == Error)
    {
      return rhs;
    }

    auto maybe_lhs_number = maybe_unwrap_number(lhs);
    auto maybe_rhs_number = maybe_unwrap_number(rhs);

    if (maybe_lhs_number.has_value() && maybe_rhs_number.has_value())
    {
      Node lhs_number = maybe_lhs_number.value();
      Node rhs_number = maybe_rhs_number.value();
      if (lhs_number->type() == JSONInt && rhs_number->type() == JSONInt)
      {
        return math(op, get_int(lhs_number), get_int(rhs_number));
      }
      else
      {
        return math(op, get_double(lhs_number), get_double(rhs_number));
      }
    }
    else
    {
      return err(
        op, "Cannot perform arithmetic operations on non-numeric values");
    }
  }

  Node Resolver::boolinfix(const Node& op, const Node& lhs, const Node& rhs)
  {
    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return JSONFalse;
    }

    if (lhs->type() == Error)
    {
      return lhs;
    }

    if (rhs->type() == Error)
    {
      return rhs;
    }

    auto maybe_lhs_number = maybe_unwrap_number(lhs);
    auto maybe_rhs_number = maybe_unwrap_number(rhs);

    if (maybe_lhs_number.has_value() && maybe_rhs_number.has_value())
    {
      Node lhs_number = maybe_lhs_number.value();
      Node rhs_number = maybe_rhs_number.value();
      if (lhs_number->type() == JSONInt && rhs_number->type() == JSONInt)
      {
        return compare(op, get_int(lhs_number), get_int(rhs_number));
      }
      else
      {
        return compare(op, get_double(lhs_number), get_double(rhs_number));
      }
    }
    else
    {
      return compare(op, to_json(lhs), to_json(rhs));
    }
  }

  std::optional<Node> Resolver::maybe_unwrap_number(const Node& node)
  {
    Node value;
    if (node->type() == Term)
    {
      value = node->front();
    }
    else
    {
      value = node;
    }

    if (value->type() == Scalar)
    {
      value = value->front();
    }

    if (value->type() == JSONInt || value->type() == JSONFloat)
    {
      return value;
    }

    return std::optional<Node>();
  }

  std::optional<Nodes> Resolver::apply_access(
    const Node& container, const Node& arg)
  {
    if (container->type() == Array)
    {
      Node index = arg;
      if (index->type() == Term)
      {
        index = index->front();
      }

      if (index->type() == Scalar)
      {
        index = index->at(wf_pass_functions / Scalar / Scalar);
      }

      if (index->type() == JSONInt)
      {
        auto i = get_int(index);
        if (i >= 0 && static_cast<std::size_t>(i) < container->size())
        {
          Node value = container->at(i);
          if (value->type() == Expr)
          {
            throw new std::runtime_error("Not implemented");
          }

          return Nodes({value});
        }
      }
    }

    if (container->type() == Object)
    {
      Node query = arg->at(wf_pass_functions / Scalar / Scalar);
      return object_lookdown(container, query);
    }

    if (
      container->type() == Input || container->type() == Data ||
      container->type() == Module)
    {
      Node key = arg->at(wf_pass_functions / Scalar / Scalar);
      std::string key_str = to_json(key);
      if (key_str.starts_with('"') && key_str.ends_with('"'))
      {
        key_str = key_str.substr(1, key_str.size() - 2);
      }
      Nodes defs = container->lookdown(key_str);
      if (
        defs[0]->type() == RuleComp || defs[0]->type() == DefaultRule ||
        defs[0]->type() == RuleFunc)
      {
        return defs;
      }

      Nodes nodes;
      for (auto& def : defs)
      {
        if (def->type() == DataModule)
        {
          nodes.push_back(def->at(wf_resolve / DataModule / Module));
        }
        else if (def->type() == ObjectItem)
        {
          nodes.push_back(def->back());
        }
        else
        {
          throw std::runtime_error("Not implemented");
        }
      }

      return nodes;
    }

    if (container->type() == Set)
    {
      std::set<std::string> reprs;
      std::string query_repr = to_json(arg);
      for (Node member : *container)
      {
        std::string repr = to_json(member);
        if (repr == query_repr)
        {
          return Nodes({Term << (Scalar << JSONTrue)});
        }
      }

      return Nodes({Term << (Scalar << JSONFalse)});
    }

    return std::nullopt;
  }

  Node Resolver::object(const Node& object_items)
  {
    Node object = NodeDef::create(Object);
    for (std::size_t i = 0; i < object_items->size(); i += 2)
    {
      std::string key_str = to_json(object_items->at(i));
      if (key_str.starts_with('"') && key_str.ends_with('"'))
      {
        key_str = key_str.substr(1, key_str.size() - 2);
      }
      object->push_back(
        ObjectItem << (Key ^ key_str) << object_items->at(i + 1));
    }

    return object;
  }

  Node Resolver::array(const Node& array_members)
  {
    Node array = NodeDef::create(Array);
    array->insert(array->end(), array_members->begin(), array_members->end());
    return array;
  }

  Node Resolver::negate(const Node& value)
  {
    auto maybe_number = maybe_unwrap_number(value);
    if (maybe_number)
    {
      return rego::negate(maybe_number.value());
    }
    else
    {
      return err(value, "unsupported negation");
    }
  }

  Node Resolver::set(const Node& set_members)
  {
    std::set<std::string> reprs;
    std::vector<Node> members;
    for (Node member : *set_members)
    {
      if (member->type() == Expr)
      {
        throw std::runtime_error("Not implemented");
      }

      std::string repr = to_json(member);
      if (reprs.find(repr) == reprs.end())
      {
        reprs.insert(repr);
        members.push_back(member);
      }
    }

    Node set = NodeDef::create(Set);
    set->insert(set->end(), members.begin(), members.end());
    return set;
  }

  std::string Resolver::expr_str(const Node& unifyexpr)
  {
    Node lhs = unifyexpr->front();
    Node rhs = unifyexpr->back();
    std::string lhs_str = std::string(lhs->location().view());
    std::string rhs_str =
      rhs->type() == Function ? func_str(rhs) : arg_str(rhs);
    return lhs_str + " = " + rhs_str;
  }

  std::string Resolver::func_str(const Node& function)
  {
    std::stringstream buf;
    Node name = function->front();
    Node args = function->back();
    buf << std::string(name->location().view()) << "(";
    std::string sep = "";
    for (const auto& child : *args)
    {
      buf << sep << arg_str(child);
      sep = ", ";
    }
    buf << ")";
    return buf.str();
  }

  std::string Resolver::arg_str(const Node& arg)
  {
    if (arg->type() == Var)
    {
      return std::string(arg->location().view());
    }

    return to_json(arg);
  }

  Node Resolver::inject_args(const Node& rulefunc, const Nodes& args)
  {
    Node ruleargs = rulefunc->at(1);
    std::size_t num_args = ruleargs->size();
    if (num_args != args.size())
    {
      std::stringstream buf;
      buf << "function has arity " << num_args << ",  received " << args.size()
          << " arguments";
      return err(rulefunc, buf.str());
    }

    for (std::size_t i = 0; i < num_args; ++i)
    {
      Node rulearg = ruleargs->at(i);
      Node arg = args[i];
      if (rulearg->type() == ArgVal)
      {
        std::string rulearg_str = to_json(rulearg->front());
        std::string arg_str = to_json(arg);
        if (rulearg_str != arg_str)
        {
          return Undefined;
        }
      }
      else if (rulearg->type() == ArgVar)
      {
        rulearg->at(1) = arg;
      }
    }

    return rulefunc;
  }

  bool Resolver::is_truthy(const Node& node)
  {
    assert(node->type() == Term || node->type() == TermSet);
    if (node->type() == TermSet)
    {
      return true;
    }

    Node value = node->front();
    if (value->type() == Scalar)
    {
      value = value->front();
      return value->type() != JSONFalse;
    }

    if (
      value->type() == Object || value->type() == Array || value->type() == Set)
    {
      return true;
    }

    return false;
  }

  bool Resolver::is_falsy(const Node& node)
  {
    if (node->type() != Term)
    {
      return false;
    }

    Node value = node->front();
    if (value->type() == Scalar)
    {
      value = value->front();
      return value->type() == JSONFalse;
    }
    else if (value->type() == Undefined)
    {
      return true;
    }

    return false;
  }

  Nodes Resolver::object_lookdown(const Node& object, const Node& query)
  {
    Nodes defs = object->lookdown(query->location());

    std::string query_str = to_json(query);
    for (auto& object_item : *object)
    {
      Node key = object_item->front();
      if (key->type() == Ref)
      {
        throw std::runtime_error("Not implemented.");
      }

      std::string key_str = to_json(key);

      if (key_str == query_str)
      {
        defs.push_back(object_item->back());
      }
    }

    return defs;
  }

  Node Resolver::resolve_query(const Node& query)
  {
    Node rulebody = query->at(wf_resolve / Query / UnifyBody);

    {
      CallStack callstack = std::make_shared<std::vector<Location>>();
      Unifier unifier(Location(), rulebody, callstack);
      unifier.unify();
    }

    query->erase(query->begin(), query->end());

    for (auto& child : *rulebody)
    {
      if (child->type() == Error)
      {
        query->push_back(child);
        continue;
      }

      if (child->type() != Local)
      {
        continue;
      }

      Node var = child->at(wf_resolve / Local / Var)->clone();
      Node term = child->back()->clone();

      if (term->type() == TermSet)
      {
        if (term->size() == 0)
        {
          term = Undefined;
        }
        else
        {
          query->push_back(err(child, "Multiple values for binding"));
        }
      }

      if (term->type() != Term)
      {
        term = Term << term;
      }

      std::string name = std::string(var->location().view());

      if (name.starts_with("value$"))
      {
        query->push_back(term);
      }
      else if (name.find('$') == std::string::npos || name[0] == '$')
      {
        // either a user-defined variable (no $) or
        // a fuzzer variable ($ followed by a number)
        query->push_back(Binding << var << term);
      }
    }

    return query;
  }

}