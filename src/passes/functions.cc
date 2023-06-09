#include "lang.h"
#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const auto inline VarOrTerm = T(Var) / T(Term);
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack);

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

      In(ArgSeq) * T(Function)[Function]([](auto& n) {
        return is_in(*n.first, UnifyBody);
      }) >>
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
              // <var>[<index>] is a special case
              // we use enumerate to access all the elements of
              // the collection and then assign the index and
              // values to the enumerated [index, value] tuple
              Location item = _.fresh({"item"});
              return Seq
                << (Lift << UnifyBody << (Local << (Var ^ item) << Undefined))
                << (Lift << UnifyBody
                         << (UnifyExpr
                             << (Var ^ item)
                             << (Enumerate << (Expr << (RefTerm << _(Var))))))
                << (Lift << UnifyBody
                         << (UnifyExpr
                             << var
                             << (RefTerm
                                 << (SimpleRef
                                     << (Var ^ item)
                                     << (RefArgBrack
                                         << (Scalar << (JSONInt ^ "0")))))))
                << (Expr
                    << (RefTerm
                        << (SimpleRef
                            << (Var ^ item)
                            << (RefArgBrack << (Scalar << (JSONInt ^ "1"))))));
            }
            else
            {
              return Function << (JSONString ^ "apply_access")
                              << (ArgSeq << _(Var) << arg);
            }
          }
          else
          {
            return Function << (JSONString ^ "apply_access")
                            << (ArgSeq << _(Var) << (Term << arg));
          }
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(ExprCall) << (T(Var)[Var] * T(ArgSeq)[ArgSeq])) >>
        [](Match& _) {
          return Function << (JSONString ^ "call")
                          << (ArgSeq << _(Var) << *_[ArgSeq]);
        },

      In(UnifyExpr) * (T(Enumerate) << T(Expr)[Expr]) >>
        [](Match& _) {
          return Function << (JSONString ^ "enumerate") << (ArgSeq << _(Expr));
        },

      (In(Array) / In(Set) / In(ObjectItem)) * (T(Expr) << T(Term)[Term]) >>
        [](Match& _) { return _(Term); },

      (In(Array) / In(Set) / In(ObjectItem)) *
          (T(Expr) << T(NumTerm)[NumTerm]) >>
        [](Match& _) { return Term << (Scalar << *_[NumTerm]); },

      // errors

      In(ObjectItem) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in object"); },

      In(Expr) * Any[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression"); },

      (In(UnifyExpr) / In(ArgSeq)) * (T(RefTerm) << T(Ref)[Ref]) >>
        [](Match& _) { return err(_(Ref), "Invalid reference"); },

      In(Array) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in array"); },

      In(Set) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in set"); },

      In(ArgSeq) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Invalid reference"); },

      In(ArgSeq) * T(Enumerate)[Enumerate] >>
        [](Match& _) { return err(_(Enumerate), "Invalid enumerate"); },

      In(Object) * T(RefObjectItem)[RefObjectItem] >>
        [](Match& _) { return err(_(RefObjectItem), "Invalid object item"); },
    };
  }
}