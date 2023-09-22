#include "internal.hh"

namespace rego
{
  // Separates out Input and Data nodes from the rest of the AST.
  PassDef input_data()
  {
    return {
      In(Input) * (T(File) << T(Group)[Group]) >>
        [](Match& _) {
          ACTION();
          return _(Group);
        },

      In(Rego) * (T(Input) << T(Group)[Group]) >>
        [](Match& _) {
          ACTION();
          return Input << (Key ^ "input") << _(Group);
        },

      In(DataSeq) * (T(File) << (T(Group) << T(Brace)[Brace])) >>
        [](Match& _) {
          ACTION();
          return Data << _(Brace);
        },

      In(Rego) * (T(Input) << T(Undefined)) >>
        [](Match&) {
          ACTION();
          return Input << (Key ^ "input") << Undefined;
        },

      // errors
      In(Input) * T(File)[File] >>
        [](Match& _) {
          ACTION();
          return err(_(File), "Invalid JSON file for input");
        },

      In(DataSeq) * T(File)[File] >>
        [](Match& _) {
          ACTION();
          return err(_(File), "Invalid JSON file for data");
        },

      In(Rego) * (T(Input)[Input] << T(Error)) >>
        [](Match& _) {
          ACTION();
          return err(_(Input), "Invalid input");
        },
    };
  }

}