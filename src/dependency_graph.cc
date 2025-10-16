#include "internal.hh"
#include "rego.hh"

#include <algorithm>
#include <stdexcept>

namespace
{
  using namespace rego;

  inline const auto RuleLocal = TokenDef("rego-ir-rulelocal");
  inline const auto ScanLocal = TokenDef("rego-ir-scanlocal");
  inline const auto Capture = TokenDef("rego-ir-capture", flag::print);
  inline const auto StaticIsObject = TokenDef("rego-ir-staticisobject");

  Node to_expr(Node expr)
  {
    if (expr == UnifyVar)
    {
      return Expr << (Term << (Var ^ expr));
    }

    if (expr->in({Ref, Var, Scalar, Array, Object, Set}))
    {
      return Expr << (Term << expr);
    }

    if (expr == Term)
    {
      return Expr << expr;
    }

    if (expr == Expr)
    {
      return expr;
    }

    return err(expr, "Not an expression");
  }

  Nodes get_unifyvars(Node node)
  {
    Nodes unifyvars;
    Nodes frontier({node});
    while (!frontier.empty())
    {
      Node current = frontier.back();
      frontier.pop_back();
      if (current == UnifyVar)
      {
        unifyvars.push_back(current);
        continue;
      }

      frontier.insert(frontier.end(), current->begin(), current->end());
    }

    return unifyvars;
  }
}

namespace rego
{
  std::string_view DependencyGraph::LiteralNode::str() const
  {
    // need better debug info here
    Node expr = literal->front();
    if (expr == Expr)
    {
      Node exprtype = expr->front();
      if (exprtype == ExprInfix)
      {
        return (exprtype / InfixOperator)->front()->front()->type().str();
      }

      return exprtype->type().str();
    }

    return expr->type().str();
  }

  void DependencyGraph::add_locals(std::set<std::string>& names, Node node)
  {
    Nodes frontier({node});
    while (!frontier.empty())
    {
      Node current = frontier.back();
      frontier.pop_back();

      if (current == UnifyVar)
      {
        names.insert(std::string(current->location().view()));
        continue;
      }

      if (current == RefArgDot)
      {
        continue;
      }

      if (current == Var)
      {
        Nodes results = current->lookup();

        if (!results.empty() && results.front()->in({Local, ExprAssign}))
        {
          results = m_scope->look(current->location());
        }

        if (results.empty() || results.front()->in({Local, ExprAssign}))
        {
          std::string name(current->location().view());
          if (name != "input")
          {
            names.insert(std::string(current->location().view()));
          }
        }

        continue;
      }

      if (current->in({ArrayCompr, ObjectCompr, SetCompr}))
      {
        Node query = current / Query;
        DependencyGraph subgraph(m_scope, {query->begin(), query->end()});
        names.insert(subgraph.captures().begin(), subgraph.captures().end());
        continue;
      }

      if (current == ExprCall)
      {
        current = current / ExprSeq;
      }

      frontier.insert(frontier.end(), current->begin(), current->end());
    }
  }

