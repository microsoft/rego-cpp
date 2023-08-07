#include "lang.h"
#include "log.h"
#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const auto inline VarOrTerm = T(Var) / T(Term);
  const auto inline Numbery = T(ArithInfix) / T(UnaryExpr) / T(NumTerm);
  const auto inline CallToken =
    T(ArrayCompr) / T(ObjectCompr) / T(SetCompr) / T(ExprCall) / T(Enumerate);

  // clang-format off
  inline const auto wfi =
      (Expr <<= (Val >>= NumTerm | RefTerm | Term | UnaryExpr | ArithInfix | BoolInfix | AssignInfix))
    | (AssignArg <<= (Val >>= RefTerm | NumTerm | UnaryExpr | ArithInfix | Term | BoolInfix))
    | (ObjectItem <<= Key * Expr)
    | (RefObjectItem <<= RefTerm * Expr)
    | (ObjectCompr <<= Var * NestedBody)
    | (ArrayCompr <<= Var * NestedBody)
    | (SetCompr <<= Var * NestedBody)
    ;
  // clang-format on
}

namespace rego
{
  using namespace wf::ops;

  // Convert all Literal nodes into UnifyExpr nodes of the form <var> = <expr> |
  // <not-expr>.
  PassDef rulebody()
  {
    return {
      In(UnifyBody) *
          (T(LiteralWith) << (T(UnifyBody)[UnifyBody] * T(WithSeq)[WithSeq])) >>
        [](Match& _) { return UnifyExprWith << _(UnifyBody) << _(WithSeq); },

      In(UnifyBody) *
          (T(LiteralEnum)
           << (T(Var)[Lhs] * T(Var)[Rhs] * T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          LOG("enum");
          Location value = _.fresh({"value"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ value) << Undefined))
                     << (UnifyExprEnum << (Var ^ value) << _(Lhs) << _(Rhs)
                                       << _(UnifyBody));
        },

      In(UnifyBody) *
          (T(Literal) << ((T(Expr) << T(AssignInfix)[AssignInfix]))) >>
        [](Match& _) { return _(AssignInfix); },

      In(With) * T(Expr)[Expr] >>
        [](Match& _) {
          LOG("with");
          Location temp = _.fresh({"with"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ temp) << Undefined))
                     << (Lift << UnifyBody
                              << (UnifyExpr << (Var ^ temp) << _(Expr)))
                     << (Var ^ temp);
        },

      // <expr>|<notexpr>
      In(UnifyBody) * (T(Literal) << (T(Expr) / T(NotExpr))[Expr]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          std::string prefix = in_query(_(Expr)) ? "value" : "unify";
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
          Location unify = _.fresh({"unify"});
          return Seq << (Local << (Var ^ unify) << Undefined)
                     << (UnifyExpr << (Var ^ unify)
                                   << (Expr << wfi / _(Lhs) / Val))
                     << (UnifyExpr << (Var ^ unify)
                                   << (Expr << wfi / _(Rhs) / Val));
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
          return UnifyExpr << _(Lhs) << (Expr << wfi / _(Rhs) / Val);
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
            UnifyExpr << (Var ^ temp) << (Expr << wfi / _(Rhs) / Val));
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
              AssignInfix << (AssignArg << wfi / lhs->at(i) / Val)
                          << (AssignArg << wfi / rhs->at(i) / Val));
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
            Node key = wfi / item / Key;
            Node value = wfi / item / Expr;
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
              AssignInfix << (AssignArg << wfi / _(Lhs)->at(i) / Val)
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
              AssignInfix << (AssignArg << wfi / _(Lhs)->at(i) / Val)
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
              Node index;
              if (item->type() == RefObjectItem)
              {
                Location key = _.fresh({"key"});
                seq->push_back(Local << (Var ^ key) << Undefined);
                seq->push_back(
                  AssignInfix << (AssignArg << (RefTerm << (Var ^ key)))
                              << (AssignArg << wfi / item / RefTerm));
                index = RefTerm << (Var ^ key);
              }
              else
              {
                std::string key =
                  std::string((wfi / item / Key)->location().view());
                index = Scalar << (JSONString ^ key);
              }
              Node ref = SimpleRef << _(Rhs)->clone() << (RefArgBrack << index);
              seq->push_back(
                AssignInfix << (AssignArg << wfi / item / Expr / Val)
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
            Node index;
            if (item->type() == RefObjectItem)
            {
              Location key = _.fresh({"key"});
              seq->push_back(Local << (Var ^ key) << Undefined);
              seq->push_back(
                AssignInfix << (AssignArg << (RefTerm << (Var ^ key)))
                            << (AssignArg << wfi / item / RefTerm));
              index = RefTerm << (Var ^ key);
            }
            else
            {
              std::string key =
                std::string((wfi / item / Key)->location().view());
              index = Scalar << (JSONString ^ key);
            }

            Node ref = SimpleRef << (Var ^ temp) << (RefArgBrack << index);
            seq->push_back(
              AssignInfix << (AssignArg << wfi / item / Expr / Val)
                          << (AssignArg << (RefTerm << ref)));
          }

