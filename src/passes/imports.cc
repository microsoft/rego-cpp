#include "passes.h"

namespace
{
  using namespace trieste;

  bool loc_equals(const Node& node, const std::string& value)
  {
    return node->location().view() == value;
  }
}

namespace rego
{
  PassDef imports()
  {
    return {
      In(ImportSeq) *
          (T(Import)
           << (T(Group)
               << (T(Var)(
                     [](auto& n) { return loc_equals(*n.first, "future"); }) *
                   T(Dot) * T(Var)([](auto& n) {
                     return loc_equals(*n.first, "keywords");
                   }) *
                   T(Dot) * T(Var)[Keyword] * End))) >>
        [](Match& _) { return Keyword << _(Keyword); },

      In(ImportSeq) *
          (T(Import)
           << (T(Group)
               << (T(Var)(
                     [](auto& n) { return loc_equals(*n.first, "future"); }) *
                   T(Dot) * T(Var)([](auto& n) {
                     return loc_equals(*n.first, "keywords");
                   }) *
                   End))) >>
        [](Match&) {
          Node seq = NodeDef::create(Seq);
          for (auto& keyword : Keywords)
          {
            seq->push_back(Keyword << (Var ^ keyword));
          }
          return seq;
        },

      // errors
      In(ImportSeq) * T(Import)[Import] >>
        [](Match& _) { return err(_(Import), "Invalid import"); },
    };
  }
}