  void DependencyGraph::update_edges(LiteralNode& node)
  {
    add_locals(node.in_edges, node.literal / WithSeq);

    Node expr = node.literal->front();
    if (expr->in({Local, RuleLocal, EveryLocal, ScanLocal}))
    {
      std::string name = std::string(expr->front()->location().view());
      if (expr != EveryLocal && !name.starts_with("funcarg$"))
      {
        name += "#empty";
      }

      node.out_edges.insert(name);
      return;
    }

    if (expr == Capture)
    {
      std::string name(expr->location().view());
      node.out_edges.insert(name);
      return;
    }

    if (expr == ExprAssign)
    {
      std::string name((expr / Lhs)->location().view());
      std::string name_empty = name + "#empty";

      node.out_edges.insert(name);
      node.in_edges.insert(name_empty);
      add_locals(node.in_edges, expr / Rhs);
      return;
    }

    if (expr == ExprScan)
    {
      std::string indexname((expr / Key)->front()->location().view());
      std::string valuename((expr / Val)->front()->location().view());
      std::string indexname_empty = indexname + "#empty";
      std::string valuename_empty = valuename + "#empty";
      node.in_edges.insert(indexname_empty);
      node.in_edges.insert(valuename_empty);
      node.out_edges.insert(indexname);
      node.out_edges.insert(valuename);
      add_locals(node.in_edges, expr / Expr);
      return;
    }

    if (expr == ExprEvery)
    {
      std::string sourcename((expr / Var)->location().view());
      node.in_edges.insert(sourcename);
      Node query = expr / Query;
      DependencyGraph subgraph(m_scope, {query->begin(), query->end()});
      node.in_edges.insert(
        subgraph.captures().begin(), subgraph.captures().end());
      return;
    }

    if (expr->in({Expr, NotExpr}))
    {
      add_locals(node.in_edges, expr / Expr);
      return;
    }

    if (expr == ExprIsArray)
    {
      std::string name((expr / Var)->location().view());
      std::string name_array = name + "#array";

      node.out_edges.insert(name_array);
      node.in_edges.insert(name);
      return;
    }

    if (expr == ExprAssignFromArray)
    {
      std::string name((expr / AssignVar)->location().view());
      std::string name_empty = name + "#empty";
      std::string arrayname =
        std::string((expr / Var)->location().view()) + "#array";
      node.in_edges.insert(arrayname);
      node.in_edges.insert(name_empty);
      node.out_edges.insert(name);
      return;
    }

    if (expr == ExprIsObject)
    {
      std::string name((expr / Var)->location().view());
      std::string name_object = name + "#object";

      node.out_edges.insert(name_object);
      node.in_edges.insert(name);
      return;
    }

    if (expr == StaticIsObject)
    {
      std::string name(expr->front()->location().view());
      std::string name_object = name + "#object";

      node.out_edges.insert(name_object);
      node.in_edges.insert(name);
      return;
    }

    if (expr == ExprAssignFromObject)
    {
      std::string name((expr / AssignVar)->location().view());
      std::string name_empty = name + "#empty";
      std::string objectname =
        std::string((expr / Var)->location().view()) + "#object";
      node.in_edges.insert(objectname);
      node.in_edges.insert(name_empty);
      add_locals(node.in_edges, expr / Expr);
      node.out_edges.insert(name);
      return;
    }

    logging::Error() << "Unrecognized literal expr: " << expr;
    throw std::runtime_error("Unrecognized literal expr");
  }

  bool DependencyGraph::is_assigned(const std::string& name)
  {
    auto it = m_locals.find(name);
    if (it == m_locals.end())
    {
      return false;
    }

    return it->second.in_edge.has_value();
  }

  bool DependencyGraph::any_unassigned(const Nodes& nodes)
  {
    for (const auto& node : nodes)
    {
      assert(node == UnifyVar);
      if (!is_assigned(std::string(node->location().view())))
      {
        return true;
      }
    }

    return false;
  }

  void DependencyGraph::add_literal(Node literal, Location location)
  {
    bool is_capture = literal == Capture;
    if (literal != Literal)
    {
      literal = (Literal ^ location) << literal << WithSeq;
    }

    size_t index = m_literals.size();
    LiteralNode node{index, literal};
    update_edges(node);
    m_literals.push_back(node);

    for (auto& name : node.in_edges)
    {
      auto it = m_locals.find(name);
      if (m_locals.find(name) == m_locals.end())
      {
        m_locals[name] = {name, is_capture};
      }
      else
      {
        m_locals[name].captured = is_capture;
      }

      m_locals[name].out_edges.push_back(index);
    }

    for (auto& name : node.out_edges)
    {
      if (m_locals.find(name) == m_locals.end())
      {
        m_locals[name] = {name, is_capture};
      }
      else
      {
        m_locals[name].captured = is_capture;
      }

      if (m_locals[name].in_edge.has_value())
      {
        logging::Error() << "Multiple assignment to local " << name << ": "
                         << literal;
      }

      m_locals[name].in_edge = index;
    }
  }

  void DependencyGraph::add_assign(Node var, Node expr)
  {
    Location location;
    Node literal = var->parent(Literal);
    if (literal == nullptr)
    {
      literal = expr->parent(Literal);
    }

    if (literal)
    {
      location = literal->location();
    }

    std::string name_empty = std::string(var->location().view()) + "#empty";
    if (!is_assigned(name_empty))
    {
      add_literal(Local << (Ident ^ var), {});
    }

    add_literal(
      Literal << (ExprAssign << (AssignVar ^ var) << to_expr(expr))
              << (literal / WithSeq)->clone(),
      location);
  }

