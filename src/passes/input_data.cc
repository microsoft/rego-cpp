#include "passes.h"
#include "errors.h"
#include "utils.h"

namespace rego
{
  // Separates out Input and Data nodes from the rest of the AST.
  PassDef input_data()
  {
    return {
      In(Input) * (T(File) << (T(Group) << (T(Brace)/T(Square))[Val])) >>
        [](Match& _) { return _(Val); },

      In(Rego) * (T(Input) << (T(Brace)/T(Square))[Val]) >>
        [](Match& _) { return Input << (Var ^ "input") << _(Val); },

      In(DataSeq) * (T(File) << (T(Group) << T(Brace)[Brace])) >>
        [](Match& _) { return Data << _(Brace); },

      In(Rego) * (T(Input) << T(Undefined)) >>
        [](Match&) { return Input << Brace; },

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