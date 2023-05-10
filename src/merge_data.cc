#include "passes.h"

namespace rego
{
  Node merge_objects(const Node& lhs, const Node& rhs)
  {
    Node list = NodeDef::create(ObjectItemList);
    std::set<std::string> keys;
    for (auto object_item : *lhs)
    {
      std::string key(object_item->front()->location().view());
      keys.insert(key);
      list->push_back(object_item);
    }

    for (auto object_item : *rhs)
    {
      std::string key(object_item->front()->location().view());
      if (keys.count(key))
      {
        return err(rhs, "Merge error");
      }

      list->push_back(object_item);
    }

    return list;
  }

  PassDef merge_data()
  {
    return {
      In(DataSeq) * (T(Data)[Lhs] * T(Data)[Rhs]) >>
        [](Match& _) {
          auto lhs = _(Lhs);
          auto rhs = _(Rhs);
          return Data << merge_objects(_(Lhs)->front(), _(Rhs)->front());
        },

      In(Rego) * (T(DataSeq) << (T(Data)[Data] * End)) >>
        [](Match& _) { return Data << (Var ^ "data") << *_[Data]; },

      In(Rego) * (T(DataSeq) << End) >>
        [](Match&) { return Data << (Var ^ "data") << ObjectItemList; },

      // errors

      T(Expr)[Expr] << (Any * Any) >>
        [](Match& _) { return err(_(Expr), "Invalid expression"); },
    };
  }

}