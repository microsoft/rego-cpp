#include "internal.hh"

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
      if (
        lhs_number->type() == Int && rhs_number->type() == Int &&
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
      auto maybe_lhs_set = unwrap(lhs, {Set});
      auto maybe_rhs_set = unwrap(rhs, {Set});
      if (maybe_lhs_set.success && maybe_rhs_set.success)
      {
        return bininfix(op, maybe_lhs_set.node, maybe_rhs_set.node);
      }

      if (maybe_lhs_number.success && maybe_rhs_set.success)
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
    auto maybe_lhs_set = unwrap(lhs, {Set});
    auto maybe_rhs_set = unwrap(rhs, {Set});

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
      return err(
        op->parent()->shared_from_this(),
        "Cannot perform set operations on non-set values");
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
      return do_bool(op, to_json(lhs), to_json(rhs));
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

      if (index->type() == Int)
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
      Node query = arg;
      if (query->type() == Term)
      {
        query = arg->front();
      }
      return object_lookdown(container, query);
    }

    if (container->type() == Data || container->type() == DataItem || container->type() == Submodule)
    {
      Node key = arg->front();
      std::string key_str = std::string((container / Key)->location().view());
      key_str += "." + strip_quotes(to_json(key));
      return module_lookdown(container, key_str);
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
      return value->clone();
    }

    if (
      value->type() == Array || value->type() == Set ||
      value->type() == Object || value->type() == Scalar)
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

  Node Resolver::object(const Node& object_items, bool is_rule)
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
      if (contains(items, key))
      {
        std::string current = to_json(items[key] / Val);
        std::string next = to_json(object_items->at(i + 1));
        if (current != next)
        {
          if (is_rule)
          {
            return err(
              object_items->at(i),
              "complete rules must not produce multiple outputs",
              EvalConflictError);
          }

          return err(
            object_items->at(i),
            "object keys must be unique",
            EvalConflictError);
        }
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
      if (!contains(members, repr))
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
      if (contains(values, to_json(term)))
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
      if (!contains(members, key))
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
      if (!contains(values, to_json(term)))
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

    if (statement->type() == UnifyExprNot)
    {
      return not_str(statement);
    }

    return expr_str(statement);
  }

  Resolver::NodePrinter Resolver::expr_str(const Node& node)
  {
    return {node, [](std::ostream& os, const Node& unifyexpr) -> std::ostream& {
              Node lhs = unifyexpr / Var;
              Node rhs = unifyexpr / Val;
              os << lhs->location().view() << " = "
                 << (rhs->type() == Function ? func_str(rhs) : arg_str(rhs));
              return os;
            }};
  }

  Resolver::NodePrinter Resolver::term_str(const Node& node)
  {
    return {node, [](std::ostream& os, const Node& term) -> std::ostream& {
              os << term->type().str() << "(" << to_json(term) << ")";
              return os;
            }};
  }

  Resolver::NodePrinter Resolver::with_str(const Node& node)
  {
    return {
      node, [](std::ostream& os, const Node& unifyexprwith) -> std::ostream& {
        Node unifybody = unifyexprwith / UnifyBody;
        os << "{";
        std::string sep = "";
        for (Node expr : *unifybody)
        {
          if (expr->type() == UnifyExpr)
          {
            os << sep << expr_str(expr);
            sep = "; ";
          }
          else if (expr->type() == UnifyExprNot)
          {
            os << sep << not_str(expr);
            sep = "; ";
          }
        }
        os << "} ";
        sep = "";
        Node withseq = unifyexprwith / WithSeq;
        for (Node with : *withseq)
        {
          Node ref = with / RuleRef;
          Node var = with / Var;
          os << sep << "with " << ref_str(ref) << " as " << arg_str(var);
          sep = "; ";
        }
        return os;
      }};
  }

  Resolver::NodePrinter Resolver::compr_str(const Node& node)
  {
    return {
      node, [](std::ostream& os, const Node& unifyexprcompr) -> std::ostream& {
        Node lhs = unifyexprcompr / Var;
        Node rhs = unifyexprcompr / Val;
        Node unifybody = unifyexprcompr / UnifyBody;
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

  Resolver::NodePrinter Resolver::enum_str(const Node& node)
  {
    return {
      node, [](std::ostream& os, const Node& unifyexprenum) -> std::ostream& {
        Node item = unifyexprenum / Item;
        Node itemseq = unifyexprenum / ItemSeq;
        Node unifybody = unifyexprenum / NestedBody / UnifyBody;
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

  Resolver::NodePrinter Resolver::not_str(const Node& node)
  {
    return {
      node, [](std::ostream& os, const Node& unifyexprnot) -> std::ostream& {
        Node unifybody = unifyexprnot->front();
        os << "not {";
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

  Resolver::NodePrinter Resolver::func_str(const Node& node)
  {
    return {node, [](std::ostream& os, const Node& function) -> std::ostream& {
              Node name = function / JSONString;
              Node args = function / ArgSeq;
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

  Resolver::NodePrinter Resolver::arg_str(const Node& node)
  {
    return {node, [](std::ostream& os, const Node& arg) -> std::ostream& {
              if (arg->type() == Var)
              {
                os << arg->location().view();
              }
              else if (arg->type() == NestedBody)
              {
                os << "{";
                Node body = arg / Val;
                std::string sep = "";
                for (Node expr : *body)
                {
                  if (expr->type() != Local)
                  {
                    os << sep << stmt_str(expr);
                    sep = "; ";
                  }
                }
                os << "}";
              }
              else if (arg->type() == VarSeq)
              {
                os << "[";
                std::string sep = "";
                for (Node var : *arg)
                {
                  os << sep << var->location().view();
                  sep = ", ";
                }
                os << "]";
              }
              else
              {
                os << to_json(arg);
              }
              return os;
            }};
  }

  Resolver::NodePrinter Resolver::ref_str(const Node& node)
  {
    return {node, [](std::ostream& os, const Node& ref_) -> std::ostream& {
              Node ref = ref_;
              if (ref->type() == RuleRef || ref->type() == RefTerm)
              {
                ref = ref->front();
                if (ref->type() == Var)
                {
                  os << ref->location().view();
                  return os;
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
        std::string rulearg_str = to_json(rulearg->front());
        std::string arg_str = to_json(arg);
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

  Nodes Resolver::module_lookdown(const Node& container, const std::string& query)
  {
    Node module = container;
    if(module->type() == Submodule || module->type() == Data || module->type() == DataItem)
    {
      module = module / Val;
    }

    if(module->type() != DataModule)
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

      if (name == query)
      {
        rules.push_back(rule);
      }
    }

    return rules;
  }

  Nodes Resolver::object_lookdown(const Node& object, const Node& query)
  {
    Nodes terms;
    std::string query_str = to_json(query);
    for (auto& object_item : *object)
    {
      Node key = object_item / Key;
      if (key->type() == Ref)
      {
        throw std::runtime_error("Not implemented.");
      }

      std::string key_str = to_json(key);
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

  Node Resolver::reduce_termset(const Node& termset)
  {
    Node reduce = NodeDef::create(TermSet);
    std::set<std::string> values;
    for (Node term : *termset)
    {
      std::string repr = to_json(term);
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

  Node Resolver::resolve_query(const Node& query, const BuiltIns& builtins)
  {
    Nodes defs = query->front()->lookup();
    if (defs.size() != 1)
    {
      return err(query, "query not found");
    }

    Node rulebody = defs[0] / Val;
    Location rulename{"query"};
    UnifierCache cache = std::make_shared<std::map<UnifierKey, Unifier>>();
    auto unifier = UnifierDef::create(
      UnifierKey{rulename, UnifierType::RuleBody},
      rulename,
      rulebody,
      std::make_shared<std::vector<Location>>(),
      std::make_shared<std::vector<ValuesLookup>>(),
      builtins,
      cache);
    Node result = unifier->unify();

    // now that unification is complete, we need to empty the
    // Unifier cache. Since they all maintain a pointer to the
    // cache, it will not be able to be freed otherwise.
    cache->clear();

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

      if (term->type() != Term)
      {
        term = Term << term;
      }

      if (is_undefined(term))
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

      if (starts_with(name, "value$"))
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
      return False ^ "false";
    }

    std::string index_str = to_json(index);
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
      indices = array_find(items, to_json(item));
    }
    else if (items->type() == Object)
    {
      indices = object_find(items, to_json(item));
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

  Resolver::NodePrinter Resolver::body_str(const Node& node)
  {
    return {node, [](std::ostream& os, const Node& unifybody) -> std::ostream& {
              os << "{" << std::endl;
              for (auto& stmt : *unifybody)
              {
                if (stmt->type() == Local)
                {
                  os << "  local " << (stmt / Var)->location().view()
                     << std::endl;
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

  Resolver::NodePrinter Resolver::rego_str(const Node& node)
  {
    return {
      node, [](std::ostream& os, const Node& rego) -> std::ostream& {
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
      std::string query_str = to_json(query);
      for (auto& object_item : *current)
      {
        Node key = object_item / Key;
        std::string key_str = to_json(key);
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

    if (current->type() != Object)
    {
      LOG_WARN(
        "Conflict: cannot merge partials into non-object: ", term_str(current));
      Node replacement = NodeDef::create(Object);
      current->parent()->replace(current, replacement);
      current = replacement;
    }

    Node key = (Scalar << (JSONString ^ path.substr(start)));
    std::string key_str = to_json(key);
    Node existing = nullptr;
    for (auto& object_item : *current)
    {
      Node item_key = object_item / Key;
      std::string item_key_str = to_json(item_key);
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
}