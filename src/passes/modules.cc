#include "lang.h"
#include "passes.h"

namespace rego
{
  const inline auto KeyToken =
    T(Var) / T(Square) / T(Dot) / ScalarToken / T(RawString) / T(JSONString);

  PassDef modules()
  {
    return {
      In(ModuleSeq) *
          (T(File)
           << ((T(Group) << (T(Package) * T(Var)[Id])) * T(Group)++[Tail])) >>
        [](Match& _) {
          return Module << (Package << _(Id)) << ImportSeq
                        << (Policy << _[Tail]);
        },

      In(List) * (T(Group) << (KeyToken++[Key] * T(Colon) * Any++[Val])) >>
        [](Match& _) {
          return ObjectItem << (Group << _[Key]) << (Group << _[Val]);
        },

      In(Brace) * (T(Group) << (KeyToken++[Key] * T(Colon) * Any++[Val])) >>
        [](Match& _) {
          return List << (ObjectItem << (Group << _[Key]) << (Group << _[Val]));
        },

      In(Policy) *
          (T(Group) << (T(Import) * (T(Var) / T(Dot))++[Import] * End)) >>
        [](Match& _) { return Import << (Group << _[Import]); },

      In(ModuleSeq) *
          (T(Module)
           << (T(Package)[Package] * T(ImportSeq)[ImportSeq] *
               (T(Policy)
                << (T(Import)[Head] * T(Import)++[Tail] *
                    (T(Group) / T(Import))++[Policy])))) >>
        [](Match& _) {
          auto import_seq = _(ImportSeq);
          import_seq->push_back(_(Head));
          auto tail = _[Tail];
          for (auto& node = tail.first; node != tail.second; ++node)
          {
            import_seq->push_back(*node);
          }

          return Module << _(Package) << _(ImportSeq) << (Policy << _[Policy]);
        },

      // errors
      In(ModuleSeq) * T(File)[File] >>
        [](Match& _) { return err(_(File), "Invalid module file"); },

      In(Group) * T(Package)[Package] >>
        [](Match& _) {
          return err(_(Package), "Invalid package declaration.");
        },

      In(Group) * T(Colon)[Colon] >>
        [](Match& _) { return err(_(Colon), "Invalid character"); },

      In(ObjectItem) * (T(Group)[Group] << End) >>
        [](Match& _) { return err(_(Group), "Invalid key/value pair"); },

      In(Group) * (T(Import)[Import] << End) >>
        [](Match& _) { return err(_(Import), "Invalid import"); },
    };
  }
}