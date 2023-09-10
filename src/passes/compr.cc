#include "internal.h"

namespace rego
{
  // Reconfigures RuleObj and RuleSet nodes as comprehensions.
  PassDef rules_to_compr()
  {
    return {
      dir::bottomup | dir::once,
      {
        In(Policy) *
            (T(RuleSet)
             << (T(Var)[Var] * (T(UnifyBody) / T(Empty))[Body] *
                 T(DataTerm)[Val])) >>
          [](Match& _) {
            return RuleSet << _(Var) << _(Body)
                           << (DataTerm << (DataSet << _[Val]));
          },

        In(Policy) * T(RuleSet) << (T(Var)[Var] * T(Empty) * T(Expr)[Val]) >>
          [](Match& _) {
            Location value = _.fresh({"value"});
            return RuleSet
              << _(Var) << Empty
              << (UnifyBody
                  << (Local << (Var ^ value) << Undefined)
                  << (Literal
                      << (Expr << (RefTerm << (Var ^ value)) << Unify
                               << (Expr << (Term << (Set << _(Val)))))));
          },

        In(Policy) * T(RuleSet)
            << (T(Var)[Var] * T(UnifyBody)[Body] * T(Expr)[Val]) >>
          [](Match& _) {
            Location value = _.fresh({"value"});
            Location compr = _.fresh({"setcompr"});
            Node body = NestedBody << (Key ^ compr) << _(Body);
            return RuleSet
              << _(Var) << Empty
              << (UnifyBody
                  << (Local << (Var ^ value) << Undefined)
                  << (Literal
                      << (Expr << (RefTerm << (Var ^ value)) << Unify
                               << (Term << (SetCompr << _(Val) << body)))));
          },

        In(Policy) *
            (T(RuleObj)
             << (T(Var)[Var] * T(Empty) * T(Expr)[Key] * T(Expr)[Val])) >>
          [](Match& _) {
            Location value = _.fresh({"value"});
            Location key = _.fresh({"key"});
            return RuleObj
              << _(Var) << Empty
              << (UnifyBody
                  << (Local << (Var ^ value) << Undefined)
                  << (Local << (Var ^ key) << Undefined)
                  << (Literal
                      << (Expr << (RefTerm << (Var ^ key)) << Unify << _(Key)))
                  << (Literal
                      << (Expr
                          << (RefTerm << (Var ^ value)) << Unify
                          << (Expr
                              << (Term
                                  << (Object
                                      << (ObjectItem
                                          << (Expr << (RefTerm << (Var ^ key)))
                                          << _(Val))))))));
          },

        In(Policy) *
            (T(RuleObj)
             << (T(Var)[Var] * (T(UnifyBody) / T(Empty))[Body] *
                 (T(DataTerm)[Key]) * T(DataTerm)[Val])) >>
          [](Match& _) {
            return RuleObj << _(Var) << _(Body)
                           << (DataTerm
                               << (DataObject
                                   << (DataObjectItem << _(Key) << _(Val))));
          },

        In(Policy) *
            (T(RuleObj)
             << (T(Var)[Var] * T(UnifyBody)[Body] * T(Expr)[Key] *
                 T(Expr)[Val])) >>
          [](Match& _) {
            Location value = _.fresh({"value"});
            Location compr = _.fresh({"objcompr"});
            Node body = _(Body);
            if (body->type() == Empty)
            {
              body = UnifyBody
                << (Literal << (Expr << (Term << (Scalar << (JSONTrue)))));
            }

            body = NestedBody << (Key ^ compr) << body;

            return RuleObj
              << _(Var) << Empty
              << (UnifyBody
                  << (Local << (Var ^ value) << Undefined)
                  << (Literal
                      << (Expr << (RefTerm << (Var ^ value)) << Unify
                               << (Expr
                                   << (Term
                                       << (ObjectCompr << _(Key) << _(Val)
                                                       << body))))));
          },

        // errors

        In(RuleObj) * (T(Expr)[Expr] * T(DataTerm)) >>
          [](Match& _) {
            return err(
              _(Expr), "Syntax error: expected matching key/value node types");
          },

        In(RuleObj) * (T(DataTerm) * T(Expr)[Expr]) >>
          [](Match& _) {
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
      dir::topdown,
      {
        In(ArrayCompr, SetCompr) *
            (T(Expr)[Expr] * T(NestedBody)[NestedBody]) >>
          [](Match& _) {
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
                  << (Expr << (RefTerm << (Var ^ out)) << Unify << _(Expr)));
            (_(NestedBody) / Val)
              ->push_front(Local << (Var ^ out) << Undefined);
            return Seq << (Var ^ out) << _(NestedBody);
          },

        In(ObjectCompr) *
            (T(Expr)[Key] * T(Expr)[Val] * T(NestedBody)[NestedBody]) >>
          [](Match& _) {
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
                  << (Expr << (RefTerm << (Var ^ out)) << Unify
                           << (Expr << (Term << (Array << _(Key) << _(Val))))));
            (_(NestedBody) / Val)
              ->push_front(Local << (Var ^ out) << Undefined);

            return Seq << (Var ^ out) << _(NestedBody);
          },
      }};
  }
}