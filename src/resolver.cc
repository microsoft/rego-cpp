#include "resolver.h"

#include "errors.h"
#include "unifier.h"
#include "utils.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  inline const auto wfi =
    (DataItemSeq <<= DataItem++)
    | (DataItem <<= Var * (Val >>= DataModule | Term))
    | (UnifyExpr <<= Var * (Val >>= Var | Scalar | Function))
    | (UnifyExprWith <<= UnifyBody * WithSeq)
    | (UnifyExprCompr <<= Var * (Val >>= ArrayCompr | SetCompr | ObjectCompr) * UnifyBody)
    | (UnifyExprEnum <<= (Unify >>= Var) * (Item >>= Var) * (ItemSeq >>= Var) * NestedBody)
    | (NestedBody <<= Key * UnifyBody)
    | (Local <<= Var * Term)
    | (Term <<= Scalar | Array | Object | Set)
    | (Scalar <<= JSONString | JSONInt | JSONFloat | JSONTrue | JSONFalse | JSONNull)
    | (ArgVal <<= Scalar | Array | Object | Set)
    | (ObjectItem <<= (Key >>= Term) * (Val >>= Term))
    | (ArgVar <<= Var * (Val >>= Term | Undefined))
    | (RuleFunc <<= Var * RuleArgs * (Body >>= UnifyBody) * (Val >>= UnifyBody))
    | (Function <<= JSONString * ArgSeq)
    | (Submodule <<= Key * DataModule)
    | (With <<= Ref * Var)
    | (Ref <<= RefHead * RefArgSeq)
    ;
  // clang-format on

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

  BigInt get_int(const Node& node)
  {
    return BigInt(node->location());
  }

  double get_double(const Node& node)
  {
    return std::stod(to_json(node));
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

    return JSONInt ^ value.loc();
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
    return JSONFloat ^ oss.str();
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
      return JSONTrue ^ "true";
    }
    else
    {
      return JSONFalse ^ "false";
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
      return JSONTrue ^ "true";
    }
    else
    {
      return JSONFalse ^ "false";
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
      return JSONTrue ^ "true";
    }
    else
    {
      return JSONFalse ^ "false";
    }
  }
}

namespace rego
{
  BigInt Resolver::get_int(const Node& node)
  {
    assert(node->type() == JSONInt);
    return ::get_int(node);
  }

  Node Resolver::scalar(BigInt value)
  {
    return JSONInt ^ value.loc();
  }

  double Resolver::get_double(const Node& node)
  {
    assert(node->type() == JSONFloat || node->type() == JSONInt);
    return ::get_double(node);
  }