  void DependencyGraph::add_equals(Node lhs, Node rhs)
  {
    Location location;
    Node literal = lhs->parent(Literal);
    if (literal == nullptr)
    {
      literal = rhs->parent(Literal);
    }

    Node withseq = NodeDef::create(WithSeq);
    if (literal)
    {
      location = literal->location();
      withseq = (literal / WithSeq)->clone();
    }

    add_literal(
      Literal << (Expr
                  << (ExprInfix << to_expr(lhs)
                                << (InfixOperator << (BoolOperator << Equals))
                                << to_expr(rhs)))
              << withseq,
      location);
  }

  bool DependencyGraph::add_unifyvars(
    Node lhs_var, std::string& lhs_name, Node rhs_var, std::string& rhs_name)
  {
    assert(lhs_var && lhs_var == UnifyVar);
    assert(rhs_var && rhs_var == UnifyVar);
    bool lhs_assigned = is_assigned(lhs_name);
    bool rhs_assigned = is_assigned(rhs_name);
    if (lhs_assigned)
    {
      if (rhs_assigned)
      {
        add_equals(lhs_var, rhs_var);
        return false;
      }

      add_assign(rhs_var, lhs_var);
      return false;
    }

    if (rhs_assigned)
    {
      add_assign(lhs_var, rhs_var);
      return false;
    }

    return true;
  }

  bool DependencyGraph::add_unifyvar_term(
    Node lhs_var, std::string& lhs_name, Node rhs_term)
  {
    assert(lhs_var && lhs_var == UnifyVar);
    assert(rhs_term && rhs_term == Term);
    bool lhs_assigned = is_assigned(lhs_name);
    std::set<std::string> rhs_locals;
    add_locals(rhs_locals, rhs_term);
    if (rhs_locals.empty())
    {
      if (lhs_assigned)
      {
        add_equals(lhs_var, rhs_term);
        return false;
      }

      add_assign(lhs_var, rhs_term);
      return false;
    }

    bool all_rhs_assigned = true;
    for (auto name : rhs_locals)
    {
      if (!is_assigned(name))
      {
        all_rhs_assigned = false;
      }
    }

    if (all_rhs_assigned)
    {
      if (lhs_assigned)
      {
        add_equals(lhs_var, rhs_term);
        return false;
      }

      add_assign(lhs_var, rhs_term);
      return false;
    }

    if (lhs_assigned)
    {
      return add_term_var(rhs_term, Var ^ lhs_var);
    }

    return true;
  }

  bool DependencyGraph::add_array_var(Node lhs_array, Node rhs_var)
  {
    assert(lhs_array && lhs_array == Array);
    assert(rhs_var && rhs_var == Var);
    Node size = (Int ^ std::to_string(lhs_array->size()));
    add_literal(ExprIsArray << rhs_var->clone() << size, {});
    for (size_t i = 0; i < lhs_array->size(); ++i)
    {
      Location arraydot_name = m_scope->fresh({"arraydot"});
      add_literal(Local << (Ident ^ arraydot_name), {});
      add_literal(
        ExprAssignFromArray << (AssignVar ^ arraydot_name) << rhs_var->clone()
                            << (Int ^ std::to_string(i)),
        {});
      bool fail = add_expr_var(lhs_array->at(i), Var ^ arraydot_name);
      assert(!fail);
    }

    return false;
  }

  bool DependencyGraph::add_array_array(Node lhs_array, Node rhs_array)
  {
    assert(lhs_array && lhs_array == Array);
    assert(rhs_array && rhs_array == Array);

    size_t size = lhs_array->size();
    if (size != rhs_array->size())
    {
      m_error = err(rhs_array, "Cannot unify: different size", EvalTypeError);
    }

    bool all_assigned = true;
    for (size_t i = 0; i < size; ++i)
    {
      Nodes lhs_vars = get_unifyvars(lhs_array->at(i));
      Nodes rhs_vars = get_unifyvars(rhs_array->at(i));
      if (any_unassigned(lhs_vars) || any_unassigned(rhs_vars))
      {
        all_assigned = false;
        break;
      }
    }

    if (all_assigned)
    {
      add_equals(lhs_array, rhs_array);
      return false;
    }

    for (size_t i = 0; i < size; ++i)
    {
      bool fail = add_exprs(lhs_array->at(i), rhs_array->at(i));
      assert(!fail);
    }

    return false;
  }

