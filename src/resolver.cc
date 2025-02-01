#include "rego.hh"
#include "unify.hh"

#include <algorithm>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  bool contains_multiple_outputs(Node term)
  {
    if (term->type() == TermSet)
    {
      return true;
    }

    for (auto& child : *term)
    {
      if (contains_multiple_outputs(child))
      {
        return true;
      }
    }

    return false;
  }

  Node do_arith(const Node& op, BigInt lhs, BigInt rhs)
  {
    BigInt value;
    if (op->type() == Add)
    {
      value = lhs + rhs;
    }
    else if (op->type() == Subtract)
    {
      value = lhs - rhs;
    }
    else if (op->type() == Multiply)
    {
      value = lhs * rhs;
    }
    else if (op->type() == Divide)
    {
      return err(op, "unsupported math operation");
      /*
      In case we need integer division in the future.
      if (rhs.is_zero())
      {
        return err(op, "divide by zero", EvalBuiltInError);
      }

      value = lhs / rhs;
      */
    }
    else if (op->type() == Modulo)
    {
      if (rhs.is_zero())
      {
        return err(op, "modulo by zero", EvalBuiltInError);
      }

      value = lhs % rhs;
    }
    else
    {
      return err(op, "unsupported math operation");
    }

    return Int ^ value.loc();
  }

  Node do_arith(const Node& op, double lhs, double rhs)
  {
    double value;
    if (op->type() == Add)
    {
      value = lhs + rhs;
    }
    else if (op->type() == Subtract)
    {
      value = lhs - rhs;
    }
    else if (op->type() == Multiply)
    {
      value = lhs * rhs;
    }
    else if (op->type() == Divide)
    {
      if (rhs == 0.0)
      {
        return err(op, "divide by zero", EvalBuiltInError);
      }

      value = lhs / rhs;
    }
    else if (op->type() == Modulo)
    {
      return err(op, "modulo on floating-point number", EvalBuiltInError);
    }
    else
    {
      return err(op, "unsupported math operation");
    }

    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::max_digits10 - 1)
        << std::noshowpoint << value;
    return Float ^ oss.str();
  }

  Node do_bool(const Node& op, BigInt lhs, BigInt rhs)
  {
    bool value;
    if (op->type() == Equals)
    {
      value = lhs == rhs;
    }
    else if (op->type() == NotEquals)
    {
      value = lhs != rhs;
    }
    else if (op->type() == LessThan)
    {
      value = lhs < rhs;
    }
    else if (op->type() == LessThanOrEquals)
    {
      value = lhs <= rhs;
    }
    else if (op->type() == GreaterThan)
    {
      value = lhs > rhs;
    }
    else if (op->type() == GreaterThanOrEquals)
    {
      value = lhs >= rhs;
    }
    else
    {
      return err(op, "unsupported comparison");
    }

    if (value)
    {
      return True ^ "true";
    }
    else
    {
      return False ^ "false";
    }
  }

  Node do_bool(const Node& op, double lhs, double rhs)
  {
    bool value;
    if (op->type() == Equals)
    {
      value = lhs == rhs;
    }
    else if (op->type() == NotEquals)
    {
      value = lhs != rhs;
    }
    else if (op->type() == LessThan)
    {
      value = lhs < rhs;
    }
    else if (op->type() == LessThanOrEquals)
    {
      value = lhs <= rhs;
    }
    else if (op->type() == GreaterThan)
    {
      value = lhs > rhs;
    }
    else if (op->type() == GreaterThanOrEquals)
    {
      value = lhs >= rhs;
    }
    else
    {
      return err(op, "unsupported comparison");
    }

    if (value)
    {
      return True ^ "true";
    }
    else
    {
      return False ^ "false";
    }
  }

  Node do_bool(const Node& op, const std::string& lhs, const std::string& rhs)
  {
    bool value;
    if (op->type() == Equals)
    {
      value = lhs == rhs;
    }
    else if (op->type() == NotEquals)
    {
      value = lhs != rhs;
    }
    else if (op->type() == LessThan)
    {
      value = lhs < rhs;
    }
    else if (op->type() == LessThanOrEquals)
    {
      value = lhs <= rhs;
    }
    else if (op->type() == GreaterThan)
    {
      value = lhs > rhs;
    }
    else if (op->type() == GreaterThanOrEquals)
    {
      value = lhs >= rhs;
    }
    else
    {
      return err(op, "unsupported comparison");
    }

    if (value)
    {
      return True ^ "true";
    }
    else
    {
      return False ^ "false";
    }
  }
}

namespace rego
{
  Node Resolver::scalar(BigInt value)
  {
    return Int ^ value.loc();
  }

