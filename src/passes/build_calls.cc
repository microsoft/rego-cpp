#include "internal.hh"

namespace rego
{
  // Builds function calls into ExprCall nodes.
  PassDef build_calls()
  {
    return {
      In(Group) * (T(Contains)[Contains] * T(Paren)[Paren]) >>
        [](Match& _) { return Seq << (Var ^ _(Contains)) << _(Paren); },

      In(Group) *
          (RuleRefToken[Head] * RuleRefToken++[Tail] * T(Paren)[Paren]) >>
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
              if (group->size() > 0)
              {
                argseq << group;
              }
            }
          }

          return ExprCall << (RuleRef << _(Head) << _[Tail]) << argseq;
        },

      // errors

      T(Group)[Group] << End >>
        [](Match& _) { return err(_(Group), "Syntax error: empty group"); },
    };
  }
}