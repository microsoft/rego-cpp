#include "lang.h"
#include "log.h"
#include "passes.h"

namespace rego
{
  // Processes all complex Ref objects and replaces them with SimpleRef objects,
  // which only have a single argument by the use of temporary variables.
  PassDef simple_refs()
  {
    return {
      In(Module) * T(Import) >> [](Match&) -> Node { return {}; },

      // non-var refhead
      In(Ref) * T(RefHead)[RefHead]([](auto& n) {
        Node head = *n.first;
        return is_in(head, {UnifyBody}) && head->front()->type() != Var;
      }) >>
        [](Match& _) {
          LOG("non-var refhead");
          Location refhead = _.fresh({"refhead"});
          Node head = _(RefHead)->front();
          if (head->type() != ExprCall)
          {
            head = Term << head;
          }

          return Seq
            << (Lift << UnifyBody << (Local << (Var ^ refhead) << Undefined))
            << (Lift << UnifyBody
                     << (Literal
                         << (Expr
                             << (AssignInfix
                                 << (AssignArg << (RefTerm << (Var ^ refhead)))
                                 << (AssignArg << head)))))
            << (RefHead << (Var ^ refhead));
        },

      // var
      In(RefTerm) *
          (T(Ref) << ((T(RefHead) << T(Var)[Var]) * (T(RefArgSeq) << End))) >>
        [](Match& _) {
          LOG("var");
          return _(Var);
        },

      // expr-call refhead
      In(RefHead) * T(ExprCall)[ExprCall]([](auto& n) {
        return is_in(*n.first, {UnifyBody});
      }) >>
        [](Match& _) {
          LOG("expr-call refhead");
          Location call = _.fresh({"call"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ call) << Undefined))
                     << (Lift
                         << UnifyBody
                         << (Literal
                             << (Expr
                                 << (AssignInfix
                                     << (AssignArg << (RefTerm << (Var ^ call)))
                                     << (AssignArg << _(ExprCall))))))
                     << (Var ^ call);
        },

      // ref = ref
      In(UnifyBody) *
          (T(Literal)
           << (T(Expr)
               << (T(AssignInfix)
                   << ((T(AssignArg) << (T(RefTerm)[Lhs] << T(Ref))) *
                       (T(AssignArg) << (T(RefTerm)[Rhs] << T(Ref))))))) >>
        [](Match& _) {
          LOG("ref = ref");
          Node seq = NodeDef::create(Seq);
          Location ref0 = _.fresh({"ref"});
          Location ref1 = _.fresh({"ref"});
          seq->push_back(Local << (Var ^ ref0) << Undefined);
          seq->push_back(Local << (Var ^ ref1) << Undefined);
          seq->push_back(
            Literal
            << (Expr
                << (AssignInfix << (AssignArg << (RefTerm << (Var ^ ref0)))
                                << (AssignArg << _(Lhs)))));
          seq->push_back(
            Literal
            << (Expr
                << (AssignInfix << (AssignArg << (RefTerm << (Var ^ ref1)))
                                << (AssignArg << _(Rhs)))));
          seq->push_back(
            Literal
            << (Expr
                << (AssignInfix << (AssignArg << (RefTerm << (Var ^ ref0)))
                                << (AssignArg << (RefTerm << (Var ^ ref1))))));
          return seq;
        },

      // ref.a
      T(RefTerm)
          << (T(Ref)([](auto& n) { return is_in(*n.first, {UnifyBody}); })
              << ((T(RefHead) << T(Var)[Var]) *
                  (T(RefArgSeq) << (T(RefArgDot))[Head] * RefArg++[Tail]))) >>
        [](Match& _) {
          LOG("ref.a");
          Node seq = NodeDef::create(Seq);
          Location ref = _.fresh({"ref"});
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ ref) << Undefined));
          seq->push_back(
            Lift << UnifyBody
                 << (Literal
                     << (Expr
                         << (AssignInfix
                             << (AssignArg << (RefTerm << (Var ^ ref)))
                             << (AssignArg
                                 << (RefTerm
                                     << (SimpleRef << _(Var) << _(Head))))))));
          NodeRange tail = _[Tail];
          if (tail.second > tail.first)
          {
            seq->push_back(
              RefTerm
              << (Ref << (RefHead << (Var ^ ref)) << (RefArgSeq << tail)));
          }
          else
          {
            seq->push_back(RefTerm << (Var ^ ref));
          }

          return seq;
        },

      // ref[a]
      T(RefTerm)
          << (T(Ref)([](auto& n) { return is_in(*n.first, {UnifyBody}); })
              << ((T(RefHead) << T(Var)[Var]) *
                  (T(RefArgSeq) << (T(RefArgBrack)[Head] * RefArg++[Tail])))) >>
        [](Match& _) {
          LOG("ref[a]");
          Node seq = NodeDef::create(Seq);
          Location ref = _.fresh({"ref"});
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ ref) << Undefined));

          Node head = _(Head);
          if (contains_local(head))
          {
            Location index = _.fresh({"index"});
            seq->push_back(
              Lift << UnifyBody << (Local << (Var ^ index) << Undefined));
            seq->push_back(
              Lift << UnifyBody
                   << (Literal
                       << (Expr
                           << (AssignInfix
                               << (AssignArg << (RefTerm << (Var ^ ref)))
                               << (AssignArg
                                   << (RefTerm
                                       << (SimpleRef
                                           << _(Var)
                                           << (RefArgBrack
                                               << (RefTerm
                                                   << (Var ^ index))))))))));
            Node arg = head->front();
            if (
              arg->type() == Array || arg->type() == Object ||
              arg->type() == Set || arg->type() == Scalar)
            {
              arg = Term << arg;
            }

            seq->push_back(
              Lift << UnifyBody
                   << (Literal
                       << (Expr
                           << (AssignInfix
                               << (AssignArg << arg)
                               << (AssignArg << (RefTerm << (Var ^ index)))))));
          }
          else
          {
            seq->push_back(
              Lift << UnifyBody
                   << (Literal
                       << (Expr
                           << (AssignInfix
                               << (AssignArg << (RefTerm << (Var ^ ref)))
                               << (AssignArg
                                   << (RefTerm
                                       << (SimpleRef << _(Var) << head)))))));
          }

          NodeRange tail = _[Tail];
          if (tail.second > tail.first)
          {
            seq->push_back(
              RefTerm
              << (Ref << (RefHead << (Var ^ ref)) << (RefArgSeq << tail)));
          }
          else
          {
            seq->push_back(RefTerm << (Var ^ ref));
          }

          return seq;
        },

      // expr-call
      T(ExprCall)([](auto& n) { return is_in(*n.first, {UnifyBody}); })
          << ((T(VarSeq) << (T(Var)[Head] * T(Var)++[Tail])) *
              T(ArgSeq)[ArgSeq]) >>
        [](Match& _) {
          LOG("expr-call");
          Node seq = NodeDef::create(Seq);
          Node head = _(Head);
          NodeRange tail = _[Tail];
          if (tail.second > tail.first)
          {
            for (auto it = tail.first; it != tail.second; ++it)
            {
              Node n = *it;
              Location ref = _.fresh({"ref"});
              seq->push_back(
                Lift << UnifyBody << (Local << (Var ^ ref) << Undefined));
              seq->push_back(
                Lift << UnifyBody
                     << (Literal
                         << (Expr
                             << (AssignInfix
                                 << (AssignArg << (RefTerm << (Var ^ ref)))
                                 << (AssignArg
                                     << (RefTerm
                                         << (SimpleRef
                                             << head << (RefArgDot << n))))))));
              head = Var ^ ref;
            }
          }
          seq->push_back(ExprCall << head << _(ArgSeq));
          return seq;
        },

      // errors
      T(Expr)[Expr] << (Any * Any) >>
        [](Match& _) { return err(_(Expr), "Invalid expression"); },

      In(ExprCall) * T(VarSeq)[VarSeq] >>
        [](Match& _) { return err(_(VarSeq), "Invalid function call"); },

      In(RefTerm) *
          T(Ref)[Ref]([](auto& n) { return !is_in(*n.first, {UnifyBody}); }) >>
        [](Match& _) { return err(_(Ref), "Unable to simplify reference"); },
    };
  }
}