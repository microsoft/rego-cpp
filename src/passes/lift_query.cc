#include "passes.h"
#include "errors.h"
#include "utils.h"

namespace rego
{
  // Lifts the query as a rule in a new module.
  PassDef lift_query()
  {
    return {
      In(Rego) *
        ((T(Query) << T(UnifyBody)[Query]) * T(Input)[Input] * T(Data)[Data] *
         T(ModuleSeq)[ModuleSeq]) >>
      [](Match& _) {
        Location querymodule = _.fresh({"querymodule"});
        Location queryrule = _.fresh({"query"});
        Node module =
          Module << (Package
                     << (Ref << (RefHead << (Var ^ querymodule)) << RefArgSeq))
                 << (Policy
                     << (RuleComp << (Var ^ queryrule) << Empty << _(Query)
                                  << (JSONInt ^ "0")));
        std::ostringstream oss;
        oss << "data." << querymodule.view() << "." << queryrule.view();
        std::string ref = oss.str();
        return Seq << (Query << (Var ^ ref)) << _(Input) << _(Data)
                   << (ModuleSeq << module << *_[ModuleSeq]);
      }};
  }
}