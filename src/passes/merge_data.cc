#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  inline const auto wfi =
      (ObjectItem <<= Key * Expr)
    | (Data <<= ObjectItemSeq)
    ;
  // clang-format on
}

namespace rego
{
  Node merge_objects(const Node& lhs, const Node& rhs)
  {
    Node list = NodeDef::create(ObjectItemSeq);
    std::set<std::string> keys;
    for (auto object_item : *lhs)
    {
      std::string key(
        object_item->at(wfi / ObjectItem / Key)->location().view());
      keys.insert(key);
      list->push_back(object_item);
    }

    for (auto object_item : *rhs)
    {
      std::string key(
        object_item->at(wfi / ObjectItem / Key)->location().view());
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
          return Data << merge_objects(
                   _(Lhs)->at(wfi / Data / ObjectItemSeq),
                   _(Rhs)->at(wfi / Data / ObjectItemSeq));
        },

      In(Rego) * (T(DataSeq) << (T(Data)[Data] * End)) >>
        [](Match& _) { return Data << (Var ^ "data") << *_[Data]; },

      In(Rego) * (T(DataSeq) << End) >>
        [](Match&) { return Data << (Var ^ "data") << ObjectItemSeq; },

      // errors
    };
  }

}