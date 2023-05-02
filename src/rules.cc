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
    Memos memos = std::make_shared<MemosDef>();
    return {
      T(Expression) << (Result[Value] * End) >>
        [](Match& _) { return _(Value); },

      T(Ref) << T(Local)[Id]([](auto& n) { return can_replace(n); }) >>
        [](Match& _) {
          auto defs = _(Id)->lookup();
          auto rule = defs.front();
          return rule->at(1)->clone();
        },

      T(Ref) << T(Lookup)[Lookup]([memos](auto& n) {
        Node node = *n.first;
        std::string key = std::string(node->location().view());
        if (memos->count(key))
        {
          return true;
        }

        return can_replace(n);
      }) >>
        [memos](Match& _) {
          auto lookup = _(Lookup);
          std::string key = std::string(lookup->location().view());
          if (memos->count(key) == 0)
          {
            auto result = search(lookup);
            if (result->type() == Module)
            {
              result = module_to_object(result);
            }
            memos->insert(std::make_pair(key, result->clone()));
          }

          auto value = memos->at(key)->clone();
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