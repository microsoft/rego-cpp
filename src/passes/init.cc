#include "internal.hh"

#include <algorithm>
#include <cstddef>
#include <deque>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  struct InitSide
  {
    std::set<Location> vars;
    std::set<Location> inits;
  };

  struct InitInfo
  {
    std::size_t index;
    InitSide lhs;
    InitSide rhs;
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

  void inits_from(
    Node node, const std::set<Location>& locals, std::set<Location>& inits)
  {
    if (node->type() == Var && contains(locals, node->location()))
    {
      inits.insert(node->location());
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
      inits_from(child, locals, inits);
    }
  }

  void vars_from(
    Node node, const std::set<Location>& locals, std::set<Location>& vars)
  {
    if (node->type() == Var && contains(locals, node->location()))
    {
      vars.insert(node->location());
    }

    for (Node child : *node)
    {
      vars_from(child, locals, vars);
    }
  }

  InitSide side_from(Node node, const std::set<Location>& locals)
  {
    InitSide side;
    inits_from(node, locals, side.inits);
    vars_from(node, locals, side.vars);
    return side;
  }

  bool any_compiler_inits(const InitSide& lhs)
  {
    return std::any_of(lhs.inits.begin(), lhs.inits.end(), [](auto& loc) {
      std::string name = loc.str();
      return starts_with(name, "unify$") || starts_with(name, "out$") ||
        starts_with(name, "value$");
    });
  }

  void remove_locals(
    std::deque<InitInfo>& init_deque, const std::set<Location>& to_remove)
  {
    std::size_t count = init_deque.size();
    for (std::size_t i = 0; i < count; ++i)
    {
      InitInfo& init_stmt = init_deque.front();
      for (auto& loc : to_remove)
      {
        init_stmt.lhs.vars.erase(loc);
        init_stmt.lhs.inits.erase(loc);
        init_stmt.rhs.vars.erase(loc);
        init_stmt.rhs.inits.erase(loc);
      }
      if (!init_stmt.lhs.inits.empty() || !init_stmt.rhs.inits.empty())
      {
        init_deque.push_back(init_stmt);
      }

      init_deque.pop_front();
    }
  }

  std::vector<InitInfo> sort_init_stmts(
    const std::set<Location>& locals, std::deque<InitInfo>& init_deque)
  {
    std::set<Location> initialized;
    std::vector<InitInfo> init_stmts;
    while (!init_deque.empty() && initialized != locals)
    {
      // find all strict init statements
      auto it =
        std::find_if(init_deque.begin(), init_deque.end(), [](auto& init_stmt) {
          return init_stmt.lhs.vars.empty() || init_stmt.rhs.vars.empty();
        });

      if (it == init_deque.end())
      {
        // we have a cycle, so we use the first statement
        it = init_deque.begin();
        init_stmts.push_back(*it);
      }
      else
      {
        init_stmts.push_back(*it);
      }

      std::set<Location> to_remove;
      to_remove.insert(it->lhs.inits.begin(), it->lhs.inits.end());
      to_remove.insert(it->rhs.inits.begin(), it->rhs.inits.end());
      init_deque.erase(it);
      remove_locals(init_deque, to_remove);
      initialized.insert(to_remove.begin(), to_remove.end());
    }

    return init_stmts;
  }

  void find_init_stmts(Node unifybody, std::set<Location>& locals)
  {
    std::deque<InitInfo> potential_init_stmts;
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

        InitSide lhs_side = side_from(lhs, locals);
        InitSide rhs_side = side_from(rhs, locals);

        if (any_compiler_inits(lhs_side))
        {
          // compiler statements will never be right-assign, so we can
          // use this fact later to help resolve some ambiguities
          rhs_side.inits.clear();
        }

        if (lhs_side.inits.empty() && rhs_side.inits.empty())
        {
          continue;
        }

        potential_init_stmts.push_back({i, lhs_side, rhs_side});
      }
    }

    std::vector<InitInfo> init_stmts =
      sort_init_stmts(locals, potential_init_stmts);
    for (std::size_t i = 0; i < init_stmts.size(); ++i)
    {
      InitInfo& init_stmt = init_stmts[i];
      Node expr = unifybody->at(init_stmt.index)->front()->front();

      Node lhs = expr->front();
      Node rhs = expr->back();

      for (auto& loc : init_stmt.lhs.inits)
      {
        locals.erase(loc);
      }

      for (auto& loc : init_stmt.rhs.inits)
      {
        locals.erase(loc);
      }

      unifybody->replace_at(
        init_stmt.index,
        to_init(lhs, init_stmt.lhs.inits, rhs, init_stmt.rhs.inits));
    }

    // where appropriate, recurse with the updated locals
    for (Node stmt : *unifybody)
    {
      if (stmt->type() == LiteralEnum)
      {
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