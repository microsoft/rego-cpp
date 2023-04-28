#include "lookup.h"
#include "math.h"
#include "passes.h"

namespace rego
{
  Node module_to_object(const Node module)
  {
    Node object = Node(Object);
    for (auto& rule : *module)
    {
      std::string key(rule->at(0)->location().view());
      Node value = rule->at(1)->clone();
      Node keyvalue = KeyValue << (Key ^ key) << value;
      object->push_back(keyvalue);
    }

    return object;
  }

  PassDef rules()
  {
    return {
      T(Expression) << (Result[Value] * End) >>
        [](Match& _) { return _(Value); },

      T(Ref) << T(Local)[Id]([](auto& n) { return can_replace(n); }) >>
        [](Match& _) {
          auto defs = _(Id)->lookup();
          auto rule = defs.front();
          return rule->at(1)->clone();
        },

      T(Ref) << T(Lookup)[Lookup]([](auto& n) { return can_replace(n); }) >>
        [](Match& _) {
          auto lookup = _(Lookup);
          auto value = search(lookup);
          if (value->type() == Module)
          {
            value = module_to_object(value);
          }

          return value->clone();
        },

      T(RuleValue) << Result[Value] >> [](Match& _) { return _(Value); },

      In(RuleBody) * Truthy >> [](Match&) { return Bool ^ "true"; },

      In(RuleBody) * T(Bool, "true") >> [](Match&) -> Node { return {}; },

      In(RuleBody) * Falsy >> [](Match&) { return Bool ^ "false"; },

      In(RuleBody) * (Any * T(Bool, "false")) >>
        [](Match&) { return Bool ^ "false"; },

      T(RuleBody) << End >> [](Match&) { return Node(RuleSeq); },

      T(RuleBody) << (T(Rule)[Head] * T(Rule)++[Tail] * End) >>
        [](Match& _) { return RuleSeq << _(Head) << _[Tail]; },

      T(RuleBody) << T(Bool, "false") >> [](Match&) { return Node(Undefined); },

      T(Rule) << (T(Ident) * Any * T(Undefined)) >>
        [](Match&) -> Node { return {}; },

      T(Math) << (MathOp[Op] * T(Int)[Lhs] * T(Int)[Rhs]) >>
        [](Match& _) {
          int lhs = get_int(_(Lhs));
          int rhs = get_int(_(Rhs));
          return math(_(Op), lhs, rhs);
        },

      T(Math)
          << (MathOp[Op] * (T(Int) / T(Float))[Lhs] *
              (T(Int) / T(Float))[Rhs]) >>
        [](Match& _) {
          double lhs = get_double(_(Lhs));
          double rhs = get_double(_(Rhs));
          return math(_(Op), lhs, rhs);
        },

      T(Math) << (MathOp * Any * T(Undefined)) >>
        [](Match&) { return Node(Undefined); },

      T(Math) << (MathOp * T(Undefined)) >>
        [](Match&) { return Node(Undefined); },

      T(Comparison) << (CompOp[Op] * T(Int)[Lhs] * T(Int)[Rhs]) >>
        [](Match& _) {
          int lhs = get_int(_(Lhs));
          int rhs = get_int(_(Rhs));
          return compare(_(Op), lhs, rhs);
        },

      T(Comparison)
          << (CompOp[Op] * (T(Int) / T(Float))[Lhs] *
              (T(Int) / T(Float))[Rhs]) >>
        [](Match& _) {
          double lhs = get_double(_(Lhs));
          double rhs = get_double(_(Rhs));
          return compare(_(Op), lhs, rhs);
        },

      T(Comparison) << (CompOp * Any * T(Undefined)) >>
        [](Match&) { return Node(Undefined); },

      T(Comparison) << (CompOp * T(Undefined)) >>
        [](Match&) { return Node(Undefined); },

      // errors
      T(Ref) << T(Local)[Id]([](auto& n) { return !exists(n); }) >>
        [](Match&) { return Node(Undefined); },

      In(Math) * NotANumber[Value] >>
        [](Match& _) { return err(_(Value), "not a valid operand"); },

      In(Math) * (Any * T(Error)[Error]) >> [](Match& _) { return _(Error); },

      T(Math) << T(Error)[Error] >> [](Match& _) { return _(Error); },

      In(Comparison) * NotANumber[Value] >>
        [](Match& _) { return err(_(Value), "not a valid operand"); },

      In(Comparison) * (Any * T(Error)[Error]) >>
        [](Match& _) { return _(Error); },

      T(Comparison) << T(Error)[Error] >> [](Match& _) { return _(Error); },

      T(Expression) << T(Error)[Error] >> [](Match& _) { return _(Error); },
    };
  }

}