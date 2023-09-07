#include "errors.h"
#include "helpers.h"
#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const auto inline VarOrTerm = T(Var) / T(Term);
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack);
}
namespace rego
{

  // Converts all UnifyExpr statements to be of either <var> = <var>,
  // <var> = <term>, or <var> = <function> forms, where <function> is a named
  // function that takes either <var> or <term> arguments.
  PassDef functions()
  {
    return {
      In(Input) * T(DataTerm)[DataTerm] >>
        [](Match& _) { return Term << *_[DataTerm]; },

      In(UnifyExpr, ArgSeq) * (T(Expr) << Any[Val]) >>
        [](Match& _) { return _(Val); },

      In(UnifyExpr, ArgSeq) * (T(Term) << T(Scalar)[Scalar]) >>
        [](Match& _) { return _(Scalar); },

      In(UnifyExpr, ArgSeq) * (T(Term) << T(Object)[Object]) >>
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

      In(ArgSeq) * T(Set)[Set] >> [](Match& _) { return Term << _(Set); },

      In(UnifyExpr, ArgSeq) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) { return Seq << *_[ObjectItem]; },

      In(UnifyExpr, ArgSeq) * (T(Enumerate) << T(Expr)[Expr]) >>
        [](Match& _) {
          return Function << (JSONString ^ "enumerate") << (ArgSeq << _(Expr));
        },

      In(UnifyExpr, ArgSeq) *
          (T(Membership)
           << (T(Expr)[Idx] * T(Expr)[Item] * T(Expr)[ItemSeq])) >>
        [](Match& _) {
          return Function << (JSONString ^ "membership-tuple")
                          << (ArgSeq << _(Idx) << _(Item) << _(ItemSeq));
        },

      In(UnifyExpr, ArgSeq) *
          (T(Membership)
           << (T(Undefined) * T(Expr)[Item] * T(Expr)[ItemSeq])) >>
        [](Match& _) {
          return Function << (JSONString ^ "membership-single")
                          << (ArgSeq << _(Item) << _(ItemSeq));
        },

      In(UnifyExpr, ArgSeq) * (T(Term) << T(Array)[Array]) >>
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

      In(UnifyExpr, ArgSeq) * (T(Term) << T(Set)[Set]) >>
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

      In(UnifyExpr, ArgSeq) *
          (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr] >>
        [](Match& _) {
          std::string name = _(Compr)->type().str();
          std::transform(
            name.begin(), name.end(), name.begin(), [](unsigned char c) {
              return static_cast<char>(std::tolower(c));
            });
          Location temp = _.fresh({name});
          return Function << (JSONString ^ name) << (ArgSeq << *_[Compr]);
          ;
        },

      In(UnifyExpr, ArgSeq) *
          (T(Term) << (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr)))[Compr] >>
        [](Match& _) {
          std::string name = _(Compr)->type().str();
          std::transform(
            name.begin(), name.end(), name.begin(), [](unsigned char c) {
              return static_cast<char>(std::tolower(c));
            });
          Location temp = _.fresh({name});
          return Function << (JSONString ^ name) << (ArgSeq << *_[Compr]);
          ;
        },

      In(UnifyExpr, ArgSeq) * (T(Merge) << T(Var)[Var]) >>
        [](Match& _) {
          return Function << (JSONString ^ "merge") << (ArgSeq << _(Var));
        },

      (In(UnifyExpr, ArgSeq) * T(NumTerm)[NumTerm]) >>
        [](Match& _) { return Scalar << _(NumTerm)->front(); },

