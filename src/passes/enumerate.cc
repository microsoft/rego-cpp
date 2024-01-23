#include "internal.hh"

namespace
{
  using namespace rego;

  const auto inline LiteralToken = T(Literal) / T(LiteralWith) /
    T(LiteralEnum) / T(LiteralInit) / T(LiteralNot) / T(Local);

  void find_all_refs_in(const Node& node, Location loc, Nodes& refs)
  {
    if (node->type() == Var)
    {
      if (node->location() == loc)
      {
        refs.push_back(node);
      }
    }
    else if (node->type() != RefArgDot)
    {
      for (auto& child : *node)
      {
        find_all_refs_in(child, loc, refs);
      }
    }
  }

  bool should_be_defined_in(Node local, Node unifybody)
  {
    Nodes refs;
    find_all_refs_in(local->scope(), (local / Var)->location(), refs);
    for (auto& ref : refs)
    {
      if (ref->parent() == local.get())
      {
        continue;
      }

      Node common_parent = unifybody->common_parent(ref);
      if (common_parent != unifybody)
      {
        return false;
      }
    }

    return true;
  }

  Node find_enum(Node unifybody, NodeIt start)
  {
    for (auto it = start; it != unifybody->end(); ++it)
    {
      Node node = *it;
      if (node->type() == LiteralEnum)
      {
        return node;
      }
    }
    return {};
  }

  Node next_enum(Node local)
  {
    Node unifybody = local->parent()->shared_from_this();
    auto it = unifybody->find(local) + 1;
    return find_enum(unifybody, it);
  }

  void vars_from(Node node, std::set<Location>& vars)
  {
    if (node->type() == Var)
    {
      vars.insert(node->location());
    }

    for (Node child : *node)
    {
      vars_from(child, vars);
    }
  }

  // Determines which statements following an implicit enum statement are needed to
  // instantiate the item sequence for that enum and adds them to the prefix.
  // Otherwise they should be captured by the enum and are placed in the postfix.
  void determine_prefix_and_postfix(
    const NodeRange tail, Node itemseq, std::vector<Node>& prefix, std::vector<Node>& postfix)
  {
    std::set<Location> vars;
    vars_from(itemseq, vars);
    for(auto it = tail.first; it != tail.second; ++it)
    {
      Node stmt = *it;
      if (stmt->type() == LiteralInit)
      {
        std::set<Location> init_vars;
        vars_from(stmt / Lhs, init_vars);
        vars_from(stmt / Rhs, init_vars);
        std::set<Location> intersection;
        std::set_intersection(
          vars.begin(),
          vars.end(),
          init_vars.begin(),
          init_vars.end(),
          std::inserter(intersection, intersection.begin()));
        if (intersection.empty())
        {
          postfix.push_back(stmt);
        }
        else
        {
          // the item sequence depends on this init, so it must
          // precede it.
          prefix.push_back(stmt);
        }
      }
      else
      {
        postfix.push_back(stmt);
      }
    }
  }
}

namespace rego
{
  // Nests statements that depend on enumeration under LiteralEnum nodes.
  PassDef explicit_enums()
  {
    return {
      "explicit_enums",
      wf_pass_explicit_enums,
      dir::topdown,
      {
        In(UnifyBody) *
            ((T(LiteralEnum) << (T(Var)[Item] * T(Expr)[ItemSeq])) *
             LiteralToken++[Tail] * End) >>
          [](Match& _) {
            ACTION();
            auto temp = _.fresh({"enum"});
            auto itemseq = _.fresh({"itemseq"});
            // all statements under the LiteralEnum node must be moved to its
            // body, as they depend on the enumerated values.
            Node body = UnifyBody << _[Tail];
            if (body->size() == 0)
            {
              body << (Literal << (Expr << (Term << (Scalar << True))));
            }
            return Seq << (Local << (Var ^ itemseq) << Undefined)
                       << (Literal
                           << (Expr << (RefTerm << (Var ^ itemseq)) << Unify
                                    << *_[ItemSeq]))
                       << (LiteralEnum << _(Item)->clone() << (Var ^ itemseq)
                                       << body);
          },
      }};
  }

