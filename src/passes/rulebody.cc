#include "lang.h"
#include "log.h"
#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const auto inline VarOrTerm = T(Var) / T(Term);

  // clang-format off
  inline const auto wfi =
      (Expr <<= (Val >>= NumTerm | RefTerm | Term | UnaryExpr | ArithInfix | BoolInfix | AssignInfix))
    | (AssignArg <<= (Val >>= RefTerm | NumTerm | UnaryExpr | ArithInfix | Term | BoolInfix))
    | ((ObjectItem <<= Key * Expr))
    | (RefObjectItem <<= RefTerm * Expr)
    ;
  // clang-format on
}

namespace rego
{
  using namespace wf::ops;

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
          LOG("<term> = <term>");
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"unify"});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          Node expr = Expr
            << (BoolInfix << (BoolArg << _(Rhs)->at(wfi / AssignArg / Val))
                          << Equals
                          << (BoolArg << _(Lhs)->at(wfi / AssignArg / Val)));
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
        [](Match& _) {
          LOG("<term> = <any>");
          return AssignInfix << _(Rhs) << _(Lhs);
        },

      // a = <term>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(RefTerm) << T(Var)[Lhs]([](auto& n) {
                                   return contains_local(*n.first);
                                 }))) *
               T(AssignArg)[Rhs])) >>
        [](Match& _) {
          LOG("a = <term>");
          return UnifyExpr << _(Lhs)
                           << (Expr << _(Rhs)->at(wfi / AssignArg / Val));
        },

      // <ref> = <term>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(RefTerm)[Lhs] << T(SimpleRef)([](auto& n) {
                                   return contains_local(*n.first);
                                 }))) *
               T(AssignArg)[Rhs])) >>
        [](Match& _) {
          LOG("<ref> = <term>");
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"ref"});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          seq->push_back(UnifyExpr << (Var ^ temp) << (Expr << _(Lhs)));
          seq->push_back(
            UnifyExpr << (Var ^ temp)
                      << (Expr << _(Rhs)->at(wfi / AssignArg / Val)));
          return seq;
        },

      // <array> = <array>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Array)[Lhs])) *
               (T(AssignArg) << (T(Term) << T(Array)[Rhs])))) >>
        [](Match& _) {
          LOG("<array> = <array>");
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
              AssignInfix << (AssignArg << lhs->at(i)->at(wfi / Expr / Val))
                          << (AssignArg << rhs->at(i)->at(wfi / Expr / Val)));
          }

          return seq;
        },

      // <object> = <object>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
               (T(AssignArg) << (T(Term) << T(Object)[Rhs])))) >>
        [](Match& _) {
          LOG("<object> = <object>");
          Node lhs = _(Lhs);
          Node rhs = _(Rhs);
          if (lhs->size() != rhs->size())
          {
            return err(_(Lhs), "Object size mismatch");
          }
          Node seq = NodeDef::create(Seq);
          for (const auto& item : *lhs)
          {
            Node key = item->at(wfi / ObjectItem / Key);
            Node value = item->at(wfi / ObjectItem / Expr);
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
               (T(AssignArg)[Rhs] << (T(Term) << T(Array)([](auto& n) {
                                        return contains_local(*n.first);
                                      }))))) >>
        [](Match& _) {
          LOG("<refterm> = <array>");
          return AssignInfix << _(Rhs) << _(Lhs);
        },

      // <array> = <var>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Array)[Lhs])) *
               (T(AssignArg) << (T(RefTerm) << T(Var)[Rhs])))) >>
        [](Match& _) {
          LOG("<array> = <var>");
          Node seq = NodeDef::create(Seq);
          for (std::size_t i = 0; i < _(Lhs)->size(); i++)
          {
            Node index = Scalar << (JSONInt ^ std::to_string(i));
            Node ref = SimpleRef << _(Rhs)->clone() << (RefArgBrack << index);
            seq->push_back(
              AssignInfix << (AssignArg << _(Lhs)->at(i)->at(wfi / Expr / Val))
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
          LOG("<array> = <ref>");
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"ref"});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          seq->push_back(UnifyExpr << (Var ^ temp) << (Expr << _(RefTerm)));
          for (std::size_t i = 0; i < _(Lhs)->size(); i++)
          {
            Node index = Scalar << (JSONInt ^ std::to_string(i));
            Node ref = SimpleRef << (Var ^ temp) << (RefArgBrack << index);
            seq->push_back(
              AssignInfix << (AssignArg << _(Lhs)->at(i)->at(wfi / Expr / Val))
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
        [](Match& _) {
          LOG("<refterm> = <object>");
          return AssignInfix << _(Rhs) << _(Lhs);
        },

      // <object> = <var>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
               (T(AssignArg) << (T(RefTerm) << T(Var)[Rhs])))) >>
        [](Match& _) {
          LOG("<object> = <var>");
          Node seq = NodeDef::create(Seq);
          for (const auto& item : *_(Lhs))
          {
            if (item->type() == ObjectItem)
            {
              Node index = Scalar
                << (JSONString ^ item->at(wfi / ObjectItem / Key)->location());
              Node ref = SimpleRef << _(Rhs)->clone() << (RefArgBrack << index);
              seq->push_back(
                AssignInfix
                << (AssignArg
                    << item->at(wfi / ObjectItem / Expr)->at(wfi / Expr / Val))
                << (AssignArg << (RefTerm << ref)));
            }
            else
            {
              seq->push_back(err(item, "Expected non-ref object item"));
            }
          }

          return seq;
        },

      // <object> = <ref>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
               (T(AssignArg) << T(RefTerm)[RefTerm]))) >>
        [](Match& _) {
          LOG("<object> = <ref>");
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"ref"});
          seq->push_back(Local << (Var ^ temp) << Undefined);
          seq->push_back(UnifyExpr << (Var ^ temp) << (Expr << _(RefTerm)));
          for (const auto& item : *_(Lhs))
          {
            std::string key =
              std::string(item->at(wfi / ObjectItem / Key)->location().view());
            Node index = Scalar << (JSONString ^ key);
            Node ref = SimpleRef << (Var ^ temp) << (RefArgBrack << index);
            seq->push_back(
              AssignInfix << (AssignArg << item->at(wfi / ObjectItem / Expr)
                                             ->at(wfi / Expr / Val))
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