          return seq;
        },

      // <numbery> = <numbery>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << Numbery[Lhs]) *
               (T(AssignArg) << Numbery[Rhs]))) >>
        [](Match& _) {
          LOG("<numbery> = <numbery>");
          Location unify = _.fresh({"unify"});
          return Seq << (Local << (Var ^ unify) << Undefined)
                     << (UnifyExpr << (Var ^ unify) << (Expr << _(Lhs)))
                     << (UnifyExpr << (Var ^ unify) << (Expr << _(Rhs)));
        },

      // <call> = <ref>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << CallToken[Val]) *
               (T(AssignArg) << T(RefTerm)[RefTerm]))) >>
        [](Match& _) {
          LOG("<call> = <ref>");
          return AssignInfix << (AssignArg << _(RefTerm))
                             << (AssignArg << _(Val));
        },

      // <ref> = <call>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << T(RefTerm) << T(Var)[Var]) *
               (T(AssignArg) << CallToken[Val]))) >>
        [](Match& _) {
          LOG("<ref> = <call>");
          return UnifyExpr << _(Var) << (Expr << _(Val));
        },

      // <compr>
      In(UnifyBody) *
          (T(UnifyExpr)
           << (T(Var)[Var] *
               (T(Expr)
                << (T(Term)
                    << (T(ArrayCompr) / T(SetCompr) /
                        T(ObjectCompr))[Compr])))) >>
        [](Match& _) {
          LOG("<compr>");
          return UnifyExprCompr << _(Var)
                                << (_(Compr)->type() << (wfi / _(Compr) / Var))
                                << (wfi / _(Compr) / NestedBody);
        },

      // <compr>
      In(UnifyBody) *
          (T(UnifyExpr)
           << (T(Var)[Var] *
               (T(Expr)
                << (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr]))) >>
        [](Match& _) {
          LOG("<compr>");
          return UnifyExprCompr << _(Var)
                                << (_(Compr)->type() << (wfi / _(Compr) / Var))
                                << (wfi / _(Compr) / NestedBody);
        },

      // <compr> (other)
      T(Term) << (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr](
        [](auto& n) { return is_in(*n.first, {UnifyBody}); }) >>
        [](Match& _) {
          LOG("<compr> (other)");
          Location term = _.fresh({"term"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ term) << Undefined))
                     << (Lift
                         << UnifyBody
                         << (UnifyExpr << (Var ^ term) << (Expr << _(Compr))))
                     << (RefTerm << (Var ^ term));
        },

      // <binarg>.<setcompr>
      In(BinArg) * T(SetCompr)[SetCompr] >>
        [](Match& _) {
          LOG("<binarg>.<setcompr>");
          Location set = _.fresh({"compr"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ set) << Undefined))
                     << (Lift
                         << UnifyBody
                         << (UnifyExpr << (Var ^ set) << (Expr << _(SetCompr))))
                     << (RefTerm << (Var ^ set));
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