  // Finds enum statements hiding as <val> = <ref>[<idx>] and lifts them to
  // LiteralEnum nodes.
  PassDef implicit_enums()
  {
    return {
      "implicit_enums",
      wf_pass_implicit_enums,
      dir::topdown,
      {
        In(UnifyBody) *
            (T(LiteralInit)
             << ((T(VarSeq) << Any)[LhsVars] * T(VarSeq)[RhsVars] *
                 (T(AssignInfix)
                  << ((T(AssignArg)[Lhs]
                       << (T(RefTerm)
                           << (T(SimpleRef)
                               << ((T(Var)[ItemSeq]) * T(RefArgBrack))))) *
                      T(AssignArg)[Rhs])))) >>
          [](Match& _) {
            ACTION();
            return LiteralInit << _(RhsVars) << _(LhsVars)
                               << (AssignInfix << _(Rhs) << _(Lhs));
          },

        In(UnifyBody) *
            ((T(LiteralInit)
              << ((T(VarSeq) << Any)[LhsVars] * (T(VarSeq) << Any)[RhsVars] *
                  (T(AssignInfix)
                   << (T(AssignArg)[Lhs] *
                       (T(AssignArg)
                        << (T(RefTerm)
                            << (T(SimpleRef)
                                << ((T(Var)[ItemSeq]) *
                                    (T(RefArgBrack)[Idx]))))))))) *
             LiteralToken++[Tail] * End) >>
          [](Match& _) {
            ACTION();
            logging::Debug() << "val = ref[idx]";

            Node idx = _(Idx)->front();
            if (idx->type() == Expr)
            {
              if (idx->size() == 1 && is_constant(idx->front()))
              {
                idx = idx->front();
              }
              else
              {
                return err(idx, "Invalid index for enumeration");
              }
            }

            if (idx->type() == NumTerm)
            {
              idx = Term << (Scalar << idx->front());
            }

            std::set<Token> term_types = {
              Scalar, Array, Set, Object, ArrayCompr, SetCompr, ObjectCompr};
            if (contains(term_types, idx->type()))
            {
              idx = Term << idx;
            }

            if (idx->type() != RefTerm && idx->type() != Term)
            {
              return err(idx, "Invalid index for enumeration");
            }

            std::vector<Node> prefix;
            std::vector<Node> postfix;
            determine_prefix_and_postfix(_[Tail], _(ItemSeq), prefix, postfix);

            auto temp = _.fresh({"enum"});
            auto item = _.fresh({"item"});
            return Seq
              << prefix << (Local << (Var ^ item) << Undefined)
              << (LiteralEnum
                  << (Var ^ item) << _(ItemSeq)
                  << (UnifyBody
                      << (LiteralInit
                          << _(RhsVars) << VarSeq
                          << (AssignInfix
                              << (AssignArg << idx)
                              << (AssignArg
                                  << (RefTerm
                                      << (SimpleRef
                                          << (Var ^ item)
                                          << (RefArgBrack
                                              << (Scalar << (Int ^ "0"))))))))
                      << (LiteralInit
                          << _(LhsVars) << VarSeq
                          << (AssignInfix
                              << _(Lhs)
                              << (AssignArg
                                  << (RefTerm
                                      << (SimpleRef
                                          << (Var ^ item)
                                          << (RefArgBrack
                                              << (Scalar << (Int ^ "1"))))))))
                      << postfix));
          },

        In(UnifyBody) *
            ((T(LiteralInit)
              << ((T(VarSeq) << End) * (T(VarSeq) << Any)[RhsVars] *
                  (T(AssignInfix)
                   << (T(AssignArg)[Lhs] *
                       (T(AssignArg)
                        << (T(RefTerm)
                            << (T(SimpleRef)
                                << ((T(Var)[ItemSeq]) *
                                    T(RefArgBrack)[Idx])))))))) *
             LiteralToken++[Tail] * End) >>
          [](Match& _) {
            ACTION();
            logging::Debug() << "val = ref[idx]";

            Node idx = _(Idx)->front();
            if (idx->type() == Expr)
            {
              if (idx->size() == 1 && is_constant(idx->front()))
              {
                idx = idx->front();
              }
              else
              {
                return err(idx, "Invalid index for enumeration");
              }
            }

            if (idx->type() == NumTerm)
            {
              idx = Term << (Scalar << idx->front());
            }

            std::set<Token> term_types = {
              Scalar, Array, Set, Object, ArrayCompr, SetCompr, ObjectCompr};
            if (contains(term_types, idx->type()))
            {
              idx = Term << idx;
            }

            std::vector<Node> prefix;
            std::vector<Node> postfix;
            determine_prefix_and_postfix(_[Tail], _(ItemSeq), prefix, postfix);

            auto temp = _.fresh({"enum"});
            auto item = _.fresh({"item"});
            return Seq
              << prefix
              << (Local << (Var ^ item) << Undefined)
              << (LiteralEnum
                  << (Var ^ item) << _(ItemSeq)
                  << (UnifyBody
                      << (LiteralInit
                          << _(RhsVars) << VarSeq
                          << (AssignInfix
                              << (AssignArg << idx)
                              << (AssignArg
                                  << (RefTerm
                                      << (SimpleRef
                                          << (Var ^ item)
                                          << (RefArgBrack
                                              << (Scalar << (Int ^ "0"))))))))
                      << (Literal
                          << (Expr
                              << (AssignInfix
                                  << _(Lhs)
                                  << (AssignArg
                                      << (RefTerm
                                          << (SimpleRef
                                              << (Var ^ item)
                                              << (RefArgBrack
                                                  << (Scalar
                                                      << (Int ^ "1")))))))))
                      << postfix));
          },
      }};
  }

