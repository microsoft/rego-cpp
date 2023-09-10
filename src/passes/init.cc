#include "internal.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  struct InitInfo
  {
    std::set<Location> lhs_vars;
    std::set<Location> rhs_vars;
  };

  using InitStatments = std::shared_ptr<NodeMap<InitInfo>>;

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
    if (!include_parents.contains(node->type()))
    {
      return;
    }

    for (Node child : *node)
    {
      vars_from(child, vars);
    }
  }

  void find_init_stmts(
    Node unifybody, std::set<Location>& locals, InitStatments init_stmts)
  {
    // gather all locals
    for (Node stmt : *unifybody)
    {
      if (stmt->type() == Local)
      {
        locals.insert((stmt / Var)->location());
      }
      else if (stmt->type() == LiteralEnum)
      {
        locals.erase((stmt / Item)->location());
        find_init_stmts(stmt / UnifyBody, locals, init_stmts);
      }
      else if (stmt->type() == LiteralWith)
      {
        find_init_stmts(stmt / UnifyBody, locals, init_stmts);
      }
      else if (stmt->type() == LiteralNot)
      {
        find_init_stmts(stmt / UnifyBody, locals, init_stmts);
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

        init_stmts->insert({stmt, InitInfo{lhs_found, rhs_found}});
      }
    }
  }

  void register_init_stmts(const Node& rule, InitStatments init_stmts)
  {
    Node body = rule / Body;
    Node val = rule / Val;

    if (body->type() == UnifyBody)
    {
      std::set<Location> locals;
      find_init_stmts(body, locals, init_stmts);
    }

    if (val->type() == UnifyBody)
    {
      std::set<Location> locals;
      find_init_stmts(val, locals, init_stmts);
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
    InitStatments init_stmts = std::make_shared<NodeMap<InitInfo>>();

    PassDef init = {
      dir::once | dir::bottomup,
      {
        In(UnifyBody) *
            (T(Literal)[Literal](
               [init_stmts](auto& n) { return init_stmts->contains(*n.first); })
             << (T(Expr)
                 << (T(AssignInfix)
                     << (T(AssignArg)[Lhs] * T(AssignArg)[Rhs])))) >>
          [init_stmts](Match& _) {
            InitInfo& init_info = init_stmts->at(_(Literal));

            Node lhs_vars = NodeDef::create(VarSeq);
            for (auto& loc : init_info.lhs_vars)
            {
              lhs_vars << (Var ^ loc);
            }

            Node rhs_vars = NodeDef::create(VarSeq);
            for (auto& loc : init_info.rhs_vars)
            {
              rhs_vars << (Var ^ loc);
            }

            if (rhs_vars->size() > 0 && lhs_vars->size() == 0)
            {
              return LiteralInit << rhs_vars << lhs_vars
                                 << (AssignInfix << _(Rhs) << _(Lhs));
            }

            return LiteralInit << lhs_vars << rhs_vars
                               << (AssignInfix << _(Lhs) << _(Rhs));
          },
      }};

    init.pre(RuleComp, [init_stmts](Node rule) {
      register_init_stmts(rule, init_stmts);
      return 0;
    });

    init.pre(RuleFunc, [init_stmts](Node rule) {
      register_init_stmts(rule, init_stmts);
      return 0;
    });

    init.pre(RuleSet, [init_stmts](Node rule) {
      register_init_stmts(rule, init_stmts);
      return 0;
    });

    init.pre(RuleObj, [init_stmts](Node rule) {
      register_init_stmts(rule, init_stmts);
      return 0;
    });

    init.pre(NestedBody, [init_stmts](Node nested) {
      std::set<Location> locals;
      find_init_stmts(nested / Val, locals, init_stmts);
      return 0;
    });

    init.pre(ExprEvery, [init_stmts](Node exprevery) {
      std::set<Location> locals;
      find_init_stmts(exprevery / UnifyBody, locals, init_stmts);
      return 0;
    });

    return init;
  }
}