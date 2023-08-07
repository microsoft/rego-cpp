#include "passes.h"

namespace rego
{
  PassDef build_calls()
  {
    return {
      In(Group) * (T(Var)[Var] * T(Paren)[Paren]) >>
        [](Match& _) {
          Node argseq = NodeDef::create(ArgSeq);
          Node paren = _(Paren);
          if (paren->front()->type() == List)
          {
            for (const auto& arg : *paren->front())
            {
              argseq->push_back(arg);
            }
          }
          else
          {
            argseq->push_back(paren->front());
          }

          return ExprCall << (VarSeq << (Group << _(Var))) << argseq;
        },

      In(Group) *
          (T(Var)[Var] * T(Dot) *
           (T(ExprCall) << (T(VarSeq)[VarSeq] * T(ArgSeq)[ArgSeq]))) >>
        [](Match& _) {
          return ExprCall << (VarSeq << (Group << _(Var)) << *_[VarSeq])
                          << _(ArgSeq);
        },

      // errors

      In(ExprCall) * (T(ArgSeq)[ArgSeq] << End) >>
        [](Match& _) { return err(_(ArgSeq), "Missing arguments"); },
    };
  }
}