  Node Resolver::scalar(double value)
  {
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::max_digits10 - 1)
        << std::noshowpoint << value;
    return JSONFloat ^ oss.str();
  }

  std::string Resolver::get_string(const Node& node)
  {
    Node value = node;
    if (value->type() == Term)
    {
      value = value->front();
    }

    if (value->type() == Scalar)
    {
      value = value->front();
    }

    if (value->type() == JSONString)
    {
      return strip_quotes(value->location().view());
    }

    return std::string(value->location().view());
  }

  Node Resolver::scalar(const std::string& value)
  {
    return JSONString ^ ("\"" + value + "\"");
  }

  bool Resolver::get_bool(const Node& node)
  {
    assert(node->type() == JSONTrue || node->type() == JSONFalse);
    return node->type() == JSONTrue;
  }

  Node Resolver::scalar(bool value)
  {
    if (value)
    {
      return JSONTrue ^ "true";
    }
    else
    {
      return JSONFalse ^ "false";
    }
  }

  Node Resolver::negate(const Node& node)
  {
    if (node->type() == JSONInt)
    {
      BigInt value = get_int(node);
      return JSONInt ^ value.negate().loc();
    }
    else if (node->type() == JSONFloat)
    {
      double value = get_double(node);
      value *= -1.0;
      return JSONFloat ^ std::to_string(value);
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
      return JSONFalse ^ "false";
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
      if (
        lhs_number->type() == JSONInt && rhs_number->type() == JSONInt &&
        op->type() != Divide)
      {
        return do_arith(op, get_int(lhs_number), get_int(rhs_number));
      }
      else
      {
        return do_arith(op, get_double(lhs_number), get_double(rhs_number));
      }
    }
    else
    {
      auto maybe_lhs_set = maybe_unwrap_set(lhs);
      auto maybe_rhs_set = maybe_unwrap_set(rhs);
      if (maybe_lhs_set.has_value() && maybe_rhs_set.has_value())
      {
        return bininfix(op, maybe_lhs_set.value(), maybe_rhs_set.value());
      }

      if (maybe_lhs_number.has_value() && maybe_rhs_set.has_value())
      {
        return err(rhs, "operand 2 must be number but got set", EvalTypeError);
      }

      return err(
        op->parent()->shared_from_this(),
        "Cannot perform arithmetic operations on non-numeric values");
    }
  }

  Node Resolver::bininfix(const Node& op, const Node& lhs, const Node& rhs)
  {
    auto maybe_lhs_set = maybe_unwrap_set(lhs);
    auto maybe_rhs_set = maybe_unwrap_set(rhs);

    if (maybe_lhs_set.has_value() && maybe_rhs_set.has_value())
    {
      if (op->type() == And)
      {
        return set_intersection(maybe_lhs_set.value(), maybe_rhs_set.value());
      }
      if (op->type() == Or)
      {
        return set_union(maybe_lhs_set.value(), maybe_rhs_set.value());
      }
      if (op->type() == Subtract)
      {
        return set_difference(maybe_lhs_set.value(), maybe_rhs_set.value());
      }

      return err(op, "Unsupported binary operator");
    }
    else
    {
      return err(
        op->parent()->shared_from_this(),
        "Cannot perform set operations on non-set values");
    }
  }

  Node Resolver::boolinfix(const Node& op, const Node& lhs, const Node& rhs)
  {
    if (lhs->type() == Undefined || rhs->type() == Undefined)
    {
      return JSONFalse ^ "false";
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
        return do_bool(op, get_int(lhs_number), get_int(rhs_number));
      }
      else
      {
        return do_bool(op, get_double(lhs_number), get_double(rhs_number));
      }
    }
    else
    {
      return do_bool(op, to_json(lhs), to_json(rhs));
    }
  }

  std::optional<Node> Resolver::maybe_unwrap_string(const Node& node)
  {
    Node value;
    if (node->type() == Term || node->type() == DataTerm)
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

    if (value->type() == JSONString)
    {
      return value;
    }

    return std::nullopt;
  }

  std::optional<Node> Resolver::maybe_unwrap_bool(const Node& node)
  {
    Node value;
    if (node->type() == Term || node->type() == DataTerm)
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

    if (value->type() == JSONTrue || value->type() == JSONFalse)
    {
      return value;
    }

    return std::nullopt;
  }

  std::optional<Node> Resolver::maybe_unwrap_number(const Node& node)
  {
    Node value;
    if (node->type() == Term || node->type() == DataTerm)
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

    return std::nullopt;
  }

  std::optional<Node> Resolver::maybe_unwrap_int(const Node& node)
  {
    Node value;
    if (node->type() == Term || node->type() == DataTerm)
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

    if (value->type() == JSONInt)
    {
      return value;
    }

    return std::nullopt;
  }

  std::optional<Node> Resolver::maybe_unwrap_set(const Node& node)
  {
    Node value;
    if (node->type() == Term || node->type() == DataTerm)
    {
      value = node->front();
    }
    else
    {
      value = node;
    }

    if (value->type() == Set)
    {
      return value;
    }

    return std::nullopt;
  }

  std::optional<Node> Resolver::maybe_unwrap_array(const Node& node)
  {
    Node value;
    if (node->type() == Term || node->type() == DataTerm)
    {
      value = node->front();
    }
    else
    {
      value = node;
    }

    if (value->type() == Array)
    {
      return value;
    }

    return std::nullopt;
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

      if (index->type() == JSONInt)
      {
        auto i = get_int(index).to_size();
        if (i < container->size())
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
      Node query = arg->front();
      return object_lookdown(container, query);
    }

    if (container->type() == Data || container->type() == DataModule)
    {
      Node key = arg->front();
      std::string key_str = strip_quotes(to_json(key));
      Nodes defs = container->lookdown(key_str);
      if (defs.size() == 0)
      {
        return Nodes({err(container, "No definition found for " + key_str)});
      }

      if (defs[0]->type() == RuleComp || defs[0]->type() == RuleFunc)
      {
        return defs;
      }

      Nodes nodes;
      for (auto& def : defs)
      {
        if (def->type() == DataItem)
        {
          nodes.push_back(wfi / def / Val);
        }
        else if (def->type() == ObjectItem)
        {
          nodes.push_back(wfi / def / Val);
        }
        else if (def->type() == Submodule)
        {
          nodes.push_back(wfi / def / DataModule);
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
          return Nodes({to_term(arg)});
        }
      }

      return std::nullopt;
    }

    return std::nullopt;
  }

  Node Resolver::to_term(const Node& value)
  {
    if (value->type() == Term || value->type() == TermSet)
    {
      return value;
    }

    if (
      value->type() == Array || value->type() == Set ||
      value->type() == Object || value->type() == Scalar)
    {
      return Term << value;
    }

    if (
      value->type() == JSONInt || value->type() == JSONFloat ||
      value->type() == JSONString || value->type() == JSONTrue ||
      value->type() == JSONFalse || value->type() == JSONNull)
    {
      return Term << (Scalar << value);
    }

    return err(value, "Not a term");
  }

  Node Resolver::object(const Node& object_items)
  {
    Node object = NodeDef::create(Object);
    std::map<std::string, Node> items;
    for (std::size_t i = 0; i < object_items->size(); i += 2)
    {
      if (object_items->at(i)->type() == Error)
      {
        return object_items->at(i);
      }

      std::string key = to_json(object_items->at(i));
      if (items.contains(key))
      {
        return err(
          object_items->at(i), "object keys must be unique", EvalConflictError);
      }

      Node item = ObjectItem << to_term(object_items->at(i))
                             << to_term(object_items->at(i + 1));
      items[key] = item;
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
    auto maybe_number = maybe_unwrap_number(value);
    if (maybe_number)
    {
      return negate(maybe_number.value());
    }
    else
    {
      return err(value, "unsupported negation");
    }
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

      std::string repr = to_json(member);
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
    if (lhs->type() != Set)
    {
      return err(lhs, "intersection: both arguments must be sets");
    }

    if (rhs->type() != Set)
    {
      return err(rhs, "intersection: both arguments must be sets");
    }

    Node set = NodeDef::create(Set);
    std::set<std::string> values;
    for (auto term : *lhs)
    {
      values.insert(to_json(term));
    }

    for (auto term : *rhs)
    {
      if (values.contains(to_json(term)))
      {
        set->push_back(term);
      }
    }

    return set;
  }

  Node Resolver::set_union(const Node& lhs, const Node& rhs)
  {
    if (lhs->type() != Set)
    {
      return err(lhs, "union: both arguments must be sets");
    }

    if (rhs->type() != Set)
    {
      return err(rhs, "union: both arguments must be sets");
    }

    std::map<std::string, Node> members;
    for (auto term : *lhs)
    {
      members[to_json(term)] = term;
    }

    for (auto term : *rhs)
    {
      std::string key = to_json(term);
      if (!members.contains(key))
      {
        members[key] = term;
      }
    }

    Node set = NodeDef::create(Set);
    for (auto [_, member] : members)
    {
      set->push_back(member);
    }

    return set;
  }

  Node Resolver::set_difference(const Node& lhs, const Node& rhs)
  {
    if (lhs->type() != Set)
    {
      return err(lhs, "difference: both arguments must be sets");
    }

    if (rhs->type() != Set)
    {
      return err(rhs, "difference: both arguments must be sets");
    }

    Node set = NodeDef::create(Set);
    std::set<std::string> values;
    for (auto term : *rhs)
    {
      values.insert(to_json(term));
    }

    for (auto term : *lhs)
    {
      if (!values.contains(to_json(term)))
      {
        set->push_back(term);
      }
    }

    return set;
  }

  Resolver::NodePrinter Resolver::stmt_str(const Node& statement)
  {
    if (statement->type() == UnifyExprEnum)
    {
      return enum_str(statement);
    }

    if (statement->type() == UnifyExprWith)
    {
      return with_str(statement);
    }

    if (statement->type() == UnifyExprCompr)
    {
      return compr_str(statement);
    }

    return expr_str(statement);
  }

  Resolver::NodePrinter Resolver::expr_str(const Node& unifyexpr)
  {
    return {
      unifyexpr, [](std::ostream& os, const Node& unifyexpr) -> std::ostream& {
        Node lhs = wfi / unifyexpr / Var;
        Node rhs = wfi / unifyexpr / Val;
        os << lhs->location().view() << " = "
           << (rhs->type() == Function ? func_str(rhs) : arg_str(rhs));
        return os;
      }};
  }

  Resolver::NodePrinter Resolver::with_str(const Node& unifyexprwith)
  {
    return {
      unifyexprwith,
      [](std::ostream& os, const Node& unifyexprwith) -> std::ostream& {
        Node unifybody = wfi / unifyexprwith / UnifyBody;
        os << "{";
        std::string sep = "";
        for (Node expr : *unifybody)
        {
          if (expr->type() == UnifyExpr)
          {
            os << sep << expr_str(expr);
            sep = "; ";
          }
        }
        os << "} ";
        sep = "";
        Node withseq = wfi / unifyexprwith / WithSeq;
        for (Node with : *withseq)
        {
          Node ref = wfi / with / Ref;
          Node var = wfi / with / Var;
          os << sep << "with " << ref_str(ref) << " as " << arg_str(var);
          sep = "; ";
        }
        return os;
      }};
  }

  Resolver::NodePrinter Resolver::compr_str(const Node& unifyexprcompr)
  {
    return {
      unifyexprcompr,
      [](std::ostream& os, const Node& unifyexprcompr) -> std::ostream& {
        Node lhs = wfi / unifyexprcompr / Var;
        Node rhs = wfi / unifyexprcompr / Val;
        Node unifybody = wfi / unifyexprcompr / UnifyBody;
        os << lhs->location().view() << " = " << rhs->type().str() << "{";
        std::string sep = "";
        for (Node expr : *unifybody)
        {
          if (expr->type() != Local)
          {
            os << sep << stmt_str(expr);
            sep = "; ";
          }
        }
        os << "}";
        return os;
      }};
  }

  Resolver::NodePrinter Resolver::enum_str(const Node& unifyexprenum)
  {
    return {
      unifyexprenum,
      [](std::ostream& os, const Node& unifyexprenum) -> std::ostream& {
        Node item = wfi / unifyexprenum / Item;
        Node itemseq = wfi / unifyexprenum / ItemSeq;
        Node unifybody = wfi / unifyexprenum / NestedBody / UnifyBody;
        os << "foreach " << item->location().view() << " in "
           << itemseq->location().view() << " unify {";
        std::string sep = "";
        for (Node expr : *unifybody)
        {
          if (expr->type() != Local)
          {
            os << sep << stmt_str(expr);
            sep = "; ";
          }
        }
        os << "}";
        return os;
      }};
  }

  Resolver::NodePrinter Resolver::func_str(const Node& function)
  {
    return {
      function, [](std::ostream& os, const Node& function) -> std::ostream& {
        Node name = wfi / function / JSONString;
        Node args = wfi / function / ArgSeq;
        os << name->location().view() << "(";
        std::string sep = "";
        for (const auto& child : *args)
        {
          os << sep << arg_str(child);
          sep = ", ";
        }
        os << ")";
        return os;
      }};
  }

  Resolver::NodePrinter Resolver::arg_str(const Node& arg)
  {
    return {arg, [](std::ostream& os, const Node& arg) -> std::ostream& {
              if (arg->type() == Var)
              {
                os << arg->location().view();
              }
              else
              {
                os << to_json(arg);
              }
              return os;
            }};
  }

  Resolver::NodePrinter Resolver::ref_str(const Node& ref)
  {
    return {ref, [](std::ostream& os, const Node& ref) -> std::ostream& {
              if (ref->type() == VarSeq)
              {
                std::string sep = "";
                for (auto var : *ref)
                {
                  os << sep << var->location().view();
                  sep = ".";
                }
                return os;
              }

              Node refhead = wfi / ref / RefHead;
              Node refargseq = wfi / ref / RefArgSeq;
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
              return os;
            }};
  }

  std::ostream& operator<<(
    std::ostream& os, const Resolver::NodePrinter& printer)
  {
    return printer.printer(os, printer.node);
  }

  Node Resolver::inject_args(const Node& rulefunc, const Nodes& args)
  {
    Node ruleargs = wfi / rulefunc / RuleArgs;
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
        std::string rulearg_str = to_json(rulearg->front());
        std::string arg_str = to_json(arg);
        if (rulearg_str != arg_str)
        {
          return Undefined;
        }
      }
      else if (rulearg->type() == ArgVar)
      {
        rulearg->at(wfi.index(ArgVar, Val)) = arg;
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

  bool Resolver::is_undefined(const Node& node)
  {
    if (node->type() == DataModule)
    {
      return false;
    }

    if (node->type() == Undefined)
    {
      return true;
    }

    for (auto& child : *node)
    {
      if (is_undefined(child))
      {
        return true;
      }
    }

    return false;
  }

  bool Resolver::is_falsy(const Node& node)
  {
    if (node->type() == Undefined)
    {
      return true;
    }

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
    else if (is_undefined(value))
    {
      return true;
    }

    return false;
  }

  Nodes Resolver::object_lookdown(const Node& object, const Node& query)
  {
    Nodes terms;
    std::string query_str = to_json(query);
    for (auto& object_item : *object)
    {
      Node key = wfi / object_item / Key;
      if (key->type() == Ref)
      {
        throw std::runtime_error("Not implemented.");
      }

      std::string key_str = to_json(key);
      if (key_str == query_str)
      {
        terms.push_back((wfi / object_item / Val)->clone());
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
            if (def->type() == DataItem || def->type() == Submodule)
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

  Node Resolver::resolve_query(const Node& query, const BuiltIns& builtins)
  {
    Nodes defs = query->front()->lookup();
    if (defs.size() != 1)
    {
      return err(query, "query not found");
    }

    Node rulebody = defs[0] / Val;
    auto unifier = UnifierDef::create(
      {"query"},
      rulebody,
      std::make_shared<std::vector<Location>>(),
      std::make_shared<std::vector<ValuesLookup>>(),
      builtins,
      std::make_shared<NodeMap<Unifier>>());
    Node result = unifier->unify();
    if (result->type() == Error)
    {
      return Query << result;
    }

    result = NodeDef::create(Query);

    for (auto& child : *rulebody)
    {
      if (child->type() == Error)
      {
        result->push_back(child);
        continue;
      }

      if (child->type() != Local)
      {
        continue;
      }

      Node var = (wfi / child / Var)->clone();
      Node term = (wfi / child / Term)->clone();

      if (term->type() == TermSet)
      {
        if (term->size() == 0)
        {
          term = Undefined;
        }
        else
        {
          result->push_back(err(child, "Multiple values for binding"));
        }
      }

      if (term->type() != Term)
      {
        term = Term << term;
      }

      if (term->front()->type() == Undefined)
      {
        continue;
      }

      if (contains_multiple_outputs(term))
      {
        result->push_back(err(
          term,
          "complete rules must not produce multiple outputs",
          EvalConflictError));
        continue;
      }

      std::string name = std::string(var->location().view());

      if (name.starts_with("value$"))
      {
        result->push_back(term);
      }
      else if (name.find('$') == std::string::npos || name[0] == '$')
      {
        // either a user-defined variable (no $) or
        // a fuzzer variable ($ followed by a number)
        result->push_back(Binding << var << term);
      }
    }

    return result;
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
      indices = array_find(items, to_json(item));
    }
    else if (items->type() == Object)
    {
      indices = object_find(items, to_json(item));
    }
    else
    {
      return JSONFalse ^ "false";
    }

    std::string index_str = to_json(index);
    for (auto& i : indices)
    {
      if (i == index_str)
      {
        return JSONTrue ^ "true";
      }
    }

    return JSONFalse ^ "false";
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
      indices = array_find(items, to_json(item));
    }
    else if (items->type() == Object)
    {
      indices = object_find(items, to_json(item));
    }
    else
    {
      return JSONFalse ^ "false";
    }

    if (indices.size() > 0)
    {
      return JSONTrue ^ "True";
    }

    return JSONFalse ^ "false";
  }

  std::vector<std::string> Resolver::array_find(
    const Node& array, const std::string& search)
  {
    std::vector<std::string> indices;
    for (std::size_t i = 0; i < array->size(); ++i)
    {
      Node item = array->at(i);
      if (to_json(item) == search)
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
      if (to_json(objectitem / Val) == search)
      {
        indices.push_back(to_json(objectitem / Key));
      }
    }

    return indices;
  }

  std::string Resolver::NodePrinter::str() const
  {
    std::ostringstream buf;
    buf << *this;
    return buf.str();
  }

  Node Resolver::unwrap(
    const Node& node,
    const Token& type,
    const std::string& error_prefix,
    const std::string& error_code,
    bool specify_number)
  {
    Node value;
    if (node->type() == Term || node->type() == DataTerm)
    {
      value = node->front();
    }
    else
    {
      value = node;
    }

    if (value->type() == type)
    {
      return value;
    }

    if (value->type() == Scalar)
    {
      value = value->front();
      if (value->type() == type)
      {
        return value;
      }
    }

    std::ostringstream error;
    error << error_prefix << "must be " << type_name(type, specify_number)
          << " but got " << type_name(value->type(), specify_number);
    return err(node, error.str(), error_code);
  }

  std::optional<Node> Resolver::maybe_unwrap(
    const Node& node, const std::set<Token>& types)
  {
    Node value;
    if (node->type() == Term || node->type() == DataTerm)
    {
      value = node->front();
    }
    else
    {
      value = node;
    }

    if (types.contains(value->type()))
    {
      return value;
    }

    if (value->type() == Scalar)
    {
      value = value->front();
      if (types.contains(value->type()))
      {
        return value;
      }
    }

    return std::nullopt;
  }

  std::string Resolver::type_name(const Token& type, bool specify_number)
  {
    if (type == JSONInt)
    {
      if (specify_number)
      {
        return "integer number";
      }
      return "number";
    }

    if (type == JSONFloat)
    {
      if (specify_number)
      {
        return "floating-point number";
      }
      return "number";
    }

    if (type == JSONString)
    {
      return "string";
    }

    if (type == JSONTrue || type == JSONFalse)
    {
      return "boolean";
    }

    return std::string(type.str());
  }

  std::string Resolver::type_name(const Node& node, bool specify_number)
  {
    Node value = node;
    if (value->type() == Term)
    {
      value = value->front();
    }

    if (value->type() == Scalar)
    {
      value = value->front();
    }

    return type_name(value->type(), specify_number);
  }

  Resolver::NodePrinter Resolver::body_str(const Node& unifybody)
  {
    return {
      unifybody, [](std::ostream& os, const Node& unifybody) -> std::ostream& {
        os << "{" << std::endl;
        for (auto& stmt : *unifybody)
        {
          if (stmt->type() == Local)
          {
            os << "  local " << (stmt / Var)->location().view() << std::endl;
          }
          else
          {
            os << "  " << stmt_str(stmt) << std::endl;
          }
        }
        os << "}";
        return os;
      }};
  }

  Resolver::NodePrinter Resolver::rego_str(const Node& rego)
  {
    return {
      rego, [](std::ostream& os, const Node& rego) -> std::ostream& {
        std::deque<Node> queue;
        queue.push_back(rego / Data);
        os << std::endl << std::endl;
        while (!queue.empty())
        {
          Node current = queue.front();
          queue.pop_front();
          if (RuleTypes.contains(current->type()))
          {
            os << current->type().str() << " ";
            os << (current / Var)->location().view();
            if (current->type() == RuleFunc || current->type() == RuleComp)
            {
              os << "#" << (current / JSONInt)->location().view();
            }
            if (current->type() == RuleFunc)
            {
              os << "(";
              std::string sep = "";
              for (auto& arg : *(current / RuleArgs))
              {
                os << sep << (arg / Var)->location().view();
                sep = ", ";
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
              os << body_str(body) << std::endl;
            }
            os << "value: ";
            Node value = current / Val;
            if (value->type() == Term)
            {
              os << to_json(value) << std::endl;
            }
            else
            {
              os << body_str(value) << std::endl;
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
        return os;
      }};
  }

  void Resolver::flatten_terms_into(const Node& termset, Node& terms)
  {
    if (termset->type() == Term)
    {
      terms->push_back(termset->front());
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
        terms->push_back(term->front());
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
      items->push_back(array->front());
      items->push_back(array->back());
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
        items->push_back(array->front());
        items->push_back(array->back());
      }
      else
      {
        items->push_back(err(term, "Not a term"));
      }
    }
  }
}