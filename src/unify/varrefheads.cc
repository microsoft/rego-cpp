#include "rego.hh"
#include "trieste/rewrite.h"
#include "unify.hh"

namespace
{
  using namespace rego;

  bool refarg_is_varref(const Node& refarg)
  {
    if (refarg != RefArgBrack)
    {
      return false;
    }

    const Node& expr = refarg->front();
    if (expr != Expr)
    {
      return false;
    }

    const Node& term = expr->front();
    if (term != Term)
    {
      return false;
    }

    return term->front() == Var || term->front() == Ref;
  }

  Node term_to_expr(const Node& node)
  {
    return Expr << (Term << node);
  }

  Node refarg_to_object_key(const Node& node)
  {
    if (node == RefArgDot)
    {
      std::string key_str = add_quotes(node->front()->location().view());
      return term_to_expr(Scalar << (JSONString ^ key_str));
    }
    else if (node->front() == Expr)
    {
      return node->front();
    }
    else if (node->front() == Placeholder)
    {
      return Expr << (Term << (Var ^ node->fresh(node->location())));
    }
    else
    {
      return err(node, "Cannot convert refarg to object key");
    }
  }
}

namespace rego
{
  bool refargs_contain_varref(const Node& node)
  {
    Node refargseq = node;
    if (node == Ref)
    {
      refargseq = node / RefArgSeq;
    }

    bool exists =
      std::find_if(refargseq->begin(), refargseq->end(), refarg_is_varref) !=
      refargseq->end();
    return exists;
  }

  // This pass finds rules which have refheads that contain variables.
  // The strategy is to trim the refhead to the point before the variable,
  // and then alter the query such that the object or set (as appropriate)
  // is constructed as a comprehension.
  PassDef varrefheads()
  {
    PassDef pass =
      {
        "varrefheads",
        wf_pass_varrefheads,
        dir::bottomup | dir::once,
        {
          In(Rule) *
              (T(True, False)[Default] *
               (T(RuleHead)
                << ((T(RuleRef) << T(Ref)[Ref]([](auto& n) {
                       return refargs_contain_varref(n.front());
                     })) *
                    (T(RuleHeadComp) << T(Expr)[Expr]))) *
               (T(RuleBodySeq) << ~T(Query)[Query])) >>
            [](Match& _) {
              ACTION();
              logging::Trace()
                << "-- Found rulecomp with variable in refhead --";

              Node refargseq = _(Ref) / RefArgSeq;

              auto start = std::find_if(
                refargseq->begin(), refargseq->end(), refarg_is_varref);

              Node key = refarg_to_object_key(refargseq->back());
              Node val = _(Expr);

              std::vector<Node> varrefargs(start, refargseq->end() - 1);
              while (!varrefargs.empty())
              {
                Node new_key = refarg_to_object_key(varrefargs.back());
                varrefargs.pop_back();
                val = term_to_expr(DynamicObject << (ObjectItem << key << val));
                key = new_key;
              }

              // everything before the varref is a valid rule ref
              refargseq->erase(start, refargseq->end());
              Node ref = _(Ref);
              if (refargseq->empty())
              {
                ref = (_(Ref) / RefHead)->front();
                if (ref != Var)
                {
                  ref = err(ref, "Invalid rule head");
                }
              }

              return Seq << _(Default)
                         << (RuleHead
                             << (RuleRef << ref)
                             << (RuleHeadObj << key << val << (True ^ "true")))
                         << (RuleBodySeq << _(Query));
            },

          In(Rule) *
              (T(False)[Default] *
               (T(RuleHead)
                << ((T(RuleRef) << T(Ref)[Ref]([](auto& n) {
                       return refargs_contain_varref(n.front());
                     })) *
                    (T(RuleHeadObj) << (T(Expr)[Key] * T(Expr)[Val])))) *
               (T(RuleBodySeq) << ~T(Query)[Query])) >>
            [](Match& _) {
              ACTION();
              logging::Trace()
                << "-- Found ruleobj with variable in refhead --";
              Node refargseq = _(Ref) / RefArgSeq;

              auto start = std::find_if(
                refargseq->begin(), refargseq->end(), refarg_is_varref);

              Node key = _(Key);
              Node val = _(Val);

              std::vector<Node> varrefargs(start, refargseq->end());
              while (!varrefargs.empty())
              {
                Node new_key = refarg_to_object_key(varrefargs.back());
                varrefargs.pop_back();
                val = term_to_expr(DynamicObject << (ObjectItem << key << val));
                key = new_key;
              }

              // everything before the varref is a valid rule ref
              refargseq->erase(start, refargseq->end());
              Node ref = _(Ref);
              if (refargseq->empty())
              {
                ref = (_(Ref) / RefHead)->front();
                if (ref != Var)
                {
                  ref = err(ref, "Invalid rule head");
                }
              }

              return Seq << _(Default)
                         << (RuleHead
                             << (RuleRef << ref)
                             << (RuleHeadObj << key << val << (True ^ "true")))
                         << (RuleBodySeq << _(Query));
            },

          In(Rule) *
              (T(False)[Default] *
               (T(RuleHead)
                << ((T(RuleRef) << T(Ref)[Ref]([](auto& n) {
                       return refargs_contain_varref(n.front());
                     })) *
                    (T(RuleHeadSet) << T(Expr)[Expr]))) *
               (T(RuleBodySeq) << ~T(Query)[Query])) >>
            [](Match& _) {
              ACTION();
              logging::Trace()
                << "-- Found ruleset with variable in refhead --";
              Node refargseq = _(Ref) / RefArgSeq;

              auto start = std::find_if(
                refargseq->begin(), refargseq->end(), refarg_is_varref);

              Node key = refarg_to_object_key(refargseq->back());
              Node val = term_to_expr(DynamicSet << _(Expr));

              std::vector<Node> varrefargs(start, refargseq->end() - 1);
              while (!varrefargs.empty())
              {
                Node new_key = refarg_to_object_key(varrefargs.back());
                varrefargs.pop_back();
                val = term_to_expr(DynamicObject << (ObjectItem << key << val));
                key = new_key;
              }

              // everything before the varref is a valid rule ref
              refargseq->erase(start, refargseq->end());
              Node ref = _(Ref);
              if (refargseq->empty())
              {
                ref = (_(Ref) / RefHead)->front();
                if (ref != Var)
                {
                  ref = err(ref, "Invalid rule head");
                }
              }

              return Seq << _(Default)
                         << (RuleHead
                             << (RuleRef << ref)
                             << (RuleHeadObj << key << val << (True ^ "true")))
                         << (RuleBodySeq << _(Query));
            },

          In(Rule) *
              (T(True, False)[Default] *
               (T(RuleHead)
                << (T(RuleRef)[RuleRef] *
                    (T(RuleHeadObj) << (T(Expr)[Key] * T(Expr)[Val])))) *
               T(RuleBodySeq)[RuleBodySeq]) >>
            [](Match& _) {
              return Seq << _(Default)
                         << (RuleHead << _(RuleRef)
                                      << (RuleHeadObj << _(Key) << _(Val)
                                                      << (False ^ "false")))
                         << _(RuleBodySeq);
            },
        }};

    return pass;
  }
}