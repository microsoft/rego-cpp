#include "passes.h"
#include "errors.h"

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
            // check if this was marked as a potential membership statement
            // As calls have precedence, we may have to split up the group.
            Node group = paren->front();
            auto comma = group->begin();
            for (; comma != group->end(); ++comma)
            {
              Node node = *comma;
              if (node->type() == Comma)
              {
                break;
              }
            }

            if (comma != group->end())
            {
              // split the group into two groups
              Node head = NodeDef::create(Group);
              Node tail = NodeDef::create(Group);
              head->insert(head->end(), group->begin(), comma);
              tail->insert(tail->end(), comma + 1, group->end());
              argseq << head << tail;
            }
            else
            {
              argseq << group;
            }
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