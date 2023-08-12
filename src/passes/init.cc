#include "lang.h"
#include "log.h"
#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  using InitStatments = std::shared_ptr<NodeMap<Location>>;

  void vars_from(Node node, std::set<Location>& vars)
  {
    std::set<Token> exclude_parents = {RefArgDot, VarSeq, UnifyBody};
    if (exclude_parents.contains(node->type()))
    {
      return;
    }

    if (node->type() == Var)
    {
      vars.insert(node->location());
    }
    else
    {
      for (Node child : *node)
      {
        vars_from(child, vars);
      }
    }
  }

  void find_init_stmts(Node unifybody, InitStatments init_stmts)
  {
    std::set<Location> locals;
    std::set<Location> initialized;
    for (Node stmt : *unifybody)
    {
      if (stmt->type() == Local)
      {
        locals.insert((stmt / Var)->location());
      }
      else if (stmt->type() == Literal)
      {
        std::set<Location> vars;
        vars_from(stmt, vars);
        std::set<Location> found;
        std::set_intersection(
          vars.begin(),
          vars.end(),
          locals.begin(),
          locals.end(),
          std::inserter(found, found.begin()));
        if (found.size() == 1)
        {
          Location loc = *found.begin();
          initialized.insert(loc);
          locals.erase(loc);
          init_stmts->insert({stmt, loc});
        }
      }
    }
  }

  bool contains_loc(const Node& node, const Location& loc)
  {
    std::set<Token> exclude_parents = {RefArgDot, VarSeq, UnifyBody};
    if (exclude_parents.contains(node->type()))
    {
      return false;
    }

    if (node->type() == Var)
    {
      return node->location() == loc;
    }
    else
    {
      for (Node child : *node)
      {
        if (contains_loc(child, loc))
        {
          return true;
        }
      }
      return false;
    }
  }
}

namespace rego
{
  using namespace wf::ops;

  // Convert all Literal nodes into UnifyExpr nodes of the form <var> = <expr> |
  // <not-expr>.
  PassDef init()
  {
    InitStatments init_stmts = std::make_shared<NodeMap<Location>>();

    PassDef init = {
      dir::once | dir::topdown,
      {
        In(UnifyBody) *
            (T(Literal)[Literal](
               [init_stmts](auto& n) { return init_stmts->contains(*n.first); })
             << (T(Expr)
                 << (T(AssignInfix)
                     << (T(AssignArg)[Lhs] * T(AssignArg)[Rhs])))) >>
          [init_stmts](Match& _) {
            Location init_var = init_stmts->at(_(Literal));
            if (contains_loc(_(Lhs), init_var))
            {
              return LiteralInit << (AssignInfix << _(Lhs) << _(Rhs));
            }
            else
            {
              return LiteralInit << (AssignInfix << _(Rhs) << _(Lhs));
            }
          },
      }};

    init.pre(UnifyBody, [init_stmts](Node unifybody) {
      find_init_stmts(unifybody, init_stmts);
      return 0;
    });

    return init;
  }
}