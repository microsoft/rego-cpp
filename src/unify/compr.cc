#include "unify.hh"

namespace rego
{
  // Reconfigures RuleObj and RuleSet nodes as comprehensions.
  PassDef rules_to_compr()
  {
    return {
      "rules_to_compr",
      wf_pass_rules_to_compr,
      dir::bottomup | dir::once,
      {
        In(Policy) *
            (T(RuleSet)
             << (T(Var)[Var] * (T(UnifyBody) / T(Empty))[Body] *
                 T(DataTerm)[Val] * T(Version)[Version])) >>
          [](Match& _) {
            ACTION();
            return RuleSet << _(Var) << _(Body)
                           << (DataTerm << (DataSet << _[Val])) << _(Version);
          },

        In(Policy) * T(RuleSet)
            << (T(Var)[Var] * T(Empty) * T(Expr)[Val] * T(Version)[Version]) >>
          [](Match& _) {
            ACTION();
            Location value = _.fresh({"value"});
            return RuleSet << _(Var) << Empty
                           << (UnifyBody
                               << (Local << (Var ^ value) << Undefined)
                               << (Literal
                                   << (Expr << expr_infix(
                                         Unify,
                                         RefTerm << (Var ^ value),
                                         Term << (Set << _(Val))))))
                           << _(Version);
          },

        In(Policy) * T(RuleSet)
            << (T(Var)[Var] * T(UnifyBody)[Body] * T(Expr)[Val] *
                T(Version)[Version]) >>
          [](Match& _) {
            ACTION();
            Location value = _.fresh({"value"});
            Location compr = _.fresh({"setcompr"});
            Node body = NestedBody << (Key ^ compr) << _(Body);
            return RuleSet
              << _(Var) << Empty
              << (UnifyBody << (Local << (Var ^ value) << Undefined)
                            << (Literal
                                << (Expr << expr_infix(
                                      Unify,
                                      RefTerm << (Var ^ value),
                                      Term << (SetCompr << _(Val) << body)))))
              << _(Version);
          },

        In(Policy) *
            (T(RuleObj)
             << (T(Var)[Var] * T(Empty) * T(Expr)[Key] * T(Expr)[Val] *
                 T(True, False)[IsVarRef] * T(Version)[Version])) >>
          [](Match& _) {
            ACTION();
            Location value = _.fresh({"value"});
            Location key = _.fresh({"key"});
            return RuleObj
              << _(Var) << Empty
              << (UnifyBody
                  << (Local << (Var ^ value) << Undefined)
                  << (Local << (Var ^ key) << Undefined)
                  << (Literal
                      << (Expr
                          << expr_infix(Unify, RefTerm << (Var ^ key), _(Key))))
                  << (Literal
                      << (Expr << expr_infix(
                            Unify,
                            RefTerm << (Var ^ value),
                            Term
                              << (Object
                                  << (ObjectItem
                                      << (Expr << (RefTerm << (Var ^ key)))
                                      << _(Val)))))))
              << _(IsVarRef) << _(Version);
          },

        In(Policy) *
            (T(RuleObj)
             << (T(Var)[Var] * (T(UnifyBody) / T(Empty))[Body] *
                 T(DataTerm)[Key] * T(DataTerm)[Val] *
                 T(True, False)[IsVarRef] * T(Version)[Version])) >>
          [](Match& _) {
            ACTION();
            return RuleObj << _(Var) << _(Body)
                           << (DataTerm
                               << (DataObject
                                   << (DataObjectItem << _(Key) << _(Val))))
                           << _(IsVarRef) << _(Version);
          },

        In(Policy) *
            (T(RuleObj)
             << (T(Var)[Var] * T(UnifyBody)[Body] * T(Expr)[Key] *
                 T(Expr)[Val] * T(True, False)[IsVarRef] *
                 T(Version)[Version])) >>
          [](Match& _) {
            ACTION();
            Location value = _.fresh({"value"});
            Location compr = _.fresh({"objcompr"});
            Node body = _(Body);
            if (body->type() == Empty)
            {
              body = UnifyBody
                << (Literal << (Expr << (Term << (Scalar << (True)))));
            }

            body = NestedBody << (Key ^ compr) << body;

            return RuleObj << _(Var) << Empty
                           << (UnifyBody
                               << (Local << (Var ^ value) << Undefined)
                               << (Literal
                                   << (Expr << expr_infix(
                                         Unify,
                                         RefTerm << (Var ^ value),
                                         Term
                                           << (ObjectCompr << _(Key) << _(Val)
                                                           << body)))))
                           << _(IsVarRef) << _(Version);
          },

        // errors

        In(RuleObj) * (T(Expr)[Expr] * T(DataTerm)) >>
          [](Match& _) {
            ACTION();
            return err(
              _(Expr), "Syntax error: expected matching key/value node types");
          },

        In(RuleObj) * (T(DataTerm) * T(Expr)[Expr]) >>
          [](Match& _) {
            ACTION();
            return err(
              _(Expr), "Syntax error: expected matching key/value node types");
          },

      }};
  }

  // Augments nested comprehension bodies to contain statements that set the
  // comprehension item.
  PassDef compr()
  {
    return {
      "compr",
      wf_pass_compr,
      dir::topdown,
      {
        In(ArrayCompr, SetCompr) *
            (T(Expr)[Expr] * T(NestedBody)[NestedBody]) >>
          [](Match& _) {
            ACTION();
            Location out = _.fresh({"out"});

            // the comprehension logic needs to live in the inner-most
            // UnifyBody. As a enumeration call will always be the final
            // statement in a UnifyBody, we can find them by checking
            // the final statement.
            Node innermost = _(NestedBody) / Val;
            while (innermost->back()->type() == LiteralEnum ||
                   innermost->back()->type() == LiteralWith)
            {
              innermost = innermost->back() / UnifyBody;
            }
            innermost
              << (Literal
                  << (Expr << expr_infix(
                        Unify, RefTerm << (Var ^ out), _(Expr))));
            (_(NestedBody) / Val)
              ->push_front(Local << (Var ^ out) << Undefined);
            return Seq << (Var ^ out) << _(NestedBody);
          },

        In(ObjectCompr) *
            (T(Expr)[Key] * T(Expr)[Val] * T(NestedBody)[NestedBody]) >>
          [](Match& _) {
            ACTION();
            Location out = _.fresh({"out"});
            Node innermost = _(NestedBody) / Val;
            // see comment above
            while (innermost->back()->type() == LiteralEnum ||
                   innermost->back()->type() == LiteralWith)
            {
              innermost = innermost->back() / UnifyBody;
            }

            innermost
              << (Literal
                  << (Expr << expr_infix(
                        Unify,
                        RefTerm << (Var ^ out),
                        Term << (Array << _(Key) << _(Val)))));
            (_(NestedBody) / Val)
              ->push_front(Local << (Var ^ out) << Undefined);

            return Seq << (Var ^ out) << _(NestedBody);
          },
      }};
  }
}