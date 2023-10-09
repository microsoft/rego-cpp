#include "internal.hh"

namespace rego
{
  // Lifts the query as a rule in a new module.
  PassDef lift_query()
  {
    return {
      "lift_query",
      wf_pass_lift_query,
      dir::topdown,
      {
      In(Rego) *
        ((T(Query) << T(UnifyBody)[Query]) * T(Input)[Input] * T(Data)[Data] *
         T(ModuleSeq)[ModuleSeq]) >>
      [](Match& _) {
        ACTION();
        Location querymodule = _.fresh({"querymodule"});
        Location queryrule = _.fresh({"query"});
        Node module =
          Module << (Package
                     << (Ref << (RefHead << (Var ^ querymodule)) << RefArgSeq))
                 << (Policy
                     << (RuleComp << (Var ^ queryrule) << Empty << _(Query)
                                  << (Int ^ "0")));
        std::ostringstream oss;
        oss << "data." << querymodule.view() << "." << queryrule.view();
        std::string ref = oss.str();
        return Seq << (Query << (Var ^ ref)) << _(Input) << _(Data)
                   << (ModuleSeq << module << *_[ModuleSeq]);
      }}};
  }
}