  bool DependencyGraph::add_object_var(Node lhs_object, Node rhs_var)
  {
    assert(lhs_object && lhs_object == Object);
    assert(rhs_var && rhs_var == Var);
    Node size = (Int ^ std::to_string(lhs_object->size()));
    add_literal(
      Literal << (ExprIsObject << rhs_var->clone() << size) << WithSeq, {});
    for (Node item : *lhs_object)
    {
      Location objectdot_name = m_scope->fresh({"objectdot"});
      add_literal(Local << (Ident ^ objectdot_name), {});
      add_literal(
        ExprAssignFromObject << (AssignVar ^ objectdot_name) << rhs_var->clone()
                             << (item / Key),
        {});
      bool fail = add_expr_var(item / Val, Var ^ objectdot_name);
      assert(!fail);
    }

    return false;
  }

  bool DependencyGraph::add_object_object(Node lhs_object, Node rhs_object)
  {
    assert(lhs_object && lhs_object == Object);
    assert(rhs_object && rhs_object == Object);

    size_t size = lhs_object->size();
    if (size != rhs_object->size())
    {
      m_error = err(rhs_object, "Cannot unify: different size", EvalTypeError);
    }

    std::map<std::string, Node> lhs_constkey_items;
    std::map<std::string, Node> lhs_varkey_items;
    std::map<std::string, Node> rhs_constkey_items;
    std::map<std::string, Node> rhs_varkey_items;
    bool lhs_varkeys = false;
    bool rhs_varkeys = false;
    bool all_assigned = true;

    for (size_t i = 0; i < size; ++i)
    {
      Node lhs_item = lhs_object->at(i);
      Nodes key_vars = get_unifyvars(lhs_item / Key);
      Nodes val_vars = get_unifyvars(lhs_item / Val);
      if (any_unassigned(key_vars))
      {
        // cannot unify object keys
        return true;
      }

      if (key_vars.empty())
      {
        // no Var nodes in the key
        std::string key_str;
        Node key_term = to_constant_term(lhs_item / Key);
        if (key_term)
        {
          // the key is a constant value, so we can add it to the dictionary
          key_str = to_key(key_term);
        }
        else
        {
          // TODO actually resolve the expression and use the resolved value
          key_str = std::string((lhs_item / Key)->location().view());
        }

        lhs_constkey_items[key_str] = lhs_item;
      }
      else
      {
        // Var nodes in the key
        lhs_varkeys = true;
        std::string key_str((lhs_item / Key)->location().view());
        lhs_varkey_items[key_str] = lhs_item;
      }

      if (any_unassigned(val_vars))
      {
        all_assigned = false;
      }

      // do the same analysis for rhs
      Node rhs_item = rhs_object->at(i);
      key_vars = get_unifyvars(rhs_item / Key);
      val_vars = get_unifyvars(rhs_item / Val);
      if (any_unassigned(key_vars))
      {
        return true;
      }

      if (key_vars.empty())
      {
        // no Var nodes in the key
        std::string key_str;
        Node key_term = to_constant_term(rhs_item / Key);
        if (key_term)
        {
          // the key is a constant value, so we can add it to the dictionary
          key_str = to_key(key_term);
        }
        else
        {
          // TODO actually resolve the expression and use the resolved value
          key_str = std::string((rhs_item / Key)->location().view());
        }

        rhs_constkey_items[key_str] = rhs_item;
      }
      else
      {
        // Var nodes in the key
        rhs_varkeys = true;
        std::string key_str((rhs_item / Key)->location().view());
        rhs_varkey_items[key_str] = rhs_item;
      }

      if (any_unassigned(val_vars))
      {
        all_assigned = false;
      }
    }

    if (all_assigned)
    {
      // no unassigned Var nodes, so we can just check for equality
      add_equals(lhs_object, rhs_object);
      return false;
    }

    Nodes lhs_unresolved;
    Nodes rhs_unresolved;

    for (auto& [key, item] : lhs_constkey_items)
    {
      auto it = rhs_constkey_items.find(key);
      if (it == rhs_constkey_items.end())
      {
        lhs_unresolved.push_back(item);
        continue;
      }

      bool fail = add_exprs(item / Val, it->second / Val);
      assert(!fail);
    }

    for (auto& [key, item] : lhs_varkey_items)
    {
      auto it = rhs_varkey_items.find(key);
      if (it == rhs_varkey_items.end())
      {
        lhs_unresolved.push_back(item);
        continue;
      }

      bool fail = add_exprs(item / Val, it->second / Val);
      assert(!fail);
    }

    for (auto& [key, item] : rhs_constkey_items)
    {
      auto it = rhs_constkey_items.find(key);
      if (it != rhs_constkey_items.end())
      {
        continue;
      }

      rhs_unresolved.push_back(item);
    }

    for (auto& [key, item] : rhs_varkey_items)
    {
      auto it = rhs_varkey_items.find(key);
      if (it != rhs_varkey_items.end())
      {
        continue;
      }

      rhs_unresolved.push_back(item);
    }

    return !lhs_unresolved.empty() || !rhs_unresolved.empty();
  }

