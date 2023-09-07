#include "errors.h"
#include "helpers.h"
#include "log.h"
#include "passes.h"

namespace
{
  using namespace rego;

  const auto inline LiteralToken = T(Literal) / T(LiteralWith) /
    T(LiteralEnum) / T(LiteralInit) / T(LiteralNot) / T(Local);

  void all_refs(Node node, Location loc, Nodes& refs)
  {
    if (node->type() == RefArgDot)
    {
      return;
    }
    if (node->type() == Var)
    {
      if (node->location() == loc)
      {
        refs.push_back(node);
      }
    }
    else
    {
      for (auto& child : *node)
      {
        all_refs(child, loc, refs);
      }
    }
  }

  bool can_grab(Node local, Node unifybody)
  {
    Nodes refs;
    all_refs(local->scope(), (local / Var)->location(), refs);
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

  Node next_enum(Node local)
  {
    Node unifybody = local->parent()->shared_from_this();
    auto it = unifybody->find(local) + 1;
    while (it != unifybody->end())
    {
      Node node = *it;
      if (node->type() == LiteralEnum)
      {
        return node;
      }
      ++it;
    }
    return {};
  }
}

namespace rego
{
  // Nests statements that depend on enumeration under LiteralEnum nodes.
  PassDef explicit_enums()
  {
    return {
      In(UnifyBody) *
          ((T(LiteralEnum) << (T(Var)[Item] * T(Expr)[ItemSeq])) *
           LiteralToken++[Tail] * End) >>
        [](Match& _) {
          auto temp = _.fresh({"enum"});
          auto itemseq = _.fresh({"itemseq"});
          // all statements under the LiteralEnum node must be moved to its
          // body, as they depend on the enumerated values.
          Node body = UnifyBody << _[Tail];
          if (body->size() == 0)
          {
            body << (Literal << (Expr << (Term << (Scalar << JSONTrue))));
          }
          return Seq << (Local << (Var ^ itemseq) << Undefined)
                     << (Literal
                         << (Expr << (RefTerm << (Var ^ itemseq)) << Unify
                                  << *_[ItemSeq]))
                     << (LiteralEnum << _(Item)->clone() << (Var ^ itemseq)
                                     << body);
        },

    };
  }

