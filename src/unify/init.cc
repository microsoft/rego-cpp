#include "unify.hh"

#include <algorithm>
#include <cstddef>
#include <list>

namespace
{
  using namespace rego;
  using namespace wf::ops;

  struct StmtSide
  {
    std::set<Location> vars;
    std::set<Location> inits;
  };

  std::ostream& operator<<(std::ostream& os, const StmtSide& side)
  {
    os << "[{";
    join(
      os,
      side.inits.begin(),
      side.inits.end(),
      ",",
      [](auto& stream, auto& loc) {
        stream << loc.view();
        return true;
      });
    os << "} < {";
    join(
      os, side.vars.begin(), side.vars.end(), ",", [](auto& stream, auto& loc) {
        stream << loc.view();
        return true;
      });
    return os << "}]";
  }

  struct StmtInfo
  {
    std::size_t index;
    StmtSide lhs;
    StmtSide rhs;

    bool remove(const Location& loc)
    {
      bool removed = lhs.vars.erase(loc) > 0;
      removed |= lhs.inits.erase(loc) > 0;
      removed |= rhs.vars.erase(loc) > 0;
      removed |= rhs.inits.erase(loc) > 0;
      return removed;
    }

    bool is_strict_init() const
    {
      return (!lhs.inits.empty() && rhs.vars.empty()) ||
        (!rhs.inits.empty() && lhs.vars.empty());
    }

    bool is_init() const
    {
      return !lhs.inits.empty() || !rhs.inits.empty();
    }
  };

  std::ostream& operator<<(std::ostream& os, const StmtInfo& info)
  {
    return os << info.index << ": " << info.lhs << " / " << info.rhs;
  }

  struct Locals
  {
    std::vector<std::map<Location, bool>> locals;

    void insert(const Location& name, bool add_local)
    {
      locals.back()[name] = add_local;
    }

    void erase(const Location& name)
    {
      locals.back().erase(name);
    }

    bool contains(const Location& name) const
    {
      return std::any_of(locals.rbegin(), locals.rend(), [name](auto& m) {
        return ::contains(m, name);
      });
    }

    void add_missing_locals(
      Node unifybody,
      std::vector<StmtInfo>& stmts,
      std::map<Location, std::size_t>& local_stmts)
    {
      for (auto& [loc, add] : locals.back())
      {
        if (add)
        {
          std::size_t i = unifybody->size();
          unifybody << (Local << (Var ^ loc) << Undefined);
          local_stmts[loc] = i;
          stmts.push_back({i, {}, {}});
        }
      }
    }

    void push()
    {
      locals.push_back({});
    }

