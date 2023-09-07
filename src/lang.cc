#include "log.h"
#include "passes.h"
#include "tokens.h"
#include "wf.h"

namespace rego
{
  bool Logger::enabled = false;
  std::string Logger::indent = "";

  std::vector<PassCheck> passes(const BuiltIns& builtins)
  {
    return {
      {"input_data", input_data(), &wf_pass_input_data},
      {"modules", modules(), &wf_pass_modules},
      {"imports", imports(), &wf_pass_imports},
      {"keywords", keywords(), &wf_pass_keywords},
      {"lists", lists(), &wf_pass_lists},
      {"ifs", ifs(), &wf_pass_ifs},
      {"elses", elses(), &wf_pass_elses},
      {"rules", rules(), &wf_pass_rules},
      {"build_calls", build_calls(), &wf_pass_build_calls},
      {"membership", membership(), &wf_pass_membership},
      {"build_refs", build_refs(), &wf_pass_build_refs},
      {"structure", structure(), &wf_pass_structure},
      {"strings", strings(), &wf_pass_strings},
      {"merge_data", merge_data(), &wf_pass_merge_data},
      {"lift_refheads", lift_refheads(), &wf_pass_lift_refheads},
      {"symbols", symbols(), &wf_pass_symbols},
      {"replace_argvals", replace_argvals(), &wf_pass_replace_argvals},
      {"lift_query", lift_query(), &wf_pass_lift_query},
      {"constants", constants(), &wf_pass_constants},
      {"explicit_enums", explicit_enums(), &wf_pass_explicit_enums},
      {"body_locals", body_locals(builtins), &wf_pass_locals},
      {"value_locals", value_locals(builtins), &wf_pass_locals},
      {"compr", compr(), &wf_pass_compr},
      {"absolute_refs", absolute_refs(), &wf_pass_absolute_refs},
      {"merge_modules", merge_modules(), &wf_pass_merge_modules},
      {"skips", skips(), &wf_pass_skips},
      {"unary", unary(), &wf_pass_unary},
      {"multiply_divide", multiply_divide(), &wf_pass_multiply_divide},
      {"add_subtract", add_subtract(), &wf_pass_add_subtract},
      {"comparison", comparison(), &wf_pass_comparison},
      {"assign", assign(builtins), &wf_pass_assign},
      {"skip_refs", skip_refs(builtins), &wf_pass_skip_refs},
      {"simple_refs", simple_refs(), &wf_pass_simple_refs},
      {"init", init(), &wf_pass_init},
      {"implicit_enums", implicit_enums(), &wf_pass_implicit_enums},
      {"rulebody", rulebody(), &wf_pass_rulebody},
      {"lift_to_rule", lift_to_rule(), &wf_pass_lift_to_rule},
      {"functions", functions(), &wf_pass_functions},
      {"unify", unify(builtins), &wf_pass_unify},
      {"query", query(), &wf_pass_query},
    };
  }

  Driver& driver(const BuiltIns& builtins)
  {
    static Driver d(
      "rego",
      nullptr,
      parser(),
      wf_parser,
      {
        {"input_data", input_data(), wf_pass_input_data},
        {"modules", modules(), wf_pass_modules},
        {"imports", imports(), wf_pass_imports},
        {"keywords", keywords(), wf_pass_keywords},
        {"lists", lists(), wf_pass_lists},
        {"ifs", ifs(), wf_pass_ifs},
        {"elses", elses(), wf_pass_elses},
        {"rules", rules(), wf_pass_rules},
        {"build_calls", build_calls(), wf_pass_build_calls},
        {"membership", membership(), wf_pass_membership},
        {"build_refs", build_refs(), wf_pass_build_refs},
        {"structure", structure(), wf_pass_structure},
        {"strings", strings(), wf_pass_strings},
        {"merge_data", merge_data(), wf_pass_merge_data},
        {"lift_refheads", lift_refheads(), wf_pass_lift_refheads},
        {"symbols", symbols(), wf_pass_symbols},
        {"replace_argvals", replace_argvals(), wf_pass_replace_argvals},
        {"lift_query", lift_query(), wf_pass_lift_query},
        {"constants", constants(), wf_pass_constants},
        {"explicit_enums", explicit_enums(), wf_pass_explicit_enums},
        {"body_locals", body_locals(builtins), wf_pass_locals},
        {"value_locals", value_locals(builtins), wf_pass_locals},
        {"compr", compr(), wf_pass_compr},
        {"absolute_refs", absolute_refs(), wf_pass_absolute_refs},
        {"merge_modules", merge_modules(), wf_pass_merge_modules},
        {"skips", skips(), wf_pass_skips},
        {"unary", unary(), wf_pass_unary},
        {"multiply_divide", multiply_divide(), wf_pass_multiply_divide},
        {"add_subtract", add_subtract(), wf_pass_add_subtract},
        {"comparison", comparison(), wf_pass_comparison},
        {"assign", assign(builtins), wf_pass_assign},
        {"skip_refs", skip_refs(builtins), wf_pass_skip_refs},
        {"simple_refs", simple_refs(), wf_pass_simple_refs},
        {"init", init(), wf_pass_init},
        {"implicit_enums", implicit_enums(), wf_pass_implicit_enums},
        {"rulebody", rulebody(), wf_pass_rulebody},
        {"lift_to_rule", lift_to_rule(), wf_pass_lift_to_rule},
        {"functions", functions(), wf_pass_functions},
        {"unify", unify(builtins), wf_pass_unify},
        {"query", query(), wf_pass_query},
      });
    return d;
  }
}