  bool DependencyGraph::add_terms(Node lhs_term, Node rhs_term)
  {
    assert(lhs_term && lhs_term == Term);
    assert(rhs_term && rhs_term == Term);
    Node lhs = lhs_term->front();
    Node rhs = rhs_term->front();

    if (lhs == UnifyVar)
    {
      std::string lhs_name(lhs->location().view());
      if (rhs == UnifyVar)
      {
        std::string rhs_name(rhs->location().view());
        return add_unifyvars(lhs, lhs_name, rhs, rhs_name);
      }

      return add_unifyvar_term(lhs, lhs_name, rhs_term);
    }

    if (rhs == UnifyVar)
    {
      std::string rhs_name(rhs->location().view());
      return add_unifyvar_term(rhs, rhs_name, lhs_term);
    }

    if (lhs == Var)
    {
      return add_term_var(rhs_term, lhs);
    }

    if (lhs == Ref)
    {
      Location ref_name = m_scope->fresh({"ref"});
      add_assign(Var ^ ref_name, lhs);
      return add_term_var(rhs_term, Var ^ ref_name);
    }

    if (rhs == Var)
    {
      return add_term_var(lhs_term, rhs);
    }

    if (rhs == Ref)
    {
      Location ref_name = m_scope->fresh({"ref"});
      add_assign(Var ^ ref_name, rhs);
      return add_term_var(lhs_term, Var ^ ref_name);
    }

    if (lhs == Array)
    {
      if (rhs == Array)
      {
        return add_array_array(lhs, rhs);
      }

      if (rhs == ArrayCompr)
      {
        Location arraycompr_name = m_scope->fresh({"arraycompr"});
        add_assign(Var ^ arraycompr_name, rhs_term);
        return add_array_var(lhs, Var ^ arraycompr_name);
      }
    }

    if (rhs == Array && lhs == ArrayCompr)
    {
      Location arraycompr_name = m_scope->fresh({"arraycompr"});
      add_assign(Var ^ arraycompr_name, lhs_term);
      return add_array_var(rhs, Var ^ arraycompr_name);
    }

    if (lhs == Object)
    {
      if (rhs == Object)
      {
        return add_object_object(lhs, rhs);
      }

      if (rhs == ObjectCompr)
      {
        Location objectcompr_name = m_scope->fresh({"objectcompr"});
        add_assign(Var ^ objectcompr_name, rhs_term);
        return add_object_var(lhs, Var ^ objectcompr_name);
      }
    }

    if (rhs == Object && lhs == ObjectCompr)
    {
      Location objectcompr_name = m_scope->fresh({"objectcompr"});
      add_assign(Var ^ objectcompr_name, lhs_term);
      return add_object_var(rhs, Var ^ objectcompr_name);
    }

    // no unification can occur
    add_equals(lhs_term, rhs_term);
    return false;
  }

