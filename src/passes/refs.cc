#include "lang.h"
#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  // clang-format off
  inline const auto wfi =
      (Top <<= Rego)
    | (RefArgBrack <<= Scalar | Var | Object | Array | Set)
    ;
  // clang-format on
}

namespace rego
{
  PassDef refs()
  {
    return {
      In(RefTerm) * (T(Ref) << (T(Var)[Var] * (T(RefArgSeq) << End))) >>
        [](Match& _) { return _(Var); },

      // ref = ref
      In(UnifyBody) *
          (T(Literal)
           << (T(Expr)
               << (T(AssignInfix)
                   << ((T(AssignArg) << (T(RefTerm)[Lhs] << T(Ref))) *
                       (T(AssignArg) << (T(RefTerm)[Rhs] << T(Ref))))))) >>
        [](Match& _) {
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

      // ref
      T(RefTerm)([](auto& n) { return is_in(*n.first, UnifyBody); })
          << (T(Ref)
              << (T(Var)[Var] *
                  (T(RefArgSeq)
                   << ((T(RefArgDot) / T(RefArgCall))[Head] *
                       RefArg++[Tail])))) >>
        [](Match& _) {
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
              RefTerm << (Ref << (Var ^ ref) << (RefArgSeq << tail)));
          }
          else
          {
            seq->push_back(RefTerm << (Var ^ ref));
          }

          return seq;
        },

      T(RefTerm)([](auto& n) { return is_in(*n.first, UnifyBody); })
          << (T(Ref)
              << (T(Var)[Var] *
                  (T(RefArgSeq) << (T(RefArgBrack)[Head] * RefArg++[Tail])))) >>
        [](Match& _) {
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
            Node arg = head->at(wfi / RefArgBrack / RefArgBrack);
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
              RefTerm << (Ref << (Var ^ ref) << (RefArgSeq << tail)));
          }
          else
          {
            seq->push_back(RefTerm << (Var ^ ref));
          }

          return seq;
        },

      // errors
      T(Expr)[Expr] << (Any * Any) >>
        [](Match& _) { return err(_(Expr), "Invalid expression"); },
    };
  }
}