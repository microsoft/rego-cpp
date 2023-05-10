#include "lookup.h"
#include "math.h"
#include "passes.h"

namespace
{
  typedef std::map<std::string, trieste::Node> MemosDef;
  typedef std::shared_ptr<MemosDef> Memos;
}

namespace rego
{
  const inline auto Number = T(JSONInt) / T(JSONFloat);
  const inline auto Truthy =
    Number / T(JSONNull) / T(String) / T(Object) / T(Array);

  Node module_to_object(const Node module)
  {
    Node object = Node(Object);
    for (auto& rule : *module)
    {
      std::string key(rule->at(0)->location().view());
      Node value = rule->at(1)->clone();
      Node object_item = ObjectItem << (String << (JSONString ^ key)) << value;
      object->push_back(object_item);
    }

    return object;
  }

  PassDef rules()
  {
    Memos memos = std::make_shared<MemosDef>();
    return {
      T(RefTerm) << T(Ref)[Ref]([memos](auto& n) {
        Node node = *n.first;
        std::string key = std::string(node->location().view());
        if (memos->count(key))
        {
          return true;
        }

        return can_replace(n);
      }) >>
        [memos](Match& _) {
          auto ref = _(Ref);
          std::string key = std::string(ref->location().view());
          if (memos->count(key) == 0)
          {
            auto result = search(ref);
            if (result->type() == Module)
            {
              result = module_to_object(result);
            }
            memos->insert(std::make_pair(key, result->clone()));
          }

          auto value = memos->at(key)->clone();
          return value->clone();
        },

      T(RefTerm) << T(Var)[Var]([](auto& n) { return can_replace(n); }) >>
        [](Match& _) {
          auto defs = _(Var)->lookup();
          if (defs.size() == 0)
          {
            return Term << Undefined;
          }

          auto rule = defs.front();
          return rule->at(1)->clone();
        },

      T(Term) << T(Term)[Term] >> [](Match& _) { return _(Term); },

      T(Literal) << T(Term)[Term] >> [](Match& _) { return _(Term); },

      (In(RuleComp) / In(Literal) / In(ObjectItem) / In(Array) / In(RuleBody)) *
          (T(Expr) << T(Term)[Term]) >>
        [](Match& _) { return _(Term); },

      In(Expr) * (T(NumTerm) << Number[Value]) >>
        [](Match& _) { return Term << (Scalar << _(Value)); },

      In(Expr) * (T(NumTerm) << T(Undefined)) >>
        [](Match&) { return Term << Undefined; },

      (In(ObjectItem) / In(RuleComp) / In(Literal) / In(RuleBody)) *
          (T(NumTerm) << Number[Value]) >>
        [](Match& _) { return Term << (Scalar << _(Value)); },

      (In(ObjectItem) / In(RuleComp) / In(Literal) / In(RuleBody)) *
          (T(NumTerm) << T(Undefined)) >>
        [](Match&) { return Term << Undefined; },

      (In(Expr) / In(ArithArg)) *
          (T(UnaryExpr) << (T(NumTerm) << Number[Value])) >>
        [](Match& _) {
          auto value = negate(_(Value));
          return NumTerm << value;
        },

      (In(Expr) / In(ArithArg)) *
          (T(UnaryExpr) << (T(NumTerm) << T(Undefined))) >>
        [](Match&) { return NumTerm << Undefined; },

      T(ArithArg) << T(NumTerm)[NumTerm] >> [](Match& _) { return _(NumTerm); },

      T(ArithArg) << (T(Term) << (T(Scalar) << Number[Value])) >>
        [](Match& _) { return NumTerm << _(Value); },

      T(ArithArg) << (T(Term) << T(Undefined)) >>
        [](Match&) { return NumTerm << Undefined; },

      (In(Expr) / In(ArithArg)) *
          (T(ArithInfix)
           << ((T(NumTerm) << T(JSONInt)[Lhs]) * ArithToken[Op] *
               (T(NumTerm) << T(JSONInt)[Rhs]))) >>
        [](Match& _) {
          int lhs = get_int(_(Lhs));
          int rhs = get_int(_(Rhs));
          auto value = math(_(Op), lhs, rhs);
          return NumTerm << value;
        },

      (In(Expr) / In(ArithArg)) *
          (T(ArithInfix)
           << ((T(NumTerm) << Number[Lhs]) * ArithToken[Op] *
               (T(NumTerm) << Number[Rhs]))) >>
        [](Match& _) {
          double lhs = get_double(_(Lhs));
          double rhs = get_double(_(Rhs));
          auto value = math(_(Op), lhs, rhs);
          return Term << (Scalar << value);
        },

      (In(Expr) / In(ArithArg)) *
          (T(ArithInfix)
           << (Any * ArithToken * (T(NumTerm) << T(Undefined)))) >>
        [](Match&) { return Term << Undefined; },

      (In(Expr) / In(ArithArg)) *
          (T(ArithInfix)
           << ((T(NumTerm) << T(Undefined)) * ArithToken * Any)) >>
        [](Match&) { return Term << Undefined; },

      In(Expr) *
          (T(BoolInfix)
           << ((T(NumTerm) << T(JSONInt)[Lhs]) * BoolToken[Op] *
               (T(NumTerm) << T(JSONInt)[Rhs]))) >>
        [](Match& _) {
          int lhs = get_int(_(Lhs));
          int rhs = get_int(_(Rhs));
          auto value = compare(_(Op), lhs, rhs);
          return Term << (Scalar << value);
        },

      In(Expr) *
          (T(BoolInfix)
           << ((T(NumTerm) << Number[Lhs]) * BoolToken[Op] *
               (T(NumTerm) << Number[Rhs]))) >>
        [](Match& _) {
          double lhs = get_double(_(Lhs));
          double rhs = get_double(_(Rhs));
          auto value = compare(_(Op), lhs, rhs);
          return Term << (Scalar << value);
        },

      In(Expr) *
          (T(BoolInfix) << (Any * BoolToken * (T(NumTerm) << T(Undefined)))) >>
        [](Match&) { return Term << Undefined; },

      In(Expr) *
          (T(BoolInfix) << ((T(NumTerm) << T(Undefined)) * BoolToken * Any)) >>
        [](Match&) { return Term << Undefined; },

      In(RuleBodySeq) * (T(RuleBody) << End) >>
        [](Match&) { return RuleBody << JSONTrue; },

      In(RuleBody) * (T(RuleComp)[RuleComp] << (T(Var) * T(Term)))([](auto& n) {
        return can_remove(n);
      }) >>
        ([](Match&) -> Node { return JSONTrue; }),

      In(RuleBody) * (T(Term) << (T(Scalar) << Any[Value])) >>
        [](Match& _) { return _(Value); },

      In(RuleBody) * (T(Term) << (T(Array) / T(Object))[Value]) >>
        [](Match& _) { return _(Value); },

      In(RuleBody) * (T(Term) << T(Undefined)) >>
        ([](Match&) -> Node { return JSONFalse; }),

      In(RuleBody) * Truthy >> ([](Match&) -> Node { return JSONTrue; }),

      In(RuleBody) * (T(JSONTrue) * T(JSONTrue)) >>
        ([](Match&) -> Node { return JSONTrue; }),

      In(RuleBody) * ((T(JSONTrue) / T(JSONFalse)) * T(JSONFalse)) >>
        ([](Match&) -> Node { return JSONFalse; }),

      In(RuleBody) * (T(JSONFalse) * (T(JSONTrue) / T(JSONFalse))) >>
        ([](Match&) -> Node { return JSONFalse; }),

      In(RuleComp) *
          (T(RuleBodySeq) << ((T(RuleBody) << (T(JSONTrue) * End)) * End)) >>
        ([](Match&) -> Node { return JSONTrue; }),

      In(RuleComp) *
          (T(RuleBodySeq) << ((T(RuleBody) << T(JSONFalse)) * End)) >>
        ([](Match&) -> Node { return JSONFalse; }),

      // errors
    };
  }
}