  bool DependencyGraph::add_term_var(Node lhs_term, Node rhs_var)
  {
    assert(lhs_term && lhs_term == Term);
    assert(rhs_var && rhs_var == Var);
    Node lhs = lhs_term->front();
    if (lhs == UnifyVar)
    {
      std::string lhs_name(lhs->location().view());
      if (is_assigned(lhs_name))
      {
        add_equals(lhs_term, rhs_var);
        return false;
      }

      add_assign(lhs, rhs_var);
      return false;
    }

    if (lhs->in({Var, Ref}))
    {
      add_equals(lhs_term, rhs_var);
      return false;
    }

    if (lhs == Scalar)
    {
      add_equals(lhs_term, rhs_var);
      return false;
    }

    if (lhs == Array)
    {
      return add_array_var(lhs, rhs_var);
    }

    if (lhs == Object)
    {
      return add_object_var(lhs, rhs_var);
    }

    logging::Warn() << "lhs: " << lhs;
    m_error = err(lhs, "Unsupported unification argument");
    return true;
  }

  bool DependencyGraph::add_expr_var(Node lhs_expr, Node rhs_var)
  {
    assert(lhs_expr && lhs_expr == Expr);
    assert(rhs_var && rhs_var == Var);
    Node lhs = lhs_expr->front();
    if (lhs == Term)
    {
      return add_term_var(lhs, rhs_var);
    }

    add_equals(lhs_expr, rhs_var);
    return false;
  }

  bool DependencyGraph::add_term_expr(Node lhs_term, Node rhs_expr)
  {
    assert(lhs_term && lhs_term == Term);
    assert(rhs_expr && rhs_expr == Expr);

    std::set<std::string> rhs_locals;
    add_locals(rhs_locals, rhs_expr);

    if (!rhs_locals.empty())
    {
      for (auto name : rhs_locals)
      {
        if (!is_assigned(name))
        {
          return true;
        }
      }
    }

    Node lhs = lhs_term->front();
    if (lhs == UnifyVar)
    {
      std::string lhs_name(lhs->location().view());
      bool lhs_assigned = is_assigned(lhs_name);

      if (lhs_assigned)
      {
        add_equals(lhs_term, rhs_expr);
        return false;
      }

      add_assign(lhs, rhs_expr);
      return false;
    }

    if (lhs == Var)
    {
      return add_expr_var(rhs_expr, lhs);
    }

    if (lhs == Ref)
    {
      Location ref_name = m_scope->fresh({"ref"});
      add_assign(Var ^ ref_name, lhs);
      return add_expr_var(rhs_expr, Var ^ ref_name);
    }

    if (lhs == Scalar)
    {
      add_equals(lhs_term, rhs_expr);
      return false;
    }

    if (lhs == Array)
    {
      Location expr = m_scope->fresh({"expr"});
      add_assign(Var ^ expr, rhs_expr);

      return add_array_var(lhs, Var ^ expr);
    }

    if (lhs == Object)
    {
      Location expr = m_scope->fresh({"expr"});
      add_assign(Var ^ expr, rhs_expr);

      return add_object_var(lhs, Var ^ expr);
    }

    logging::Warn() << "lhs: " << lhs;
    m_error = err(lhs, "Unsupported unify argument");
    return true;
  }

  bool DependencyGraph::add_exprs(Node lhs_expr, Node rhs_expr)
  {
    assert(lhs_expr && lhs_expr == Expr);
    assert(rhs_expr && rhs_expr == Expr);
    Node lhs = lhs_expr->front();
    Node rhs = rhs_expr->front();

    if (lhs == Term)
    {
      if (rhs == Term)
      {
        return add_terms(lhs, rhs);
      }

      return add_term_expr(lhs, rhs_expr);
    }

    if (rhs == Term)
    {
      return add_term_expr(rhs, lhs_expr);
    }

    logging::Warn() << "lhs: " << lhs << "rhs: " << rhs;
    m_error = err(lhs, "Unsupported unification argument");
    return true;
  }

