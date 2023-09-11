#include "internal.hh"

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

      // ref.a/ref[a]
      T(RefTerm)
          << (T(Ref)([](auto& n) { return is_in(*n.first, {UnifyBody}); })
              << ((T(RefHead) << T(Var)[Var]) *
                  (T(RefArgSeq) << (RefArg[Head] * RefArg++[Tail])))) >>
        [](Match& _) {
          LOG("ref.a/ref[a]");
          if (_(Var)->location().view() == "data")
          {
            // At this point all possible documents are fully qualified and in
            // the symbol table. As such, a reference such as this, which points
            // to a top-level module or rule, is a dead link and can be
            // replaced.
            Location dead = _.fresh({"dead"});
            return RefTerm << (Var ^ dead);
          }

          NodeRange tail = _[Tail];
          Location ref = _.fresh({"ref"});
          Node seq =
            Seq << (Lift << UnifyBody << (Local << (Var ^ ref) << Undefined))
                << (Lift << UnifyBody
                         << (Literal
                             << (Expr
                                 << (AssignInfix
                                     << (AssignArg << (RefTerm << (Var ^ ref)))
                                     << (AssignArg
                                         << (RefTerm
                                             << (SimpleRef << _(Var)
                                                           << _(Head))))))));

          if (tail.first == tail.second)
          {
            return seq << (RefTerm << (Var ^ ref));
          }

          return seq
            << (RefTerm
                << (Ref << (RefHead << (Var ^ ref)) << (RefArgSeq << tail)));
        },

      In(ExprCall) * (T(RuleRef) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

      In(ExprCall) *
          (T(RuleRef)[RuleRef](
             [](auto& n) { return is_in(*n.first, {UnifyBody}); })
           << T(Ref)) >>
        [](Match& _) {
          Location call_func = _.fresh({"call_func"});
          return Seq
            << (Lift << UnifyBody << (Local << (Var ^ call_func) << Undefined))
            << (Lift << UnifyBody
                     << (Literal
                         << (Expr
                             << (AssignInfix
                                 << (AssignArg
                                     << (RefTerm << (Var ^ call_func)))
                                 << (AssignArg << (RefTerm << *_[RuleRef]))))))
            << (Var ^ call_func);
        },

      // errors
      T(Expr)[Expr] << (Any * Any) >>
        [](Match& _) { return err(_(Expr), "Invalid expression"); },

      In(ExprCall) * T(RuleRef)[RuleRef] >>
        [](Match& _) { return err(_(RuleRef), "Invalid function call"); },

      In(RefTerm) *
          T(Ref)[Ref]([](auto& n) { return !is_in(*n.first, {UnifyBody}); }) >>
        [](Match& _) { return err(_(Ref), "Unable to simplify reference"); },

      In(RuleRef) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Invalid rule reference call"); },
    };
  }
}