  // Finds enum statements hiding as <val> = <ref>[<idx>] and lifts them to
  // LiteralEnum nodes. Also fixes situations in which a local has been
  // incorrectly captured by an enum.
  PassDef implicit_enums()
  {
    return {
      In(UnifyBody) *
          (T(LiteralInit)
           << (T(VarSeq)[LhsVars](
                 [](auto& n) { return (*n.first)->size() > 0; }) *
               T(VarSeq)[RhsVars](
                 [](auto& n) { return (*n.first)->size() >= 0; }) *
               (T(AssignInfix)
                << ((T(AssignArg)[Lhs]
                     << (T(RefTerm)
                         << (T(SimpleRef)
                             << ((T(Var)[ItemSeq]) * T(RefArgBrack))))) *
                    T(AssignArg)[Rhs])))) >>
        [](Match& _) {
          return LiteralInit << _(RhsVars) << _(LhsVars)
                             << (AssignInfix << _(Rhs) << _(Lhs));
        },

      In(UnifyBody) *
          ((
             T(LiteralInit)
             << (T(VarSeq)[LhsVars](
                   [](auto& n) { return (*n.first)->size() > 0; }) *
                 T(VarSeq)[RhsVars](
                   [](auto& n) { return (*n.first)->size() > 0; }) *
                 (T(AssignInfix)
                  << (T(AssignArg)[Lhs] *
                      (T(AssignArg)
                       << (T(RefTerm)
                           << (T(SimpleRef)
                               << ((T(Var)[ItemSeq]) *
                                   (T(RefArgBrack)[Idx]))))))))) *
           LiteralToken++[Tail] * End) >>
        [](Match& _) {
          LOG("val = ref[idx]");

          Node idx = _(Idx)->front();
          if (
            idx->type() == Expr && idx->size() == 1 &&
            is_constant(idx->front()))
          {
            idx = idx->front();
          }
          else
          {
            return err(idx, "Invalid index for enumeration");
          }

          if (idx->type() == NumTerm)
          {
            idx = Term << (Scalar << idx->front());
          }

          std::set<Token> term_types = {
            Scalar, Array, Set, Object, ArrayCompr, SetCompr, ObjectCompr};
          if (term_types.contains(idx->type()))
          {
            idx = Term << idx;
          }

          if (idx->type() != RefTerm && idx->type() != Term)
          {
            return err(idx, "Invalid index for enumeration");
          }

          auto temp = _.fresh({"enum"});
          auto item = _.fresh({"item"});
          return Seq
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
                                            << (Scalar << (JSONInt ^ "0"))))))))
                    << (LiteralInit
                        << _(LhsVars) << VarSeq
                        << (AssignInfix
                            << _(Lhs)
                            << (AssignArg
                                << (RefTerm
                                    << (SimpleRef
                                        << (Var ^ item)
                                        << (RefArgBrack
                                            << (Scalar << (JSONInt ^ "1"))))))))
                    << _[Tail]));
        },

      In(UnifyBody) *
          ((
             T(LiteralInit)
             << (T(VarSeq)([](auto& n) { return (*n.first)->size() == 0; }) *
                 T(VarSeq)[RhsVars](
                   [](auto& n) { return (*n.first)->size() > 0; }) *
                 (T(AssignInfix)
                  << (T(AssignArg)[Lhs] *
                      (T(AssignArg)
                       << (T(RefTerm)
                           << (T(SimpleRef)
                               << ((T(Var)[ItemSeq]) *
                                   T(RefArgBrack)[Idx])))))))) *
           LiteralToken++[Tail] * End) >>
        [](Match& _) {
          LOG("val = ref[idx]");

          Node idx = _(Idx)->front();
          if (
            idx->type() == Expr && idx->size() == 1 &&
            is_constant(idx->front()))
          {
            idx = idx->front();
          }
          else
          {
            return err(idx, "Invalid index for enumeration");
          }

          if (idx->type() == NumTerm)
          {
            idx = Term << (Scalar << idx->front());
          }

          std::set<Token> term_types = {
            Scalar, Array, Set, Object, ArrayCompr, SetCompr, ObjectCompr};
          if (term_types.contains(idx->type()))
          {
            idx = Term << idx;
          }

          auto temp = _.fresh({"enum"});
          auto item = _.fresh({"item"});
          return Seq
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
                                            << (Scalar << (JSONInt ^ "0"))))))))
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
                                                    << (JSONInt ^ "1")))))))))
                    << _[Tail]));
        },

      In(UnifyBody) * T(Local)[Local]([](auto& n) {
        Node local = *n.first;
        if (in_query(local))
        {
          return false;
        }
        if ((local / Var)->location().view().starts_with("out$"))
        {
          return false;
        }

        Node literalenum = next_enum(local);
        if (literalenum == nullptr)
        {
          return false;
        }

        return can_grab(local, literalenum / UnifyBody);
      }) >>
        [](Match& _) {
          Node unifybody = next_enum(_(Local)) / UnifyBody;
          unifybody->push_front(_(Local));
          return Node{};
        },

      In(UnifyBody) * T(Local)[Local]([](auto& n) {
        Node unifybody = (*n.first)->parent()->shared_from_this();
        return is_in(*n.first, {LiteralEnum}) && !can_grab(*n.first, unifybody);
      }) >>
        [](Match& _) { return Lift << LiteralEnum << _(Local); },

      In(LiteralEnum) * T(Local)[Local] >>
        [](Match& _) { return Lift << UnifyBody << _(Local); }

    };
  }
}