  void DependencyGraph::resolve_unify_literals()
  {
    while (!m_unify_literals.empty())
    {
      size_t size = m_unify_literals.size();
      for (size_t i = 0; i < size; ++i)
      {
        Node current = m_unify_literals.front();
        m_unify_literals.pop_front();

        assert(current && current == ExprUnify);

        if (add_exprs(current / Lhs, current / Rhs))
        {
          m_unify_literals.push_back(current);
        }
      }

      if (size == m_unify_literals.size())
      {
        logging::Warn() << "Unable to unify the following " << size
                        << " expressions:";
        for (auto& n : m_unify_literals)
        {
          logging::Warn() << n;
        }

        m_error = err(m_unify_literals[0], "Unify cycle");
        return;
      }
    }
  }

  void DependencyGraph::add_captures()
  {
    for (auto& [name, local] : m_locals)
    {
      if (local.in_edge.has_value())
      {
        continue;
      }

      if (name.ends_with("#empty"))
      {
        std::string local_name = name.substr(0, name.size() - 6);
        if (
          local_name.starts_with("scanindex$") ||
          local_name.starts_with("scanvalue$"))
        {
          add_literal(ScanLocal << (Ident ^ local_name), {});
        }
        else
        {
          add_literal(Local << (Ident ^ local_name), {});
        }
      }
      else
      {
        m_captures.push_back(name);
        add_literal(Capture ^ name, {});
      }
    }
  }

  void DependencyGraph::find_graphs()
  {
    std::vector<size_t> xs(m_literals.size());
    std::iota(xs.begin(), xs.end(), 0);

    std::set<size_t> visited;
    m_graphs.clear();

    while (!xs.empty())
    {
      Subgraph graph;
      size_t start = xs.back();
      xs.pop_back();
      if (visited.find(start) != visited.end())
      {
        continue;
      }

      std::vector<size_t> frontier{start};
      while (!frontier.empty())
      {
        size_t current = frontier.back();
        frontier.pop_back();

        if (graph.indices.find(current) != graph.indices.end())
        {
          continue;
        }

        const LiteralNode& literal = m_literals[current];
        graph.indices.insert(current);
        for (auto& name : literal.in_edges)
        {
          frontier.insert(
            frontier.end(),
            m_locals[name].out_edges.begin(),
            m_locals[name].out_edges.end());
          frontier.push_back(m_locals[name].in_edge.value());
          graph.in_edges.insert(name);
        }

        for (auto& name : literal.out_edges)
        {
          graph.out_edges.insert(name);
          frontier.insert(
            frontier.end(),
            m_locals[name].out_edges.begin(),
            m_locals[name].out_edges.end());
        }
      }

      std::set<std::string> in_edges;
      std::set<std::string> out_edges;
      std::set_difference(
        graph.in_edges.begin(),
        graph.in_edges.end(),
        graph.out_edges.begin(),
        graph.out_edges.end(),
        std::inserter(in_edges, in_edges.begin()));
      std::set_difference(
        graph.out_edges.begin(),
        graph.out_edges.end(),
        graph.in_edges.begin(),
        graph.in_edges.end(),
        std::inserter(out_edges, out_edges.begin()));
      graph.in_edges = in_edges;
      graph.out_edges = out_edges;

      visited.insert(graph.indices.begin(), graph.indices.end());
      m_graphs.push_back(graph);
    }

    if (m_graphs.size() > 1)
    {
      std::sort(
        m_graphs.begin(),
        m_graphs.end(),
        [](const Subgraph& a, const Subgraph& b) {
          return a.out_edges.size() < b.out_edges.size();
        });
    }
  }

  void DependencyGraph::topological_sort(const std::set<size_t>& graph)
  {
    std::deque<size_t> frontier;
    for (size_t start : graph)
    {
      if (m_literals[start].in_edges.empty())
      {
        frontier.push_back(start);
      }
    }

    Node current = m_orderedseq;
    while (!current->empty() && current->back() / Expr == ExprScan)
    {
      current = (current->back() / Expr)->back();
    }

    std::set<std::string> visited_locals;
    std::set<size_t> visited_literals;
    while (!frontier.empty())
    {
      size_t index = frontier.front();
      frontier.pop_front();

      if (visited_literals.find(index) != visited_literals.end())
      {
        continue;
      }

      visited_literals.insert(index);

      LiteralNode& node = m_literals[index];
      Node expr = node.literal->front();

      if (!expr->in(
            {Local, ScanLocal, RuleLocal, EveryLocal, Capture, StaticIsObject}))
      {
        current << node.literal;
      }

      if (expr == ExprScan)
      {
        current = NodeDef::create(Query);
        expr << current;
      }

      if (node.out_edges.empty())
      {
        // terminal node
        continue;
      }

      visited_locals.insert(node.out_edges.begin(), node.out_edges.end());

      for (const auto& name : node.out_edges)
      {
        LocalNode& local = m_locals[name];
        for (size_t next : local.out_edges)
        {
          const auto& in_edges = m_literals[next].in_edges;
          if (std::includes(
                visited_locals.begin(),
                visited_locals.end(),
                in_edges.begin(),
                in_edges.end()))
          {
            frontier.push_back(next);
          }
        }
      }
    }

    if (current->empty())
    {
      current
        << (Literal << (Expr << (Term << (Scalar << (True ^ "true"))))
                    << WithSeq);
    }
  }