  Node Resolver::scalar(double value)
  {
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::max_digits10 - 1)
        << std::noshowpoint << value;
    return Float ^ oss.str();
  }

  Node Resolver::scalar()
  {
    return Null ^ "null";
  }

  Node Resolver::term(BigInt value)
  {
    return Term << (Scalar << scalar(value));
  }

  Node Resolver::term(double value)
  {
    return Term << (Scalar << scalar(value));
  }

  Node Resolver::term(bool value)
  {
    return Term << (Scalar << scalar(value));
  }

  Node Resolver::term(const char* value)
  {
    return Term << (Scalar << scalar(value));
  }

  Node Resolver::term(const std::string& value)
  {
    return Term << (Scalar << scalar(value));
  }

  Node Resolver::term()
  {
    return Term << (Scalar << scalar());
  }

  Node Resolver::scalar(const char* value)
  {
    return scalar(std::string(value));
  }

  Node Resolver::scalar(const std::string& value)
  {
    return JSONString ^ ("\"" + value + "\"");
  }

  Node Resolver::scalar(bool value)
  {
    if (value)
    {
      return True ^ "true";
    }
    else
    {
      return False ^ "false";
    }
  }

  Node Resolver::negate(const Node& node)
  {
    if (node->type() == Int)
    {
      BigInt value = get_int(node);
      return Int ^ value.negate().loc();
    }
    else if (node->type() == Float)
    {
      double value = get_double(node);
      value *= -1.0;
      return Float ^ std::to_string(value);
    }
    else
    {
      return err(node, "Invalid argument for negation");
    }
  }

  Node Resolver::arithinfix(const Node& op, const Node& lhs, const Node& rhs)
  {
    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return False ^ "false";
    }

    if (lhs->type() == Error)
    {
      return lhs;
    }

    if (rhs->type() == Error)
    {
      return rhs;
    }

    auto maybe_lhs_number = unwrap(lhs, {Int, Float});
    auto maybe_rhs_number = unwrap(rhs, {Int, Float});

    if (maybe_lhs_number.success && maybe_rhs_number.success)
    {
      Node lhs_number = maybe_lhs_number.node;
      Node rhs_number = maybe_rhs_number.node;
      auto lhs_int = try_get_int(lhs_number);
      auto rhs_int = try_get_int(rhs_number);
      if (lhs_int.has_value() && rhs_int.has_value() && op->type() != Divide)
      {
        return do_arith(op, *lhs_int, *rhs_int);
      }
      else
      {
        return do_arith(op, get_double(lhs_number), get_double(rhs_number));
      }
    }
    else
    {
      auto maybe_lhs_set = unwrap(lhs, {Set, DynamicSet});
      auto maybe_rhs_set = unwrap(rhs, {Set, DynamicSet});
      if (maybe_lhs_set.success && maybe_rhs_set.success)
      {
        return bininfix(op, maybe_lhs_set.node, maybe_rhs_set.node);
      }

      if (maybe_lhs_number.success && maybe_rhs_set.success)
      {
        return err(rhs, "operand 2 must be number but got set", EvalTypeError);
      }

      if (maybe_lhs_set.success && maybe_rhs_number.success)
      {
        return err(rhs, "operand 2 must be set but got number", EvalTypeError);
      }

      return Undefined;
    }
  }

  Node Resolver::bininfix(const Node& op, const Node& lhs, const Node& rhs)
  {
    auto maybe_lhs_set = unwrap(lhs, {Set, DynamicSet});
    auto maybe_rhs_set = unwrap(rhs, {Set, DynamicSet});

    if (maybe_lhs_set.success && maybe_rhs_set.success)
    {
      if (op->type() == And)
      {
        return set_intersection(maybe_lhs_set.node, maybe_rhs_set.node);
      }
      if (op->type() == Or)
      {
        return set_union(maybe_lhs_set.node, maybe_rhs_set.node);
      }
      if (op->type() == Subtract)
      {
        return set_difference(maybe_lhs_set.node, maybe_rhs_set.node);
      }

      return err(op, "Unsupported binary operator");
    }
    else
    {
      return Undefined;
    }
  }

  Node Resolver::boolinfix(const Node& op, const Node& lhs, const Node& rhs)
  {
    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return False ^ "false";
    }

    if (lhs->type() == Error)
    {
      return lhs;
    }

    if (rhs->type() == Error)
    {
      return rhs;
    }

    auto maybe_lhs_number = unwrap(lhs, {Int, Float});
    auto maybe_rhs_number = unwrap(rhs, {Int, Float});

    if (maybe_lhs_number.success && maybe_rhs_number.success)
    {
      Node lhs_number = maybe_lhs_number.node;
      Node rhs_number = maybe_rhs_number.node;
      if (lhs_number->type() == Int && rhs_number->type() == Int)
      {
        return do_bool(op, get_int(lhs_number), get_int(rhs_number));
      }
      else
      {
        return do_bool(op, get_double(lhs_number), get_double(rhs_number));
      }
    }
    else
    {
      return do_bool(op, to_key(lhs), to_key(rhs));
    }
  }

  std::optional<Nodes> Resolver::apply_access(
    const Node& container, const Node& arg)
  {
    if (is_undefined(container))
    {
      return std::nullopt;
    }

    if (container->type() == Array)
    {
      Node index = arg;
      if (index->type() == Term)
      {
        index = index->front();
      }

      if (index->type() == Scalar)
      {
        index = index->front();
      }

      auto maybe_i = try_get_int(index);
      if (maybe_i.has_value())
      {
        auto i = maybe_i->to_size();
        if (i < container->size())
        {
          Node value = container->at(i);
          if (value->type() == Expr)
          {
            throw std::runtime_error("Not implemented");
          }

          return Nodes({value});
        }
      }
    }

    if (container == Object || container == DynamicObject)
    {
      Node query = arg;
      if (query == Term)
      {
        query = arg->front();
      }
      return object_lookdown(container, query);
    }

    if (container->type() == Data || container->type() == Submodule)
    {
      Node key = arg->front();
      std::string key_str = std::string((container / Key)->location().view());
      key_str += "." + strip_quotes(to_key(key));
      return module_lookdown(container, key_str);
    }

    if (container == Set || container == DynamicSet)
    {
      std::set<std::string> reprs;
      std::string query_repr = to_key(arg);
      for (Node member : *container)
      {
        std::string repr = to_key(member);
        if (repr == query_repr)
        {
          return Nodes({to_term(arg)});
        }
      }

      return std::nullopt;
    }

    return std::nullopt;
  }

  Node Resolver::to_term(const Node& value)
  {
    if (value == Term || value == TermSet)
    {
      return value->clone();
    }

    if (
      value == Array || value == DynamicSet || value == Set ||
      value == DynamicObject || value == Object || value == Scalar)
    {
      return Term << value->clone();
    }

    if (
      value->type() == Int || value->type() == Float ||
      value->type() == JSONString || value->type() == True ||
      value->type() == False || value->type() == Null)
    {
      return Term << (Scalar << value->clone());
    }

    return err(value, "Not a term");
  }

  Node Resolver::merge_sets(const Node& lhs, const Node& rhs, bool unwrapped)
  {
    logging::Trace() << "merge_sets" << lhs << rhs << std::endl;

    Node lhs_set = lhs;
    Node rhs_set = rhs;

    if (!unwrapped)
    {
      auto maybe_lhs = unwrap(lhs, DynamicSet);
      auto maybe_rhs = unwrap(rhs, DynamicSet);

      if (!maybe_lhs.success && !maybe_rhs.success)
      {
        auto lhs_key = to_key(lhs);
        auto rhs_key = to_key(rhs);
        if (lhs_key == rhs_key)
        {
          return lhs;
        }
        else
        {
          return err(lhs, "object keys must be unique", EvalConflictError);
        }
      }

      if (!maybe_lhs.success)
      {
        return err(lhs, "conflicting values for rule", EvalConflictError);
      }

      if (!maybe_rhs.success)
      {
        return err(rhs, "conflicting values for rule", EvalConflictError);
      }

      lhs_set = maybe_lhs.node;
      rhs_set = maybe_rhs.node;
    }

    Node set = NodeDef::create(DynamicSet);
    std::set<std::string> items;
    for (auto& item : *lhs_set)
    {
      items.insert(to_key(item));
      set << item;
    }

    for (auto& item : *rhs_set)
    {
      if (!contains(items, to_key(item)))
      {
        set << item;
      }
    }

    return Term << set;
  }

  Node Resolver::merge_objects(const Node& lhs, const Node& rhs, bool unwrapped)
  {
    logging::Trace() << "merge_objects" << lhs << rhs << std::endl;

    Node lhs_obj = lhs;
    Node rhs_obj = rhs;
    if (!unwrapped)
    {
      auto maybe_lhs = unwrap(lhs, {DynamicObject, DynamicSet});
      auto maybe_rhs = unwrap(rhs, {DynamicObject, DynamicSet});

      if (!maybe_lhs.success && !maybe_rhs.success)
      {
        auto lhs_key = to_key(lhs);
        auto rhs_key = to_key(rhs);
        if (lhs_key == rhs_key)
        {
          return lhs;
        }
        else
        {
          return err(lhs, "object keys must be unique", EvalConflictError);
        }
      }

      if (!maybe_lhs.success)
      {
        return err(lhs, "conflicting values for rule", EvalConflictError);
      }

      if (!maybe_rhs.success)
      {
        return err(rhs, "conflicting values for rule", EvalConflictError);
      }

      lhs_obj = maybe_lhs.node;
      rhs_obj = maybe_rhs.node;

      if (lhs_obj == DynamicSet && rhs_obj == DynamicSet)
      {
        return merge_sets(lhs_obj, rhs_obj, true);
      }

      if (lhs_obj != DynamicObject || rhs_obj != DynamicObject)
      {
        return err(rhs, "conflicting values for rule", EvalConflictError);
      }
    }

    std::map<std::string, Node> items;
    for (auto& item : *lhs_obj)
    {
      items[to_key(item / Key)] = item;
    }

    for (auto& item : *rhs_obj)
    {
      std::string key = to_key(item / Key);
      if (contains(items, key))
      {
        Node merged = merge_objects(items[key] / Val, item / Val, false);
        if (merged == Error)
        {
          return merged;
        }
        items[key] = ObjectItem << (item / Key) << merged;
      }
      else
      {
        items[key] = item;
      }
    }

    auto object = NodeDef::create(DynamicObject);
    for (auto& [_, item] : items)
    {
      object->push_back(item);
    }

    return Term << object;
  }

  Node Resolver::object(const Node& object_items, bool is_dynamic)
  {
    Node object = NodeDef::create(is_dynamic ? DynamicObject : Object);
    std::map<std::string, Node> items;
    for (std::size_t i = 0; i < object_items->size(); i += 2)
    {
      const Node& key = object_items->at(i);
      const Node& val = object_items->at(i + 1);
      if (key == Error)
      {
        return key;
      }

      if (val == Error)
      {
        return val;
      }

      std::string key_str = to_key(key);
      if (contains(items, key_str))
      {
        Node old_val = items[key_str] / Val;
        auto maybe_lhs = unwrap(old_val, {DynamicObject, DynamicSet});
        auto maybe_rhs = unwrap(val, {DynamicObject, DynamicSet});

        if (!maybe_lhs.success && !maybe_rhs.success)
        {
          auto lhs_str = to_key(old_val);
          auto rhs_str = to_key(val);
          if (lhs_str != rhs_str)
          {
            if (is_dynamic)
            {
              return err(
                val,
                "complete rules must not produce multiple outputs",
                EvalConflictError);
            }
            else
            {
              return err(val, "object keys must be unique", EvalConflictError);
            }
          }
          else
          {
            continue;
          }
        }

        if (!maybe_lhs.success)
        {
          return err(old_val, "conflicting values for rule", EvalConflictError);
        }

        if (!maybe_rhs.success)
        {
          return err(val, "conflicting values for rule", EvalConflictError);
        }

        Node lhs = maybe_lhs.node;
        Node rhs = maybe_rhs.node;

        if (lhs == DynamicObject)
        {
          if (rhs == DynamicObject)
          {
            items[key_str] = ObjectItem << key << merge_objects(lhs, rhs, true);
          }
          else
          {
            return err(val, "conflicting values for rule", EvalConflictError);
          }
        }
        else if (lhs == DynamicSet)
        {
          if (rhs == DynamicSet)
          {
            items[key_str] = ObjectItem << key << merge_sets(lhs, rhs, true);
          }
          else
          {
            return err(val, "conflicting values for rule", EvalConflictError);
          }
        }
        else
        {
          return err(old_val, "conflicting values for rule", EvalConflictError);
        }
      }
      else
      {
        items[key_str] = ObjectItem << key << val;
      }
    }

    // objects need to be created with sorted keys
    for (auto [_, item] : items)
    {
      object->push_back(item);
    }

    return object;
  }

  Node Resolver::array(const Node& array_members)
  {
    Node array = NodeDef::create(Array);
    for (Node member : *array_members)
    {
      array->push_back(to_term(member));
    }
    return array;
  }

  Node Resolver::unary(const Node& value)
  {
    auto maybe_number = unwrap(value, {Int, Float});
    if (maybe_number.success)
    {
      return negate(maybe_number.node);
    }
    else
    {
      return err(value, "unsupported negation");
    }
  }

  Node Resolver::set(const Node& set_members, bool is_dynamic)
  {
    std::map<std::string, Node> members;
    for (Node member : *set_members)
    {
      if (member->type() == Expr)
      {
        throw std::runtime_error("Not implemented");
      }

      std::string repr = to_key(member);
      if (!contains(members, repr))
      {
        members[repr] = to_term(member);
      }
    }

    Node set = NodeDef::create(is_dynamic ? DynamicSet : Set);
    for (auto [_, member] : members)
    {
      set->push_back(member);
    }
    return set;
  }

  Node Resolver::set_intersection(const Node& lhs, const Node& rhs)
  {
    if (!(lhs == Set || lhs == DynamicSet))
    {
      return err(lhs, "intersection: both arguments must be sets");
    }

    if (!(rhs == Set || rhs == DynamicSet))
    {
      return err(rhs, "intersection: both arguments must be sets");
    }

    Node set = NodeDef::create(
      lhs == DynamicSet || rhs == DynamicSet ? DynamicSet : Set);

    std::set<std::string> values;
    for (auto term : *lhs)
    {
      values.insert(to_key(term));
    }

    for (auto term : *rhs)
    {
      if (contains(values, to_key(term)))
      {
        set->push_back(term->clone());
      }
    }

    return set;
  }

  Node Resolver::set_union(const Node& lhs, const Node& rhs)
  {
    if (!(lhs == Set || lhs == DynamicSet))
    {
      return err(lhs, "union: both arguments must be sets");
    }

    if (!(rhs == Set || rhs == DynamicSet))
    {
      return err(rhs, "union: both arguments must be sets");
    }

    Node set = NodeDef::create(
      lhs == DynamicSet || rhs == DynamicSet ? DynamicSet : Set);

    std::map<std::string, Node> members;
    for (auto term : *lhs)
    {
      members[to_key(term)] = term;
    }

    for (auto term : *rhs)
    {
      std::string key = to_key(term);
      if (!contains(members, key))
      {
        members[key] = term;
      }
    }

    for (auto [_, member] : members)
    {
      set->push_back(member->clone());
    }

    return set;
  }

  Node Resolver::set_difference(const Node& lhs, const Node& rhs)
  {
    if (!(lhs == Set || lhs == DynamicSet))
    {
      return err(lhs, "difference: both arguments must be sets");
    }

    if (!(rhs == Set || rhs == DynamicSet))
    {
      return err(rhs, "difference: both arguments must be sets");
    }

    Node set = NodeDef::create(
      lhs == DynamicSet || rhs == DynamicSet ? DynamicSet : Set);
    std::set<std::string> values;
    for (auto term : *rhs)
    {
      values.insert(to_key(term));
    }

    for (auto term : *lhs)
    {
      if (!contains(values, to_key(term)))
      {
        set->push_back(term->clone());
      }
    }

    return set;
  }

  void Resolver::stmt_str(logging::Log& log, const Node& statement)
  {
    if (statement->type() == UnifyExprEnum)
    {
      enum_str(log, statement);
    }
    else if (statement->type() == UnifyExprWith)
    {
      with_str(log, statement);
    }
    else if (statement->type() == UnifyExprCompr)
    {
      compr_str(log, statement);
    }
    else if (statement->type() == UnifyExprNot)
    {
      not_str(log, statement);
    }
    else
    {
      expr_str(log, statement);
    }
  }

  void Resolver::expr_str(logging::Log& os, const Node& unifyexpr)
  {
    Node lhs = unifyexpr / Var;
    Node rhs = unifyexpr / Val;
    os << lhs->location().view() << " = ";
    if (rhs->type() == Function)
      func_str(os, rhs);
    else
      arg_str(os, rhs);
  }

  void Resolver::term_str(logging::Log& log, const Node& node)
  {
    log << node->type().str() << "(" << to_key(node) << ")";
  }

  void Resolver::with_str(logging::Log& os, const Node& unifyexprwith)
  {
    Node unifybody = unifyexprwith / UnifyBody;
    os << "{";
    logging::Sep sep{"; "};
    for (Node expr : *unifybody)
    {
      if (expr->type() == UnifyExpr)
      {
        os << sep << ExprStr(expr);
      }

      if (expr->type() == UnifyExprNot)
      {
        os << sep << NotStr(expr);
      }
    }
    os << "} ";

    Node withseq = unifyexprwith / WithSeq;
    logging::Sep sep2{"; "};
    for (Node with : *withseq)
    {
      Node ref = with / RuleRef;
      Node var = with / Var;
      os << sep2 << "with " << RefStr(ref) << " as " << ArgStr(var);
    }
  }

  void Resolver::compr_str(logging::Log& os, const Node& unifyexprcompr)
  {
    Node lhs = unifyexprcompr / Var;
    Node rhs = unifyexprcompr / Val;
    Node unifybody = unifyexprcompr / UnifyBody;
    os << lhs->location().view() << " = " << rhs->type().str() << "{";
    logging::Sep sep{"; "};
    for (Node expr : *unifybody)
    {
      if (expr->type() != Local)
      {
        os << sep << StmtStr(expr);
      }
    }
    os << "}";
  }

  void Resolver::enum_str(logging::Log& os, const Node& unifyexprenum)
  {
    Node item = unifyexprenum / Item;
    Node itemseq = unifyexprenum / ItemSeq;
    Node unifybody = unifyexprenum / NestedBody / UnifyBody;
    os << "foreach " << item->location().view() << " in "
       << itemseq->location().view() << " unify {";
    logging::Sep sep{"; "};
    for (Node expr : *unifybody)
    {
      if (expr->type() != Local)
      {
        os << sep << StmtStr(expr);
      }
    }
    os << "}";
  }

  void Resolver::not_str(logging::Log& os, const Node& unifyexprnot)
  {
    Node unifybody = unifyexprnot->front();
    os << "not {";
    logging::Sep sep{"; "};
    for (Node expr : *unifybody)
    {
      if (expr->type() != Local)
      {
        os << sep << StmtStr(expr);
      }
    }
    os << "}";
  }

  void Resolver::func_str(logging::Log& os, const Node& function)
  {
    Node name = function / JSONString;
    Node args = function / ArgSeq;
    os << name->location().view() << "(";
    logging::Sep sep{", "};
    for (Node arg : *args)
    {
      os << sep << ArgStr(arg);
    }
    os << ")";
  }

  void Resolver::arg_str(logging::Log& os, const Node& arg)
  {
    if (arg->type() == Var)
    {
      os << arg->location().view();
    }
    else if (arg->type() == NestedBody)
    {
      os << "{";
      Node body = arg / Val;
      logging::Sep sep{"; "};
      for (Node expr : *body)
      {
        if (expr->type() == Local)
          continue;

        os << sep << StmtStr(expr);
      }
      os << "}";
    }
    else if (arg->type() == VarSeq)
    {
      os << "[";
      logging::Sep sep{", "};
      for (Node var : *arg)
      {
        os << sep << var->location().view();
      }
      os << "]";
    }
    else
    {
      os << to_key(arg);
    }
  }

  void Resolver::ref_str(logging::Log& os, const Node& ref_)
  {
    Node ref = ref_;
    if (ref->type() == RuleRef || ref->type() == RefTerm)
    {
      ref = ref->front();
      if (ref->type() == Var)
      {
        os << ref->location().view();
        return;
      }
    }

    Node refhead = ref / RefHead;
    Node refargseq = ref / RefArgSeq;
    os << refhead->front()->location().view();
    for (Node refarg : *refargseq)
    {
      if (refarg->type() == RefArgDot)
      {
        os << "." << refarg->front()->location().view();
      }
      else if (refarg->type() == RefArgBrack)
      {
        os << "[" << refarg->front()->location().view() << "]";
      }
      else
      {
        throw std::runtime_error("Not implemented");
      }
    }
  }

  Node Resolver::inject_args(const Node& rulefunc, const Nodes& args)
  {
    Node ruleargs = rulefunc / RuleArgs;
    std::size_t num_args = ruleargs->size();
    if (num_args != args.size())
    {
      std::ostringstream buf;
      buf << "function has arity " << num_args << ",  received " << args.size()
          << " arguments";
      return err(rulefunc, buf.str());
    }

    for (std::size_t i = 0; i < num_args; ++i)
    {
      Node rulearg = ruleargs->at(i);
      Node arg = args[i]->clone();
      if (rulearg->type() == ArgVal)
      {
        std::string rulearg_str = to_key(rulearg->front());
        std::string arg_str = to_key(arg);
        if (rulearg_str != arg_str)
        {
          return Undefined;
        }
      }
      else if (rulearg->type() == ArgVar)
      {
        rulearg->replace_at(1, arg);
      }
    }

    return rulefunc;
  }

  Nodes Resolver::module_lookdown(
    const Node& container, const std::string_view& query)
  {
    Node module = container;
    if (module->type() == Submodule || module->type() == Data)
    {
      module = module / Val;
    }

    if (module->type() != DataModule)
    {
      return {err(container, "Not a module")};
    }

    Nodes rules;
    for (auto& rule : *module)
    {
      if (rule->type() == RuleFunc)
      {
        Node args = rule / RuleArgs;
        if (args->size() > 0)
        {
          // no way to include this without arguments
          continue;
        }
      }

      Location name;
      if (rule->type() == Submodule)
      {
        name = (rule / Key)->location();
      }
      else
      {
        name = (rule / Var)->location();
      }

      if (name.view() == query)
      {
        rules.push_back(rule);
      }
    }

    return rules;
  }

  Nodes Resolver::object_lookdown(const Node& object, const Node& query)
  {
    return Resolver::object_lookdown(object, to_key(query));
  }

  Nodes Resolver::object_lookdown(
    const Node& object, const std::string_view& query_str)
  {
    Nodes terms;
    for (auto& object_item : *object)
    {
      Node key = object_item / Key;
      if (key->type() == Ref)
      {
        throw std::runtime_error("Not implemented.");
      }

      std::string key_str = to_key(key);
      if (key_str == query_str)
      {
        terms.push_back((object_item / Val)->clone());
      }
    }

    return terms;
  }

  Nodes Resolver::resolve_varseq(const Node& varseq)
  {
    Nodes results;
    for (Node var : *varseq)
    {
      if (results.size() == 0)
      {
        results = var->lookup();
      }
      else
      {
        Nodes new_results;
        for (Node result : results)
        {
          Nodes defs = result->lookdown(var->location());
          for (auto& def : defs)
          {
            if (def->type() == Submodule)
            {
              new_results.push_back(def / Val);
            }
            else
            {
              new_results.push_back(def);
            }
          }
        }
        results = new_results;
      }
    }

    return results;
  }

  Node Resolver::reduce_termset(const Node& termset)
  {
    Node reduce = NodeDef::create(TermSet);
    std::set<std::string> values;
    for (Node term : *termset)
    {
      std::string repr = to_key(term);
      if (!contains(values, repr))
      {
        values.insert(repr);
        reduce->push_back(term->clone());
      }
    }

    if (reduce->size() == 1)
    {
      return reduce->front();
    }

    return reduce;
  }

  Nodes Resolver::resolve_query(const Node& query, BuiltIns builtins)
  {
    Nodes defs = query->front()->lookup();
    if (defs.size() != 1)
    {
      return {err(query, "query not found")};
    }

    Node rulebody = defs[0] / Val;
    Location rulename{"query"};
    Location version = (defs[0] / Version)->location();
    UnifierCache cache = std::make_shared<std::map<UnifierKey, Unifier>>();
    auto unifier = UnifierDef::create(
      UnifierKey{rulename, UnifierType::RuleBody},
      rulename,
      version,
      rulebody,
      std::make_shared<std::vector<Location>>(),
      std::make_shared<std::vector<ValuesLookup>>(),
      builtins,
      cache);
    Node maybe_error = unifier->unify();

    // now that unification is complete, we need to empty the
    // Unifier cache. Since they all maintain a pointer to the
    // cache, it will not be able to be freed otherwise.
    cache->clear();

    if (maybe_error == Error)
    {
      return {Result << maybe_error};
    }

    Nodes results{Result << Terms << Bindings};

    for (auto& child : *rulebody)
    {
      if (child->type() == Error)
      {
        results.back() / Terms << child;
        continue;
      }

      if (child->type() != Local)
      {
        continue;
      }

      Node var = (child / Var)->clone();
      Node term = (child / Val)->clone();

      if (term->type() == TermSet)
      {
        if (term->size() == 0)
        {
          term = Undefined;
        }
        else
        {
          term = reduce_termset(term);
        }
      }

      if (is_undefined(term))
      {
        continue;
      }

      std::string name = std::string(var->location().view());

      if (term == TermSet)
      {
        Node termset = term;
        // there are multiple bindings/terms
        while (results.size() < termset->size())
        {
          results.push_back(Result << Terms << Bindings);
        }

        for (std::size_t i = 0; i < termset->size(); i++)
        {
          term = termset->at(i);
          if (contains_multiple_outputs(term))
          {
            results[i] / Terms << err(
              term,
              "complete rules must not produce multiple outputs",
              EvalConflictError);
          }
          else if (starts_with(name, "value$"))
          {
            results[i] / Terms << term;
          }
          else
          {
            results[i] / Bindings << (Binding << var->clone() << term);
          }
        }
      }
      else if (results.size() == 1)
      {
        if (contains_multiple_outputs(term))
        {
          results.back() / Terms << err(
            term,
            "complete rules must not produce multiple outputs",
            EvalConflictError);
        }
        else if (starts_with(name, "value$"))
        {
          results.back() / Terms << term;
        }
        else if (name.find('$') == std::string::npos || name[0] == '$')
        {
          results.back() / Bindings << (Binding << var << term);
        }
      }
    }

    if (
      results.size() == 1 && (results.back() / Terms)->empty() &&
      (results.back() / Bindings)->empty())
    {
      results.pop_back();
    }

    return results;
  }

  Node Resolver::membership(
    const Node& index, const Node& item, const Node& itemseq)
  {
    Node items = itemseq;
    if (items->type() == Term)
    {
      items = items->front();
    }

    std::vector<std::string> indices;
    if (items->type() == Array || items->type() == Set)
    {
      indices = array_find(items, to_key(item));
    }
    else if (items->type() == Object)
    {
      indices = object_find(items, to_key(item));
    }
    else
    {
      return False ^ "false";
    }

    std::string index_str = to_key(index);
    for (auto& i : indices)
    {
      if (i == index_str)
      {
        return True ^ "true";
      }
    }

    return False ^ "false";
  }

  Node Resolver::membership(const Node& item, const Node& itemseq)
  {
    Node items = itemseq;
    if (items->type() == Term)
    {
      items = items->front();
    }

    std::vector<std::string> indices;
    if (items->type() == Array || items->type() == Set)
    {
      indices = array_find(items, to_key(item));
    }
    else if (items->type() == Object)
    {
      indices = object_find(items, to_key(item));
    }
    else
    {
      return False ^ "false";
    }

    if (indices.size() > 0)
    {
      return True ^ "True";
    }

    return False ^ "false";
  }

  std::vector<std::string> Resolver::array_find(
    const Node& array, const std::string& search)
  {
    std::vector<std::string> indices;
    for (std::size_t i = 0; i < array->size(); ++i)
    {
      Node item = array->at(i);
      if (to_key(item) == search)
      {
        indices.push_back(std::to_string(i));
      }
    }

    return indices;
  }

  std::vector<std::string> Resolver::object_find(
    const Node& object, const std::string& search)
  {
    std::vector<std::string> indices;
    for (auto& objectitem : *object)
    {
      if (to_key(objectitem / Val) == search)
      {
        indices.push_back(to_key(objectitem / Key));
      }
    }

    return indices;
  }

  void Resolver::body_str(logging::Log& os, const Node& unifybody)
  {
    os << "{" << std::endl;
    for (auto& stmt : *unifybody)
    {
      if (stmt->type() == Local)
      {
        os << "  local " << (stmt / Var)->location().view() << std::endl;
      }
      else
      {
        os << "  ";
        stmt_str(os, stmt);
        os << std::endl;
      }
    }
    os << "}";
  }

  void Resolver::rego_str(logging::Log& os, const Node& rego)
  {
    std::deque<Node> queue;
    queue.push_back(rego / Data);
    os << std::endl << std::endl;
    while (!queue.empty())
    {
      Node current = queue.front();
      queue.pop_front();
      if (contains(RuleTypes, current->type()))
      {
        os << current->type().str() << " ";
        os << (current / Var)->location().view();
        if (current->type() == RuleFunc || current->type() == RuleComp)
        {
          os << "#" << (current / Idx)->location().view();
        }
        if (current->type() == RuleFunc)
        {
          os << "(";
          Node args = current / RuleArgs;
          logging::Sep sep{", "};
          for (auto& arg : *args)
          {
            os << sep << (arg / Var)->location().view();
          }
          os << ")";
        }
        os << std::endl;
        os << "body: ";
        Node body = current / Body;
        if (body->type() == Empty)
        {
          os << "empty" << std::endl;
        }
        else
        {
          body_str(os, body);
          os << std::endl;
        }
        os << "value: ";
        Node value = current / Val;
        if (value->type() == Term)
        {
          os << to_key(value) << std::endl;
        }
        else
        {
          body_str(os, value);
          os << std::endl;
        }
        os << std::endl;
      }
      else
      {
        for (auto& child : *current)
        {
          queue.push_back(child);
        }
      }
    }
  }

  void Resolver::flatten_terms_into(const Node& termset, Node& terms)
  {
    if (is_undefined(termset))
    {
      return;
    }

    if (termset->type() == Term)
    {
      terms->push_back(termset->front()->clone());
      return;
    }

    if (termset->type() != TermSet)
    {
      terms->push_back(err(termset, "Not a term"));
      return;
    }

    for (auto& term : *termset)
    {
      if (term->type() == TermSet)
      {
        flatten_terms_into(term, terms);
      }
      else if (term->type() == Term)
      {
        terms->push_back(term->front()->clone());
      }
      else
      {
        terms->push_back(err(term, "Not a term"));
      }
    }
  }

  void Resolver::flatten_items_into(const Node& termset, Node& items)
  {
    if (termset->type() == Term)
    {
      Node array = termset->front();
      items->push_back(array->front()->clone());
      items->push_back(array->back()->clone());
      return;
    }

    if (termset->type() != TermSet)
    {
      items->push_back(err(termset, "Not a term"));
      return;
    }

    for (auto& term : *termset)
    {
      if (term->type() == TermSet)
      {
        flatten_items_into(term, items);
      }
      else if (term->type() == Term)
      {
        Node array = term->front();
        items->push_back(array->front()->clone());
        items->push_back(array->back()->clone());
      }
      else
      {
        items->push_back(err(term, "Not a term"));
      }
    }
  }

  void Resolver::insert_into_object(
    Node& object, const std::string& path, const Node& value)
  {
    Node current = object;
    std::size_t start = 0;
    std::size_t pos = path.find('.');
    while (pos != path.npos)
    {
      std::string prefix = path.substr(start, pos - start);
      Node query = JSONString ^ prefix;

      bool found = false;
      std::string query_str = to_key(query);
      for (auto& object_item : *current)
      {
        Node key = object_item / Key;
        std::string key_str = to_key(key);
        if (key_str == query_str)
        {
          current = object_item / Val;
          found = true;
          break;
        }
      }

      if (!found)
      {
        current = NodeDef::create(Object);
        object->push_back(
          ObjectItem << (Term << (Scalar << query)) << (Term << current));
      }
      else
      {
        if (current->type() == Term)
        {
          current = current->front();
        }
      }

      start = pos + 1;
      pos = path.find('.', start);
    }

    if (!(current == Object || current == DynamicObject))
    {
      logging::Warn() << "Conflict: cannot merge partials into non-object: "
                      << TermStr(current);
      Node replacement = NodeDef::create(Object);
      current->parent()->replace(current, replacement);
      current = replacement;
    }

    Node key = (Scalar << (JSONString ^ path.substr(start)));
    std::string key_str = to_key(key);
    Node existing = nullptr;
    for (auto& object_item : *current)
    {
      Node item_key = object_item / Key;
      std::string item_key_str = to_key(item_key);
      if (key_str == item_key_str)
      {
        existing = object_item;
        break;
      }
    }

    if (existing == nullptr)
    {
      current->push_back(
        ObjectItem << (Term << key) << (Term << value->clone()));
    }
    else
    {
      current->replace(
        existing, ObjectItem << (Term << key) << (Term << value->clone()));
    }
  }

  Nodes Resolver::walk(Node x)
  {
    Nodes nodes;
    std::vector<std::pair<Nodes, Node>> stack;
    stack.push_back({{}, x});
    while (!stack.empty())
    {
      auto [path_nodes, current] = stack.back();
      stack.pop_back();

      Node path_array = NodeDef::create(Array);
      for (auto& node : path_nodes)
      {
        path_array->push_back(to_term(node->clone()));
      }

      nodes.push_back(
        Array << (Term << path_array) << to_term(current->clone()));

      auto maybe_node =
        unwrap(current, {Array, Set, DynamicSet, Object, DynamicObject});
      if (!maybe_node.success)
      {
        continue;
      }

      current = maybe_node.node;
      if (current == Array)
      {
        for (size_t i = 0; i < current->size(); i++)
        {
          Node index = term(BigInt(i));
          Nodes path(path_nodes);
          path.push_back(index);
          stack.push_back({path, current->at(i)});
        }
      }
      else if (current->in({Set, DynamicSet}))
      {
        for (auto child : *current)
        {
          Nodes path(path_nodes);
          path.push_back(child);
          stack.push_back({path, child});
        }
      }
      else if (current->in({Object, DynamicObject}))
      {
        for (auto child : *current)
        {
          Nodes path(path_nodes);
          path.push_back(child / Key);
          stack.push_back({path, child / Val});
        }
      }
    }

    return nodes;
  }
}