      In(ArgSeq) * T(Function)[Function]([](auto& n) {
        return is_in(*n.first, {UnifyBody});
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

      In(UnifyExpr, ArgSeq) * (T(Not) << T(Expr)[Expr]) >>
        [](Match& _) {
          return Function << (JSONString ^ "not") << (ArgSeq << _(Expr));
        },

      In(UnifyExpr, ArgSeq) * (T(UnaryExpr) << T(ArithArg)[ArithArg]) >>
        [](Match& _) {
          return Function << (JSONString ^ "unary")
                          << (ArgSeq << _(ArithArg)->front());
        },

      In(UnifyExpr, ArgSeq) *
          (T(ArithInfix) << (T(ArithArg)[Lhs] * Any[Op] * T(ArithArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "arithinfix")
                          << (ArgSeq << _(Op) << _(Lhs)->front()
                                     << _(Rhs)->front());
        },

      In(UnifyExpr, ArgSeq) *
          (T(BinInfix) << (T(BinArg)[Lhs] * Any[Op] * T(BinArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "bininfix")
                          << (ArgSeq << _(Op) << _(Lhs)->front()
                                     << _(Rhs)->front());
        },

      In(UnifyExpr, ArgSeq) *
          (T(BoolInfix) << (T(BoolArg)[Lhs] * Any[Op] * T(BoolArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "boolinfix")
                          << (ArgSeq << _(Op) << _(Lhs)->front()
                                     << _(Rhs)->front());
        },

      In(UnifyExpr, ArgSeq) * (T(RefTerm) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

      In(UnifyExpr, ArgSeq) *
          (T(RefTerm)
           << (T(SimpleRef) << (T(Var)[Var] * (T(RefArgDot)[RefArgDot])))) >>
        [](Match& _) {
          Location field_name = _(RefArgDot)->front()->location();
          Node arg = Scalar << (JSONString ^ field_name);
          return Function << (JSONString ^ "apply_access")
                          << (ArgSeq << _(Var) << arg);
        },

      In(UnifyExpr, ArgSeq) *
          (T(RefTerm)
           << (T(SimpleRef)
               << (T(Var)[Var] * (T(RefArgBrack)[RefArgBrack])))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Node arg = _(RefArgBrack)->front();
          if (arg->type() == RefTerm || arg->type() == Expr)
          {
            return Function << (JSONString ^ "apply_access")
                            << (ArgSeq << _(Var) << arg);
          }
          else
          {
            return Function << (JSONString ^ "apply_access")
                            << (ArgSeq << _(Var) << (Term << arg));
          }
        },

      In(UnifyExpr, ArgSeq) *
          (T(ExprCall) << (T(Var)[Var] * T(ArgSeq)[ArgSeq])) >>
        [](Match& _) {
          return Function << (JSONString ^ "call")
                          << (ArgSeq << _(Var) << *_[ArgSeq]);
        },

      In(Array, Set, ObjectItem) * (T(Expr) << T(Term)[Term]) >>
        [](Match& _) { return _(Term); },

      In(Array, Set, ObjectItem) * (T(Expr) << T(NumTerm)[NumTerm]) >>
        [](Match& _) { return Term << (Scalar << *_[NumTerm]); },

      In(RuleComp, RuleFunc, RuleObj, RuleSet, DataItem) *
          T(DataTerm)[DataTerm] >>
        [](Match& _) { return Term << *_[DataTerm]; },

      In(Term) * T(DataArray)[DataArray] >>
        [](Match& _) { return Array << *_[DataArray]; },

      In(Term) * T(DataSet)[DataSet] >>
        [](Match& _) { return Set << *_[DataSet]; },

      In(Term) * T(DataObject)[DataObject] >>
        [](Match& _) { return Object << *_[DataObject]; },

      In(Object) * T(DataObjectItem)[DataObjectItem] >>
        [](Match& _) { return ObjectItem << *_[DataObjectItem]; },

      In(ObjectItem, Array, Set) * T(DataTerm)[DataTerm] >>
        [](Match& _) { return Term << *_[DataTerm]; },

      // errors

      In(ObjectItem) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in object"); },

      In(Expr) * Any[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression"); },

      In(UnifyExpr, ArgSeq) * (T(RefTerm) << T(Ref)[Ref]) >>
        [](Match& _) { return err(_(Ref), "Invalid reference"); },

      In(Array) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in array"); },

      In(Set) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in set"); },

      In(ArgSeq) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Invalid reference"); },

      In(ObjectItem) * T(DataModule)[DataModule] >>
        [](Match& _) {
          return err(
            _(DataModule),
            "Syntax error: module not allowed as object item value");
        },
    };
  }
}