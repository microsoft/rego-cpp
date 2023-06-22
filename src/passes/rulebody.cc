#include "lang.h"
#include "passes.h"

namespace
{
  using namespace rego;
  const auto inline VarOrTerm = T(Var) / T(Term);
}

namespace rego
{
  PassDef rulebody()
  {
    return {
      In(UnifyBody) *
          (T(Literal) << ((T(Expr) << T(AssignInfix)[AssignInfix]))) >>
        [](Match& _) { return _(AssignInfix); },

      // <expr>|<notexpr>
      In(UnifyBody) * (T(Literal) << (T(Expr) / T(NotExpr))[Expr]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          std::string prefix = is_in(_(Expr), Query) ? "value" : "unify";
          Location temp = _.fresh({prefix});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          seq->push_back(UnifyExpr << (Var ^ temp) << _(Expr));
          return seq;
        },

      // <term> = <term>
      In(UnifyBody) *
          (T(AssignInfix)
           << (T(AssignArg)[Lhs](
                 [](auto& n) { return !contains_local(*n.first); }) *
               T(AssignArg)[Rhs](
                 [](auto& n) { return !contains_local(*n.first); }))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"unify"});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          Node expr = Expr
            << (BoolInfix << (BoolArg << _(Rhs)->front()) << Equals
                          << (BoolArg << _(Lhs)->front()));
          seq->push_back(UnifyExpr << (Var ^ temp) << expr);
          return seq;
        },

      // <term> = any
      In(UnifyBody) *
          (T(AssignInfix)
           << (T(AssignArg)[Lhs](
                 [](auto& n) { return !contains_local(*n.first); }) *
               T(AssignArg)[Rhs](
                 [](auto& n) { return contains_local(*n.first); }))) >>
        [](Match& _) { return AssignInfix << _(Rhs) << _(Lhs); },

      // a = <term>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(RefTerm) << T(Var)[Lhs]([](auto& n) {
                                   return contains_local(*n.first);
                                 }))) *
               T(AssignArg)[Rhs])) >>
        [](Match& _) {
          return UnifyExpr << _(Lhs) << (Expr << _(Rhs)->front());
        },

      // <ref> = <term>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(RefTerm)[Lhs] << T(SimpleRef)([](auto& n) {
                                   return contains_local(*n.first);
                                 }))) *
               T(AssignArg)[Rhs])) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"ref"});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          seq->push_back(UnifyExpr << (Var ^ temp) << (Expr << _(Lhs)));
          seq->push_back(
            UnifyExpr << (Var ^ temp) << (Expr << _(Rhs)->front()));
          return seq;
        },

      // <array> = <array>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Array)[Lhs])) *
               (T(AssignArg) << (T(Term) << T(Array)[Rhs])))) >>
        [](Match& _) {
          Node lhs = _(Lhs);
          Node rhs = _(Rhs);
          if (lhs->size() != rhs->size())
          {
            return err(_(Lhs), "Array size mismatch");
          }
          Node seq = NodeDef::create(Seq);
          for (std::size_t i = 0; i < lhs->size(); i++)
          {
            seq->push_back(
              AssignInfix << (AssignArg << lhs->at(i)->front())
                          << (AssignArg << rhs->at(i)->front()));
          }

          return seq;
        },

      // <object> = <object>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
               (T(AssignArg) << (T(Term) << T(Object)[Rhs])))) >>
        [](Match& _) {
          Node lhs = _(Lhs);
          Node rhs = _(Rhs);
          if (lhs->size() != rhs->size())
          {
            return err(_(Lhs), "Object size mismatch");
          }
          Node seq = NodeDef::create(Seq);
          for (const auto& item : *lhs)
          {
            Node key = item->front();
            Node value = item->back();
            Nodes defs = rhs->lookdown(key->location());
            if (defs.size() == 0)
            {
              return err(key, "Object key not found");
            }
            seq->push_back(
              AssignInfix << (AssignArg << value) << (AssignArg << defs[0]));
          }
          return seq;
        },

      // <refterm> = <array>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg)[Lhs] << T(RefTerm)) *
               (T(AssignArg)[Rhs] << (T(Term) << T(Array))))) >>
        [](Match& _) { return AssignInfix << _(Rhs) << _(Lhs); },

      // <array> = <var>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Array)[Lhs])) *
               (T(AssignArg) << (T(RefTerm) << T(Var)[Rhs])))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          for (std::size_t i = 0; i < _(Lhs)->size(); i++)
          {
            Node index = Scalar << (JSONInt ^ std::to_string(i));
            Node ref = SimpleRef << _(Rhs)->clone() << (RefArgBrack << index);
            seq->push_back(
              AssignInfix << (AssignArg << _(Lhs)->at(i)->front())
                          << (AssignArg << (RefTerm << ref)));
          }

          return seq;
        },

      // <array> = <ref>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Array)[Lhs])) *
               (T(AssignArg) << T(RefTerm)[RefTerm]))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"ref"});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          seq->push_back(UnifyExpr << (Var ^ temp) << (Expr << _(RefTerm)));
          for (std::size_t i = 0; i < _(Lhs)->size(); i++)
          {
            Node index = Scalar << (JSONInt ^ std::to_string(i));
            Node ref = SimpleRef << (Var ^ temp) << (RefArgBrack << index);
            seq->push_back(
              AssignInfix << (AssignArg << _(Lhs)->at(i)->front())
                          << (AssignArg << (RefTerm << ref)));
          }

          return seq;
        },

      // <refterm> = <object>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg)[Lhs] << T(RefTerm)) *
               (T(AssignArg)[Rhs] << (T(Term) << T(Object)([](auto& n) {
                                        return contains_local(*n.first);
                                      }))))) >>
        [](Match& _) { return AssignInfix << _(Rhs) << _(Lhs); },

      // <object> = <var>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
               (T(AssignArg) << (T(RefTerm) << T(Var)[Rhs])))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          for (const auto& item : *_(Lhs))
          {
            Node index = Scalar << (JSONString ^ item->front()->location());
            Node ref = SimpleRef << _(Rhs)->clone() << (RefArgBrack << index);
            seq->push_back(
              AssignInfix << (AssignArg << item->back()->front())
                          << (AssignArg << (RefTerm << ref)));
          }

          return seq;
        },

      // <object> = <ref>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
               (T(AssignArg) << T(RefTerm)[RefTerm]))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"ref"});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          seq->push_back(UnifyExpr << (Var ^ temp) << (Expr << _(RefTerm)));
          for (const auto& item : *_(Lhs))
          {
            std::string key = std::string(item->front()->location().view());
            Node index = Scalar << (JSONString ^ key);
            Node ref = SimpleRef << (Var ^ temp) << (RefArgBrack << index);
            seq->push_back(
              AssignInfix << (AssignArg << item->back()->front())
                          << (AssignArg << (RefTerm << ref)));
          }

          return seq;
        },

      // errors

      In(BoolArg) * T(BoolInfix)[BoolInfix] >>
        [](Match& _) {
          return err(_(BoolInfix), "Invalid boolean expression");
        },

      (In(UnifyBody) / In(Expr)) * T(AssignInfix)[AssignInfix] >>
        [](Match& _) { return err(_(AssignInfix), "Invalid assignment"); },
    };
  }
}