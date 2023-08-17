#include "lang.h"
#include "passes.h"

namespace rego
{
  // Augments nested comprehension bodies to contain statements that set the
  // comprehension item. Also reconfigures RuleObj and RuleSet
  // nodes as comprehensions.
  PassDef compr()
  {
    return {
      dir::topdown | dir::once,
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
            return RuleSet
              << _(Var) << Empty
              << (UnifyBody
                  << (Local << (Var ^ value) << Undefined)
                  << (Literal
                      << (Expr << (RefTerm << (Var ^ value)) << Unify
                               << (Term << (SetCompr << _(Val) << _(Body))))));
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
                      << (Expr << (RefTerm << (Var ^ value)) << Unify
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
            std::string key = strip_quotes(to_json(_(Key)));
            return RuleObj << _(Var) << _(Body)
                           << (DataTerm
                               << (DataObject
                                   << (DataItem << (Key ^ key) << _(Val))));
          },

        In(Policy) *
            (T(RuleObj)
             << (T(Var)[Var] * T(UnifyBody)[Body] * T(Expr)[Key] *
                 T(Expr)[Val])) >>
          [](Match& _) {
            Location value = _.fresh({"value"});
            Node body = _(Body);
            if (body->type() == Empty)
            {
              body = UnifyBody
                << (Literal << (Expr << (Term << (Scalar << (JSONTrue)))));
            }

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

        (In(ArrayCompr) / In(SetCompr)) *
            (T(Expr)[Expr] * T(UnifyBody)[UnifyBody]) >>
          [](Match& _) {
            Location out = _.fresh({"out"});
            Location compr = _.fresh({"compr"});
            Node tail = _(UnifyBody);
            while (tail->back()->type() == LiteralEnum)
            {
              tail = tail->back() / UnifyBody;
            }
            tail
              << (Literal
                  << (Expr << (RefTerm << (Var ^ out)) << Unify << _(Expr)));
            _(UnifyBody)->push_front(Local << (Var ^ out) << Undefined);
            return Seq << (Var ^ out)
                       << (NestedBody << (Key ^ compr) << _(UnifyBody));
          },

        In(ObjectCompr) *
            (T(Expr)[Key] * T(Expr)[Val] * T(UnifyBody)[UnifyBody]) >>
          [](Match& _) {
            Location out = _.fresh({"out"});
            Location compr = _.fresh({"compr"});
            Node tail = _(UnifyBody);
            while (tail->back()->type() == LiteralEnum)
            {
              tail = tail->back() / UnifyBody;
            }

            tail
              << (Literal
                  << (Expr << (RefTerm << (Var ^ out)) << Unify
                           << (Expr << (Term << (Array << _(Key) << _(Val))))));
            _(UnifyBody)->push_front(Local << (Var ^ out) << Undefined);

            return Seq << (Var ^ out)
                       << (NestedBody << (Key ^ compr) << _(UnifyBody));
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
}