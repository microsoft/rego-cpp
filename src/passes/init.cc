#include "internal.hh"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  struct InitInfo
  {
    std::set<Location> lhs_vars;
    std::set<Location> rhs_vars;
  };

  Node to_init(
    const Node& lhs,
    const std::set<Location>& lhs_locs,
    const Node& rhs,
    const std::set<Location>& rhs_locs)
  {
    Node lhs_vars = NodeDef::create(VarSeq);
    for (auto& loc : lhs_locs)
    {
      lhs_vars << (Var ^ loc);
    }

    Node rhs_vars = NodeDef::create(VarSeq);
    for (auto& loc : rhs_locs)
    {
      rhs_vars << (Var ^ loc);
    }

    if (rhs_vars->size() > 0 && lhs_vars->size() == 0)
    {
      return LiteralInit << rhs_vars << lhs_vars << (AssignInfix << rhs << lhs);
    }

    return LiteralInit << lhs_vars << rhs_vars << (AssignInfix << lhs << rhs);
  }

  void vars_from(Node node, std::set<Location>& vars)
  {
    if (node->type() == Var)
    {
      vars.insert(node->location());
      return;
    }

    if (node->type() == ObjectItem)
    {
      node = node / Val;
    }

    if (node->type() == SimpleRef)
    {
      node = node / Op;
    }

    if (node->type() == RefArgBrack)
    {
      node = node->front();
    }

    if (node->type() == Expr)
    {
      node = node->front();
    }

    if (node->type() == Term)
    {
      node = node->front();
    }

    std::set<Token> include_parents = {AssignArg, RefTerm, Object, Array};
    if (!contains(include_parents, node->type()))
    {
      return;
    }

    for (Node child : *node)
    {
      vars_from(child, vars);
    }
  }

  void find_init_stmts(Node unifybody, std::set<Location>& locals)
  {
    // gather all locals
    for (std::size_t i = 0; i < unifybody->size(); ++i)
    {
      Node stmt = unifybody->at(i);
      if (stmt->type() == Local)
      {
        locals.insert((stmt / Var)->location());
      }
      else if (stmt->type() == LiteralEnum)
      {
        locals.erase((stmt / Item)->location());
        find_init_stmts(stmt / UnifyBody, locals);
      }
      else if (stmt->type() == LiteralWith)
      {
        find_init_stmts(stmt / UnifyBody, locals);
      }
      else if (stmt->type() == LiteralNot)
      {
        find_init_stmts(stmt / UnifyBody, locals);
      }
      else if (stmt->type() == Literal)
      {
        Node expr = stmt->front()->front();
        if (expr->type() != AssignInfix)
        {
          continue;
        }

        Node lhs = expr->front();
        Node rhs = expr->back();
        std::set<Location> lhs_vars;
        vars_from(lhs, lhs_vars);
        std::set<Location> lhs_found;
        std::set_intersection(
          lhs_vars.begin(),
          lhs_vars.end(),
          locals.begin(),
          locals.end(),
          std::inserter(lhs_found, lhs_found.begin()));

        std::set<Location> rhs_vars;
        vars_from(rhs, rhs_vars);
        std::set<Location> rhs_found;
        std::set_intersection(
          rhs_vars.begin(),
          rhs_vars.end(),
          locals.begin(),
          locals.end(),
          std::inserter(rhs_found, rhs_found.begin()));

        if (lhs_found.empty() && rhs_found.empty())
        {
          continue;
        }

        for (auto& loc : lhs_found)
        {
          locals.erase(loc);
        }

        for (auto& loc : rhs_found)
        {
          locals.erase(loc);
        }

        unifybody->replace_at(i, to_init(lhs, lhs_found, rhs, rhs_found));
      }
    }
  }

  void register_init_stmts(const Node& rule)
  {
    Node body = rule / Body;
    Node val = rule / Val;

    if (body->type() == UnifyBody)
    {
      std::set<Location> locals;
      find_init_stmts(body, locals);
    }

    if (val->type() == UnifyBody)
    {
      std::set<Location> locals;
      find_init_stmts(val, locals);
    }
  }
}

namespace rego
{
  using namespace wf::ops;

  // This pass performs initialization analysis on the program and finds
  // all assignments which initialize a variable (as opposed to unifying it
  // with an additional result). It wraps these in special LiteralInit nodes,
  // which record which variables on both sides of the assignment are being
  // initialized. These allow later passes to correctly handle these statements.
  PassDef init()
  {
    PassDef init = {"init", wf_pass_init, dir::once | dir::bottomup};

    init.pre(RuleComp, [](Node rule) {
      register_init_stmts(rule);
      return 0;
    });

    init.pre(RuleFunc, [](Node rule) {
      register_init_stmts(rule);
      return 0;
    });

    init.pre(RuleSet, [](Node rule) {
      register_init_stmts(rule);
      return 0;
    });

    init.pre(RuleObj, [](Node rule) {
      register_init_stmts(rule);
      return 0;
    });

    init.pre(NestedBody, [](Node nested) {
      std::set<Location> locals;
      find_init_stmts(nested / Val, locals);
      return 0;
    });

    init.pre(ExprEvery, [](Node exprevery) {
      std::set<Location> locals;
      find_init_stmts(exprevery / UnifyBody, locals);
      return 0;
    });

    return init;
  }
}