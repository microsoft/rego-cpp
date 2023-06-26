#include "lang.h"
#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const auto inline VarOrTerm = T(Var) / T(Term);
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack) / T(RefArgCall);

  // clang-format off
  inline const auto wfi =
      (ObjectItem <<= Key * Expr)
    | (RefObjectItem <<= RefTerm * Expr)
    | (NumTerm <<= JSONInt | JSONFloat)
    | (ArithArg <<= RefTerm | NumTerm | UnaryExpr | ArithInfix)
    | (BoolArg <<= Term | RefTerm | NumTerm | UnaryExpr | ArithInfix)
    | (RefArgDot <<= Var)
    | (RefArgBrack <<= Scalar | Var | Object | Array | Set)
    | (RefTerm <<= SimpleRef)
    ;
  // clang-format on
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
          seq->push_back(_(ObjectItem)->at(wfi / ObjectItem / Key));
          seq->push_back(_(ObjectItem)->at(wfi / ObjectItem / Expr));
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) * T(RefObjectItem)[RefObjectItem] >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          seq->push_back(_(RefObjectItem)->at(wfi / RefObjectItem / RefTerm));
          seq->push_back(_(RefObjectItem)->at(wfi / RefObjectItem / Expr));
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
        [](Match& _) {
          return (Scalar << _(NumTerm)->at(wfi / NumTerm / NumTerm));
        },

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
                          << (ArgSeq
                              << _(ArithArg)->at(wfi / ArithArg / ArithArg));
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(ArithInfix) << (T(ArithArg)[Lhs] * Any[Op] * T(ArithArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "arithinfix")
                          << (ArgSeq << _(Op)
                                     << _(Lhs)->at(wfi / ArithArg / ArithArg)
                                     << _(Rhs)->at(wfi / ArithArg / ArithArg));
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(BoolInfix) << (T(BoolArg)[Lhs] * Any[Op] * T(BoolArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "boolinfix")
                          << (ArgSeq << _(Op)
                                     << _(Lhs)->at(wfi / BoolArg / BoolArg)
                                     << _(Rhs)->at(wfi / BoolArg / BoolArg));
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(RefTerm) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(RefTerm)
           << (T(SimpleRef) << (T(Var)[Var] * (T(RefArgDot)[RefArgDot])))) >>
        [](Match& _) {
          Location field_name =
            _(RefArgDot)->at(wfi / RefArgDot / Var)->location();
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

          Node arg = _(RefArgBrack)->at(wfi / RefArgBrack / RefArgBrack);
          if (arg->type() == RefTerm)
          {
            Node var = arg->at(wfi / RefTerm / SimpleRef);
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