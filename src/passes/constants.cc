#include "internal.hh"

namespace rego
{
  // Converts all rules with constant terms to be DataTerm nodes.
  // Also reshapes ExprEvery nodes so they can be lifted later.
  PassDef constants()
  {
    return {
      In(RuleComp, RuleFunc, RuleSet, DefaultRule) *
          T(Term)[Term]([](auto& n) { return is_constant(*n.first); }) >>
        [](Match& _) { return DataTerm << *_[Term]; },

      In(RuleComp, RuleFunc) *
          T(Term)[Term]([](auto& n) { return !is_constant(*n.first); }) >>
        [](Match& _) {
          Location value = _.fresh({"value"});
          return UnifyBody
            << (Literal
                << (Expr << (RefTerm << (Var ^ value)) << Unify << _(Term)));
        },

      In(RuleSet) *
          T(Term)[Term]([](auto& n) { return !is_constant(*n.first); }) >>
        [](Match& _) { return Expr << _(Term); },

      In(RuleObj) * (T(Term)[Key] * T(Term)[Val])([](auto& n) {
        Node key = n.first[0];
        Node val = n.first[1];
        return is_constant(key) && is_constant(val);
      }) >>
        [](Match& _) {
          std::string key = strip_quotes(to_json(_(Key)));
          return Seq << (DataTerm << *_[Key]) << (DataTerm << *_[Val]);
        },

      In(RuleObj) * (T(Term)[Key] * T(Term)[Val])([](auto& n) {
        Node key = n.first[0];
        Node val = n.first[1];
        return !is_constant(key) || !is_constant(val);
      }) >>
        [](Match& _) { return Seq << (Expr << _(Key)) << (Expr << _(Val)); },

      In(RuleObj) * (T(Expr)[Key] * T(Term)[Val]) >>
        [](Match& _) { return Seq << _(Key) << (Expr << _(Val)); },

      In(RuleObj) * (T(Term)[Key] * T(Expr)[Val]) >>
        [](Match& _) { return Seq << (Expr << _(Key)) << _(Val); },

      In(DataTerm) * T(Array)[Array] >>
        [](Match& _) { return DataArray << *_[Array]; },

      In(DataTerm) * T(Set)[Set] >> [](Match& _) { return DataSet << *_[Set]; },

      In(DataTerm) * T(Object)[Object] >>
        [](Match& _) { return DataObject << *_[Object]; },

      In(DataArray, DataSet) * (T(Expr) << T(Expr)[Expr]) >>
        [](Match& _) { return _(Expr); },

      In(DataArray, DataSet) * (T(Expr) << T(Term)[Term]) >>
        [](Match& _) { return DataTerm << _(Term)->front(); },

      In(DataArray, DataSet) * (T(Expr) << T(Set)[Set]) >>
        [](Match& _) { return DataTerm << _(Set); },

      In(DataArray, DataSet) * (T(Expr) << T(NumTerm)[NumTerm]) >>
        [](Match& _) { return DataTerm << (Scalar << _(NumTerm)->front()); },

      In(DataObject) *
          (T(ObjectItem)
           << ((T(Expr) << (T(Term) / T(NumTerm) / T(Set))[Key]) *
               (T(Expr) << (T(Term) / T(NumTerm) / T(Set))[Val]))) >>
        [](Match& _) {
          Node key = _(Key);
          if (key->type() == NumTerm)
          {
            key = Scalar << key->front();
          }
          else if (key->type() == Set)
          {
            key = DataTerm << key;
          }
          else
          {
            key = key->front();
          }

          Node val = _(Val);
          if (val->type() == NumTerm)
          {
            val = Scalar << val->front();
          }
          else if (val->type() == Set)
          {
            val = DataTerm << val;
          }
          else
          {
            val = val->front();
          }

          return DataObjectItem << (DataTerm << key) << (DataTerm << val);
        },

      In(Expr) *
          (T(ExprEvery)([](auto& n) {
             return is_in(*n.first, {Policy}) && is_in(*n.first, {UnifyBody});
           })
           << ((T(VarSeq) << (T(Var)[Val] * End)) * T(UnifyBody)[UnifyBody] *
               (T(IsIn) << T(Expr)[Expr]))) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          Location every = _.fresh({"every"});

          return ExprEvery
            << (UnifyBody
                << (Local << (Var ^ item) << Undefined)
                << (LiteralNot
                    << (UnifyBody
                        << (LiteralEnum << (Var ^ item) << _(Expr))
                        << (Literal
                            << (Expr << (RefTerm << _(Val)) << Unify
                                     << (RefTerm
                                         << (Ref << (RefHead << (Var ^ item))
                                                 << (RefArgSeq
                                                     << (RefArgBrack
                                                         << (Scalar
                                                             << (Int ^
                                                                 "1"))))))))
                        << (LiteralNot << _(UnifyBody)))));
        },

      In(Expr) *
          (T(ExprEvery)([](auto& n) {
             return is_in(*n.first, {Policy}) && is_in(*n.first, {UnifyBody});
           })
           << ((T(VarSeq) << (T(Var)[Idx] * T(Var)[Val] * End)) *
               T(UnifyBody)[UnifyBody] * (T(IsIn) << T(Expr)[Expr]))) >>
        [](Match& _) {
          Location item = _.fresh({"item"});
          Location every = _.fresh({"every"});

          return ExprEvery
            << (UnifyBody
                << (Local << (Var ^ item) << Undefined)
                << (LiteralNot
                    << (UnifyBody
                        << (LiteralEnum << (Var ^ item) << _(Expr))
                        << (Literal
                            << (Expr << (RefTerm << _(Idx)->clone()) << Unify
                                     << (RefTerm
                                         << (Ref << (RefHead << (Var ^ item))
                                                 << (RefArgSeq
                                                     << (RefArgBrack
                                                         << (Scalar
                                                             << (Int ^
                                                                 "0"))))))))

                        << (Literal
                            << (Expr << (RefTerm << _(Val)->clone()) << Unify
                                     << (RefTerm
                                         << (Ref << (RefHead << (Var ^ item))
                                                 << (RefArgSeq
                                                     << (RefArgBrack
                                                         << (Scalar
                                                             << (Int ^
                                                                 "1"))))))))
                        << (LiteralNot << _(UnifyBody)))));
        },

      In(ModuleSeq) * (T(Module)[Lhs] * T(Module)[Rhs])([](auto& n) {
        Node lhs = n.first[0];
        Node rhs = n.first[1];
        auto lhs_package = flatten_ref((lhs / Package)->front());
        auto rhs_package = flatten_ref((rhs / Package)->front());

        return lhs_package == rhs_package;
      }) >>
        [](Match& _) {
          Node pkg = _(Lhs) / Package;
          Node policy = _(Lhs) / Policy;
          Node rhs_policy = _(Rhs) / Policy;
          policy->push_back({rhs_policy->begin(), rhs_policy->end()});
          return Module << pkg << policy;
        },

      // errors
    };
  }
}