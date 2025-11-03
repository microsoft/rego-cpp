#include "internal.hh"
#include "rego.hh"

namespace
{
  using namespace rego;
  using namespace wf::ops;

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
    std::string str = oss.str();
    if (str.find('.') == std::string::npos)
    {
      return Int ^ str;
    }

    return Float ^ str;
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
      auto maybe_lhs_set = unwrap(lhs, Set);
      auto maybe_rhs_set = unwrap(rhs, Set);
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
    auto maybe_lhs_set = unwrap(lhs, Set);
    auto maybe_rhs_set = unwrap(rhs, Set);

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

  Node Resolver::to_term(const Node& value)
  {
    if (value == Term)
    {
      return value->clone();
    }

    if (value->in({Array, Set, Object, Scalar}))
    {
      return Term << value->clone();
    }

    if (value->in({Int, Float, JSONString, True, False, Null}))
    {
      return Term << (Scalar << value->clone());
    }

    return err(value, "Not a term");
  }

  Node Resolver::object(const Node& object_items)
  {
    Node object = NodeDef::create(Object);
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
      if (items.contains(key_str))
      {
        Node old_val = items[key_str] / Val;
        auto lhs_str = to_key(old_val);
        auto rhs_str = to_key(val);
        if (lhs_str != rhs_str)
        {
          return err(val, "object keys must be unique", EvalConflictError);
        }
        else
        {
          continue;
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

  Node Resolver::array(const Nodes& array_members)
  {
    Node array = NodeDef::create(Array);
    for (Node member : array_members)
    {
      array->push_back(to_term(member));
    }
    return array;
  }

  Node Resolver::set(const Node& set_members)
  {
    std::map<std::string, Node> members;
    for (Node member : *set_members)
    {
      if (member->type() == Expr)
      {
        throw std::runtime_error("Not implemented");
      }

      std::string repr = to_key(member);
      if (!members.contains(repr))
      {
        members[repr] = to_term(member);
      }
    }

    Node set = NodeDef::create(Set);
    for (auto [_, member] : members)
    {
      set->push_back(member);
    }
    return set;
  }

  Node Resolver::set_intersection(const Node& lhs, const Node& rhs)
  {
    if (lhs != Set)
    {
      return err(lhs, "intersection: both arguments must be sets");
    }

    if (rhs != Set)
    {
      return err(rhs, "intersection: both arguments must be sets");
    }

    Node set = NodeDef::create(Set);

    std::set<std::string> values;
    for (auto term : *lhs)
    {
      values.insert(to_key(term));
    }

    for (auto term : *rhs)
    {
      if (values.contains(to_key(term)))
      {
        set->push_back(term->clone());
      }
    }

    return set;
  }

  Node Resolver::set_union(const Node& lhs, const Node& rhs)
  {
    if (lhs != Set)
    {
      return err(lhs, "union: both arguments must be sets");
    }

    if (rhs != Set)
    {
      return err(rhs, "union: both arguments must be sets");
    }

    Node set = NodeDef::create(Set);

    std::map<std::string, Node> members;
    for (auto term : *lhs)
    {
      members[to_key(term)] = term;
    }

    for (auto term : *rhs)
    {
      std::string key = to_key(term);
      if (!members.contains(key))
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
    if (lhs != Set)
    {
      return err(lhs, "difference: both arguments must be sets");
    }

    if (rhs != Set)
    {
      return err(rhs, "difference: both arguments must be sets");
    }

    Node set = NodeDef::create(Set);
    std::set<std::string> values;
    for (auto term : *rhs)
    {
      values.insert(to_key(term));
    }

    for (auto term : *lhs)
    {
      if (!values.contains(to_key(term)))
      {
        set->push_back(term->clone());
      }
    }

    return set;
  }

  Nodes Resolver::object_lookdown(const Node& object, const Node& query)
  {
    return Resolver::object_lookdown(object, to_key(query));
  }

  Nodes Resolver::object_lookdown(
    const Node& object, const std::string_view& query_str)
  {
    assert(object == Object);
    Nodes terms;
    for (auto& object_item : *object)
    {
      assert(object_item == ObjectItem);
      Node key = object_item / Key;
      if (key->type() == Ref)
      {
        throw std::runtime_error("Not implemented.");
      }

      std::string key_str = to_key(key);
      if (key_str == query_str)
      {
        terms.push_back((object_item / Val)->clone());
        continue;
      }

      auto maybe_string = unwrap(key, JSONString);
      if (
        maybe_string.success &&
        maybe_string.node->location().view() == query_str)
      {
        // the query string was provided without quotes
        terms.push_back((object_item / Val)->clone());
      }
    }

    return terms;
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
}