  // Fixes situations in which a local has been in the wrong place. If the local
  // should have been defined inside the enum, it will move the declaration
  // inside. Conversely, if it should not be defined inside the enum it will
  // move it out into the containing scope.
  PassDef enum_locals()
  {
    PassDef enum_locals{
      "enum_locals", wf_pass_enum_locals, dir::bottomup | dir::once};

    std::shared_ptr<NodeMap<std::map<Location, Nodes>>> all_refs =
      std::make_shared<NodeMap<std::map<Location, Nodes>>>();
    std::shared_ptr<NodeMap<Node>> moves = std::make_shared<NodeMap<Node>>();

    enum_locals.pre(Local, [moves](Node local) {
      if (in_query(local))
      {
        return 0;
      }

      if (starts_with((local / Var)->location().view(), "out$"))
      {
        // this variable is an output of a rule, and needs to stay
        // at the scope where it was added
        return 0;
      }

      if (is_in(local, {LiteralEnum}))
      {
        // should this local be defined here?
        Node unifybody = local->parent()->shared_from_this();
        bool requires_move = false;
        while (!should_be_defined_in(local, unifybody))
        {
          // we need to keep popping out of nested enums until we find the
          // correct scope
          requires_move = true;
          NodeDef* literalenum = unifybody->parent();
          if (literalenum->type() != LiteralEnum)
          {
            // we've popped out of the nested enums
            break;
          }

          unifybody = literalenum->parent()->shared_from_this();
        }

        if (requires_move)
        {
          moves->insert({local, unifybody});
          return 0;
        }
      }

      Node literalenum = next_enum(local);
      if (literalenum == nullptr)
      {
        // no enum to move to
        return 0;
      }

      Node unifybody = nullptr;
      while (should_be_defined_in(local, literalenum / UnifyBody))
      {
        // continue nesting until we find the most constrained scope
        unifybody = literalenum / UnifyBody;
        literalenum = find_enum(unifybody, unifybody->begin());
        if (literalenum == nullptr)
        {
          // no further nested enums to check
          break;
        }
      }

      if (unifybody != nullptr)
      {
        moves->insert({local, unifybody});
      }

      return 0;
    });

    enum_locals.post([moves](Node) {
      for (auto& [local, target] : *moves)
      {
        local->parent()->replace(local);
        target->push_front(local);
      }

      return 0;
    });

    return enum_locals;
  }
}