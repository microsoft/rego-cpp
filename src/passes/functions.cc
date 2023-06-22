#include "lang.h"
#include "passes.h"

namespace
{
  using namespace rego;
  const auto inline VarOrTerm = T(Var) / T(Term);
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack) / T(RefArgCall);
}
namespace rego
{
  PassDef functions()
  {
    return {
      (In(UnifyExpr) / In(ArgSeq)) * (T(Expr) << Any[Val]) >>
        [](Match& _) { return _(Val); },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Term) << T(Scalar)[Scalar]) >>
        [](Match& _) { return _(Scalar); },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Term) << T(Object)[Object]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"obj"});
          Node function = Function << (JSONString ^ "object")
                                   << (ArgSeq << *_[Object]);
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << function));
          seq->push_back(Var ^ temp);
          return seq;
        },

      In(ArgSeq) * T(Key)[Key] >>
        [](Match& _) { return Scalar << (JSONString ^ _(Key)); },

      (In(UnifyExpr) / In(ArgSeq)) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          seq->push_back(_(ObjectItem)->front());
          seq->push_back(_(ObjectItem)->back());
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) * T(RefObjectItem)[RefObjectItem] >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          seq->push_back(_(RefObjectItem)->front());
          seq->push_back(_(RefObjectItem)->back());
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Term) << T(Array)[Array]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"array"});
          Node function = Function << (JSONString ^ "array")
                                   << (ArgSeq << *_[Array]);
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << function));
          seq->push_back(Var ^ temp);
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Term) << T(Set)[Set]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"set"});
          Node function = Function << (JSONString ^ "set")
                                   << (ArgSeq << *_[Set]);
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << function));
          seq->push_back(Var ^ temp);
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq) * T(NumTerm)[NumTerm]) >>
        [](Match& _) { return (Scalar << _(NumTerm)->front()); },

      In(ArgSeq) * T(Function)[Function] >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"func"});
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << _(Function)));
          seq->push_back(Var ^ temp);

          return seq;
        },

      In(UnifyExpr) * (T(NotExpr) << T(Expr)[Expr]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"expr"});
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << _(Expr)));
          seq->push_back(
            Function << (JSONString ^ "not") << (ArgSeq << (Var ^ temp)));

          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(UnaryExpr) << T(ArithArg)[ArithArg]) >>
        [](Match& _) {
          return Function << (JSONString ^ "unary")
                          << (ArgSeq << _(ArithArg)->front());
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(ArithInfix) << (T(ArithArg)[Lhs] * Any[Op] * T(ArithArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "arithinfix")
                          << (ArgSeq << _(Op) << _(Lhs)->front()
                                     << _(Rhs)->front());
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(BoolInfix) << (T(BoolArg)[Lhs] * Any[Op] * T(BoolArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "boolinfix")
                          << (ArgSeq << _(Op) << _(Lhs)->front()
                                     << _(Rhs)->front());
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(RefTerm) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(RefTerm)
           << (T(SimpleRef) << (T(Var)[Var] * (T(RefArgDot)[RefArgDot])))) >>
        [](Match& _) {
          Location field_name = _(RefArgDot)->front()->location();
          Node arg = Scalar << (JSONString ^ field_name);
          return Function << (JSONString ^ "apply_access")
                          << (ArgSeq << _(Var) << arg);
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(RefTerm)
           << (T(SimpleRef)
               << (T(Var)[Var] * (T(RefArgBrack)[RefArgBrack])))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);

          Node arg = _(RefArgBrack)->front();
          if (arg->type() == RefTerm)
          {
            Node var = arg->front();
            if (var->type() == Var && contains_local(var))
            {
              Node function = Function << (JSONString ^ "access_args")
                                       << (ArgSeq << _(Var)->clone());
              seq->push_back(
                Lift << UnifyBody << (UnifyExpr << arg->clone() << function));

              arg = var;
            }
          }
          else
          {
            arg = Term << arg;
          }
          seq->push_back(
            Function << (JSONString ^ "apply_access")
                     << (ArgSeq << _(Var) << arg));
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(RefTerm)
           << (T(SimpleRef) << (T(Var)[Var] * (T(RefArgCall)[RefArgCall])))) >>
        [](Match& _) {
          Node args = _(Head);
          return Function << (JSONString ^ "call")
                          << (ArgSeq << _(Var) << *_[RefArgCall]);
        },

      (In(Array) / In(Set) / In(ObjectItem)) * (T(Expr) << T(Term)[Term]) >>
        [](Match& _) { return _(Term); },

      (In(Array) / In(Set) / In(ObjectItem)) *
          (T(Expr) << T(NumTerm)[NumTerm]) >>
        [](Match& _) { return Term << (Scalar << *_[NumTerm]); },

      // errors

      In(ObjectItem) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in object"); },

      (In(UnifyExpr) / In(ArgSeq)) * (T(RefTerm) << T(Ref)[Ref]) >>
        [](Match& _) { return err(_(Ref), "Invalid reference"); },

      In(Array) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in array"); },

      In(Set) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in set"); },

      In(ArgSeq) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Invalid reference"); },
    };
  }
}