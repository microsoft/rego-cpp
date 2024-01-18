#include "internal.hh"

#include <algorithm>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  struct InitInfo
  {
    std::size_t index;
    bool strict;
    std::set<Location> lhs;
    std::set<Location> rhs;
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

  std::size_t count_locals(Node node, const std::set<Location>& locals)
  {
    if (node->type() == Var && locals.contains(node->location()))
    {
      return 1;
    }

    std::size_t count = 0;
    for (Node child : *node)
    {
      count += count_locals(child, locals);
    }

    return count;
  }

  void find_init_stmts(Node unifybody, std::set<Location>& locals)
  {
    // find the assignment statement for each local which
    // contains the fewest number of local variables. We'll
    // use this as that variable's init statement.
    std::map<Location, InitInfo> init_stmt_for_local;
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

        // find all the variables in the expression
        Node lhs = expr->front();
        Node rhs = expr->back();
        std::set<Location> lhs_vars;
        vars_from(lhs, lhs_vars);

        std::set<Location> rhs_vars;
        vars_from(rhs, rhs_vars);

        // filter to just the local variables
        std::set<Location> lhs_found;
        std::set_intersection(
          lhs_vars.begin(),
          lhs_vars.end(),
          locals.begin(),
          locals.end(),
          std::inserter(lhs_found, lhs_found.begin()));

        std::set<Location> rhs_found;
        std::set_intersection(
          rhs_vars.begin(),
          rhs_vars.end(),
          locals.begin(),
          locals.end(),
          std::inserter(rhs_found, rhs_found.begin()));

        // we want to give preference to "strict" assignment statements,
        // that is those which have no variables on one side of the
        // assignment.
        std::size_t lhs_count = count_locals(lhs, locals);
        std::size_t rhs_count = count_locals(rhs, locals);
        bool strict_assign = lhs_count == 0 || rhs_count == 0;

        // compiler statements will never be right-assign, so we can
        // use this fact later to help resolve some ambiguities
        bool compiler_stmt =
          std::any_of(lhs_found.begin(), lhs_found.end(), [](auto& loc) {
            std::string name = loc.str();
            return starts_with(name, "unify$") || starts_with(name, "out$") ||
              starts_with(name, "value$");
          });

        for (auto& loc : lhs_found)
        {
          if (init_stmt_for_local.contains(loc) && !strict_assign)
          {
            continue;
          }

          if (compiler_stmt)
          {
            init_stmt_for_local[loc] = {
              i, strict_assign, lhs_found, std::set<Location>{}};
          }
          else
          {
            init_stmt_for_local[loc] = {i, strict_assign, lhs_found, rhs_found};
          }
        }

        if (!compiler_stmt)
        {
          for (auto& loc : rhs_found)
          {
            if (init_stmt_for_local.contains(loc) && !strict_assign)
            {
              continue;
            }

            init_stmt_for_local[loc] = {i, strict_assign, lhs_found, rhs_found};
          }
        }
      }
    }

    // transform the init statements into LiteralInit nodes
    std::vector<InitInfo> init_stmts;
    std::transform(
      init_stmt_for_local.begin(),
      init_stmt_for_local.end(),
      std::back_inserter(init_stmts),
      [](auto& pair) { return pair.second; });

    // sort the init statements so that statements with no local variables
    // on one side come first.
    std::sort(init_stmts.begin(), init_stmts.end(), [](auto& a, auto& b) {
      if (a.strict)
      {
        if (b.strict)
        {
          return a.index < b.index;
        }
        else
        {
          return true;
        }
      }
      else
      {
        if (b.strict)
        {
          return false;
        }
        else
        {
          return a.index < b.index;
        }
      }
    });

    for (std::size_t i = 0; i < init_stmts.size(); ++i)
    {
      InitInfo& init_stmt = init_stmts[i];
      if (i > 0 && init_stmts[i].index == init_stmts[i - 1].index)
      {
        // this statement is a duplicate of the previous one
        continue;
      }

      Node expr = unifybody->at(init_stmt.index)->front()->front();
      if (expr->type() != AssignInfix)
      {
        continue;
      }

      Node lhs = expr->front();
      Node rhs = expr->back();

      std::set<Location> lhs_vars;
      std::set_intersection(
        init_stmt.lhs.begin(),
        init_stmt.lhs.end(),
        locals.begin(),
        locals.end(),
        std::inserter(lhs_vars, lhs_vars.begin()));

      std::set<Location> rhs_vars;
      std::set_intersection(
        init_stmt.rhs.begin(),
        init_stmt.rhs.end(),
        locals.begin(),
        locals.end(),
        std::inserter(rhs_vars, rhs_vars.begin()));

      // as this is the best init statement for these variables
      // we can remove them from the set of locals
      for (auto& loc : lhs_vars)
      {
        locals.erase(loc);
      }

      for (auto& loc : rhs_vars)
      {
        locals.erase(loc);
      }

      unifybody->replace_at(
        init_stmt.index, to_init(lhs, lhs_vars, rhs, rhs_vars));
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