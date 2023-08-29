#include "errors.h"
#include "passes.h"
#include "utils.h"

namespace rego
{
  // Separates out Input and Data nodes from the rest of the AST.
  PassDef input_data()
  {
    return {
      In(Input) * (T(File) << T(Group)[Group]) >>
        [](Match& _) { return _(Group); },

      In(Rego) * (T(Input) << T(Group)[Group]) >>
        [](Match& _) { return Input << (Key ^ "input") << _(Group); },

      In(DataSeq) * (T(File) << (T(Group) << T(Brace)[Brace])) >>
        [](Match& _) { return Data << _(Brace); },

      In(Rego) * (T(Input) << T(Undefined)) >>
        [](Match&) { return Input << (Group << JSONNull); },

      // errors
      In(Input) * T(File)[File] >>
        [](Match& _) { return err(_(File), "Invalid JSON file for input"); },

      In(DataSeq) * T(File)[File] >>
        [](Match& _) { return err(_(File), "Invalid JSON file for data"); },

      In(Rego) * (T(Input)[Input] << T(Error)) >>
        [](Match& _) { return err(_(Input), "Invalid input"); },
    };
  }

}