  void DependencyGraph::add_rule_locals()
  {
    Node localseq = m_scope / LocalSeq;
    for (Node local : *localseq)
    {
      Node ident = local->front()->clone();
      if (local == EveryLocal)
      {
        add_literal(local->clone(), local->location());
      }
      else
      {
        add_literal(RuleLocal << ident, local->location());
      }
    }
  }

  void DependencyGraph::add_unify_literals()
  {
    for (Node literal : m_nodes)
    {
      assert(literal == Literal);
      if (literal->front() == ExprUnify)
      {
        m_needs_sort = true;
        m_unify_literals.push_back(literal->front());
        continue;
      }
      else if (literal->front() == ExprScan)
      {
        m_needs_sort = true;
      }

      add_literal(literal, literal->location());
    }
  }

  DependencyGraph::DependencyGraph(Node scope, const NodeRange& nodes) :
    m_scope(scope), m_nodes(nodes.begin(), nodes.end())
  {
    add_rule_locals();
    add_unify_literals();
    resolve_unify_literals();
    add_captures();
    find_graphs();
  }

  Node DependencyGraph::sort()
  {
    if (!m_needs_sort)
    {
      m_orderedseq = Seq << m_nodes;
      return nullptr;
    }

    if (!m_unify_literals.empty())
    {
      Node errorseq = NodeDef::create(Seq);
      for (Node node : m_unify_literals)
      {
        errorseq << err(node, "Unable to unify due to cycle");
      }

      return errorseq;
    }

    if (logging::Trace().active())
    {
      log();
    }

    m_orderedseq = NodeDef::create(Seq);
    for (const auto& graph : m_graphs)
    {
      topological_sort(graph.indices);
    }

    return nullptr;
  }

  void DependencyGraph::log() const
  {
    logging::Trace() << "```mermaid";
    logging::Trace() << "flowchart TD";
    {
      logging::LocalIndent indent;
      for (size_t i = 0; i < m_graphs.size(); ++i)
      {
        if (m_graphs.size() > 1)
        {
          logging::Trace() << "subgraph g" << i;
        }

        std::set<std::string> locals;
        for (size_t index : m_graphs[i].indices)
        {
          const LiteralNode& node = m_literals[index];
          logging::Trace() << "stmt" << node.index << "{{\"" << node.str()
                           << "\"}}";
          for (auto& name : node.in_edges)
          {
            logging::Trace() << name << " --> " << "stmt" << node.index;
            locals.insert(name);
          }

          for (auto& name : node.out_edges)
          {
            logging::Trace() << "stmt" << node.index << " --> " << name;
            locals.insert(name);
          }
        }

        for (auto& name : locals)
        {
          logging::Trace() << name << "([" << name << "])";
        }

        if (m_graphs.size() > 1)
        {
          logging::Trace() << "end";
        }
      }
    }
    logging::Trace() << "```";
  }

  void DependencyGraph::merge_locals(std::set<std::string>& locals) const
  {
    for (auto [key, local] : m_locals)
    {
      if (
        local.captured || key.ends_with("#empty") ||
        key.starts_with("scanindex$") || key.starts_with("scanvalue$"))
      {
        continue;
      }

      locals.insert(key);
    }
  }

  Node DependencyGraph::orderedseq() const
  {
    return m_orderedseq;
  }

  const std::vector<std::string>& DependencyGraph::captures() const
  {
    return m_captures;
  }
}
