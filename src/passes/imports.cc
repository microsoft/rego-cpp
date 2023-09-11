#include "internal.hh"

namespace
{
  using namespace rego;

  bool loc_equals(const Node& node, const std::string& value)
  {
    return node->location().view() == value;
  }

  const inline auto ImportToken = T(Var) / T(Dot) / T(Square);
}

namespace rego
{
  // Handles import statements, grouping them into an ImportSeq node. Also
  // transforms With statements to give them additional structure.
  PassDef imports()
  {
    return {
      In(With) *
          (T(Group)[RuleRef] * (T(Group) << (T(As) * Any++[WithExpr]))) >>
        [](Match& _) {
          return Seq << (RuleRef << _(RuleRef))
                     << (WithExpr << (Group << _[WithExpr]));
        },

      In(ImportSeq) *
          (T(Import)
           << (T(Group)
               << (T(Var)(
                     [](auto& n) { return loc_equals(*n.first, "future"); }) *
                   T(Dot) * T(Var)([](auto& n) {
                     return loc_equals(*n.first, "keywords");
                   }) *
                   T(Dot) * T(Var)[Keyword] * End))) >>
        [](Match& _) {
          if (_(Keyword)->location().view() == "every")
          {
            return Seq << (Keyword << (Var ^ "every"))
                       << (Keyword << (Var ^ "in"));
          }
          return Keyword << _(Keyword);
        },

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

      In(ImportSeq) *
          (T(Import)
           << ((T(Group)
                << (ImportToken[Head] * ImportToken++[Tail] * T(As) *
                    T(Var)[Var] * End)) *
               End)) >>
        [](Match& _) {
          return Import << (ImportRef << (Group << _(Head) << _[Tail])) << As
                        << _(Var);
        },

      In(ImportSeq) *
          (T(Import)
           << ((T(Group) << (ImportToken[Head] * ImportToken++[Tail] * End)) *
               End)) >>
        [](Match& _) {
          return Import << (ImportRef << (Group << _(Head) << _[Tail])) << As
                        << Undefined;
        },

      // errors
      In(Import) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid import"); },

      In(With) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid with reference"); },

      In(Group) * T(As)[As] >>
        [](Match& _) { return err(_(As), "Invalid as statement"); },

      In(WithExpr) * (T(Group)[Group] << End) >>
        [](Match& _) { return err(_(Group), "Invalid with expression"); },
    };
  }
}