    void pop()
    {
      locals.pop_back();
    }
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
    Node node,
    Locals& locals,
    std::set<Location>& inits,
    const std::set<Location>& outer_inits)
  {
    if (node == Var)
    {
      if (locals.contains(node->location()))
      {
        inits.insert(node->location());
      }

      if (contains(outer_inits, node->location()))
      {
        locals.insert(node->location(), true);
        inits.insert(node->location());
      }

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

    if (!node->in({AssignArg, RefTerm, Object, Array}))
    {
      return;
    }

    for (Node child : *node)
    {
      inits_from(child, locals, inits, outer_inits);
    }
  }

  void vars_from(Node node, const Locals& locals, std::set<Location>& vars)
  {
    if (node->type() == Var && locals.contains(node->location()))
    {
      vars.insert(node->location());
    }

    if (node == RefArgDot)
    {
      return;
    }

    for (Node child : *node)
    {
      vars_from(child, locals, vars);
    }
  }

  StmtSide side_from(
    Node node, Locals& locals, const std::set<Location>& outer_inits)
  {
    StmtSide side;
    inits_from(node, locals, side.inits, outer_inits);
    vars_from(node, locals, side.vars);
    return side;
  }

  bool any_compiler_inits(const StmtSide& lhs)
  {
    return std::any_of(lhs.inits.begin(), lhs.inits.end(), [](auto& loc) {
      std::string name = loc.str();
      return starts_with(name, "unify$") || starts_with(name, "out$") ||
        starts_with(name, "value$");
    });
  }

  void find_init_stmts(
    Node unifybody, Locals& locals, const std::set<Location>& outer_inits)
  {
    logging::Trace() << std::endl
                     << "Finding init statements and re-ordering body";
    locals.push();
    // Our ultimate goal is a re-ordering of statements such that all init
    // statements come before the statements which depend on them
    std::vector<StmtInfo> stmts;
    std::map<Location, std::size_t> local_stmts;
    for (std::size_t i = 0; i < unifybody->size(); ++i)
    {
      Node stmt = unifybody->at(i);
      if (stmt->type() == Local)
      {
        // this is a local for this context
        // as such it forms a starting point for the graph
        Location loc = (stmt / Var)->location();
        locals.insert(loc, false);
        local_stmts[loc] = i;
        stmts.push_back({i, {}, {}});
        logging::Trace() << "Local: " << loc.view();
      }
      else if (stmt->type() == LiteralEnum)
      {
        // a literal enum initializes its item from the item sequence, which
        // cannot be an init statement. The item variable is only
        // used inside the enum, so we can safely ignore it.
        Location item_loc = (stmt / Item)->location();
        locals.erase(item_loc);
        StmtSide rhs;
        vars_from(stmt / ItemSeq, locals, rhs.vars);
        stmts.push_back({i, {}, rhs});
      }
      else if (stmt->type() == Literal)
      {
        Node expr = stmt->front()->front();
        if (expr->type() != AssignInfix)
        {
          // no possibility of assignment
          // store this for later ordering
          StmtSide rhs;
          vars_from(stmt->front(), locals, rhs.vars);
          stmts.push_back({i, {}, rhs});
          continue;
        }

        Node lhs = expr->front();
        Node rhs = expr->back();

        StmtSide lhs_side = side_from(lhs, locals, outer_inits);
        StmtSide rhs_side = side_from(rhs, locals, outer_inits);

        if (any_compiler_inits(lhs_side))
        {
          // compiler statements will never be right-assign, so we can
          // use this fact later to help resolve some ambiguities
          rhs_side.inits.clear();
        }

        stmts.push_back({i, lhs_side, rhs_side});
      }
      else
      {
        StmtSide rhs;
        vars_from(stmt->front(), locals, rhs.vars);
        stmts.push_back({i, {}, rhs});
      }
    }

    locals.add_missing_locals(unifybody, stmts, local_stmts);

    for (auto& stmt : stmts)
    {
      logging::Trace() << stmt;
    }

    // build the graph
    std::list<size_t> remaining(stmts.size());
    std::iota(remaining.begin(), remaining.end(), 0);
    std::vector<std::set<size_t>> edges(stmts.size());
    while (!remaining.empty())
    {
      // find the first strict init statement
      auto it =
        std::find_if(remaining.begin(), remaining.end(), [stmts](auto& i) {
          return stmts[i].is_strict_init();
        });

      if (it == remaining.end())
      {
        // there is a loop, so break the tie using statement order
        it = std::find_if(remaining.begin(), remaining.end(), [stmts](auto& i) {
          return stmts[i].is_init();
        });

        if (it == remaining.end())
        {
          // no init statements left
          break;
        }
      }

      size_t index = *it;
      remaining.erase(it);

      StmtInfo& stmt = stmts[index];
      std::set<Location> init;
      init.insert(stmt.lhs.inits.begin(), stmt.lhs.inits.end());
      init.insert(stmt.rhs.inits.begin(), stmt.rhs.inits.end());
      for (auto& loc : init)
      {
        logging::Trace() << "removing init " << loc.view();
        edges[stmt.index].insert(local_stmts[loc]);
        for (auto& s : stmts)
        {
          if (s.index == index)
          {
            continue;
          }

          if (s.remove(loc))
          {
            edges[s.index].insert(index);
          }
        }
      }
    }

    // topological sort
    std::set<size_t> not_visited;
    std::set<size_t> frontier;
    for (size_t i = 0; i < stmts.size(); ++i)
    {
      not_visited.insert(i);
      if (edges[i].empty())
      {
        frontier.insert(i);
      }
    }

    logging::Trace() << "Reordered statements:";
    Nodes ordered_stmts;
    while (!frontier.empty())
    {
      size_t index = *frontier.begin();
      frontier.erase(frontier.begin());
      not_visited.erase(index);

      StmtInfo& stmt = stmts[index];
      logging::Trace() << stmt;
      Node node = unifybody->at(index);
      if (stmt.is_init())
      {
        Node expr = node->front()->front();

        Node lhs = expr->front();
        Node rhs = expr->back();
        for (auto& loc : stmt.lhs.inits)
        {
          locals.erase(loc);
        }

        for (auto& loc : stmt.rhs.inits)
        {
          locals.erase(loc);
        }

        ordered_stmts.push_back(
          to_init(lhs, stmt.lhs.inits, rhs, stmt.rhs.inits));
      }
      else
      {
        ordered_stmts.push_back(node);
      }

      for (size_t i = 0; i < stmts.size(); ++i)
      {
        if (edges[i].erase(index) > 0 && edges[i].empty())
        {
          frontier.insert(i);
        }
      }
    }

    if (!not_visited.empty())
    {
      logging::Warn() << "Unable to order body statements, possible error in "
                         "unification logic";

      while (!not_visited.empty())
      {
        size_t index = *not_visited.begin();
        not_visited.erase(not_visited.begin());
        Node node = unifybody->at(index);
        ordered_stmts.push_back(node);
      }
    }

    // where appropriate, recurse with the updated locals
    for (Node stmt : ordered_stmts)
    {
      if (stmt->in({LiteralEnum, LiteralWith, LiteralNot}))
      {
        find_init_stmts(stmt / UnifyBody, locals, outer_inits);
      }
    }

    unifybody->erase(unifybody->begin(), unifybody->end());
    unifybody->insert(
      unifybody->end(), ordered_stmts.begin(), ordered_stmts.end());
    locals.pop();
  }

  int register_init_stmts(Node rule)
  {
    Node body = rule / Body;
    Node val = rule / Val;

    if (body->type() == UnifyBody)
    {
      Locals locals;
      find_init_stmts(body, locals, {});
    }

    if (val->type() == UnifyBody)
    {
      Locals locals;
      find_init_stmts(val, locals, {});
    }

    return 0;
  }
}

namespace rego
{
  // This pass performs initialization analysis on the program and finds
  // all assignments which initialize a variable (as opposed to unifying it
  // with an additional result). It wraps these in special LiteralInit nodes,
  // which record which variables on both sides of the assignment are being
  // initialized. These allow later passes to correctly handle these statements.
  PassDef init()
  {
    PassDef init = {"init", wf_pass_init, dir::once | dir::bottomup};

    init.pre(RuleComp, register_init_stmts);
    init.pre(RuleFunc, register_init_stmts);
    init.pre(RuleSet, register_init_stmts);
    init.pre(RuleObj, register_init_stmts);
    init.pre(NestedBody, [](Node nested) {
      Locals locals;
      std::set<Location> init_vars;
      Node init_parent = nested->parent(LiteralInit);
      if (init_parent != nullptr)
      {
        // if this nested body is part of an init statement, then the
        // init vars cannot be referenced within the statement.
        Node lhs = init_parent / Lhs;
        Node rhs = init_parent / Rhs;
        for (Node var : *lhs)
        {
          init_vars.insert(var->location());
        }
        for (Node var : *rhs)
        {
          init_vars.insert(var->location());
        }
      }
      find_init_stmts(nested / Val, locals, init_vars);
      return 0;
    });

    init.pre(ExprEvery, [](Node exprevery) {
      Locals locals;
      find_init_stmts(exprevery / UnifyBody, locals, {});
      return 0;
    });

    return init;
  }
}