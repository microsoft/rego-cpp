# rego-cpp Pass Pipeline Reference

The rego-cpp compiler has two main pipelines, each composed of sequential Trieste passes.

## Pipeline 1: File-to-Rego (Parsing → Structured AST)

**Source**: `src/file_to_rego.cc`
**Input**: Raw Rego source text
**Output**: Structured Rego AST conforming to `wf` (defined in `include/rego/rego.hh`)

| # | Pass Name | Direction | Purpose |
|---|-----------|-----------|---------|
| 1 | `prep` | bottomup, once | Token preparation from parse tree: organize raw tokens into initial structure |
| 2 | `some_every` | bottomup, once | Extract `some` and `every` declarations from token groups |
| 3 | `ref_args` | bottomup, once | Process reference bracket/dot arguments into RefArgBrack/RefArgDot nodes |
| 4 | `refs` | bottomup, once | Build Ref and RefTerm expressions from tokens |
| 5 | `groups` | bottomup, once | Group tokens into Array, Object, Set collections |
| 6 | `terms` | bottomup, once | Extract Term nodes from expressions |
| 7 | `unary` | bottomup, once | Handle unary minus/negation operators |
| 8 | `arithbin_first` | bottomup, fixpoint | First-precedence operators: ×, ÷, % (multiply/divide/modulo) |
| 9 | `arithbin_second` | bottomup, fixpoint | Second-precedence operators: +, − (add/subtract) |
| 10 | `comparison` | bottomup, once | Comparison operators: ==, !=, <, >, <=, >= |
| 11 | `membership` | bottomup, once | Membership/containment: `in` operator |
| 12 | `assign` | bottomup, once | Assignment operators: `:=` and `=` unification |
| 13 | `else_not` | bottomup, once | Process `else` and `not` keywords |
| 14 | `collections` | bottomup, once | Array/object/set comprehension construction |
| 15 | `lines` | topdown, once | Statement line boundary detection |
| 16 | `rules` | bottomup, once | Rule head/body extraction and structuring |
| 17 | `literals` | bottomup, once | Literal formation from values |
| 18 | `structure` | bottomup, once | Final module structure assembly |

### Operator Precedence Passes (8–12)

Passes 8–12 implement operator precedence via the Trieste multi-pass approach:
- **Higher precedence first**: `arithbin_first` (×÷%) runs before `arithbin_second` (+−)
- This naturally produces correct binary tree nesting without explicit precedence tables
- The same pattern from the Trieste infix tutorial: separate passes per precedence level

## Pipeline 2: Rego-to-Bundle (Structured AST → Executable Bytecode)

**Source**: `src/rego_to_bundle.cc`
**Input**: Structured Rego AST conforming to `wf_bundle_input` (defined in `src/internal.hh`)
**Output**: Executable bundle conforming to `wf_bundle` (defined in `include/rego/rego.hh`)

| # | Pass Name | Direction | Purpose |
|---|-----------|-----------|---------|
| 1 | `refheads` | bottomup, once | Rule head reference processing |
| 2 | `rules` | topdown, once | Symbol table population for rules and modules |
| 3 | `locals` | bottomup, once | Local variable identification and scoping |
| 4 | `implicit_scans` | bottomup, once | Implicit iteration discovery (e.g., iterating over sets/arrays) |
| 5 | `merge` | topdown, once | Virtual document hierarchy merging |
| 6 | `unify` | bottomup, fixpoint | Unification: convert expressions to assignments/equality tests using dependency graph |
| 7 | `expr_to_opblock` | bottomup, once | Convert high-level expressions to operation blocks (bytecode-like) |
| 8 | `lift_functions` | bottomup, once | Lift rules to callable functions |
| 9 | `with_rules` | bottomup, once | Handle `with` statement rewriting and function reification |
| 10 | `add_plans` | topdown, once | Generate execution plans for entrypoints |
| 11 | `index_strings_locals` | topdown, once | Index string constants and local variables for the VM |

## Well-formedness Chain

```
wf_parser (parse.cc output)
  → wf_prep → wf_some_every → wf_ref_args → ... → wf_structure
      = wf (rego.hh, the "Rego source" grammar)

wf_bundle_input (internal.hh, starting point for pipeline 2)
  → wf_refheads → wf_rules → wf_locals → ... → wf_index_strings_locals
      = wf_bundle (rego.hh, the executable format)
```

Each pass's WF extends the previous with `|` (adding new node types) or replaces entries (changing node structure). WF definitions are defined **inline** in the same file as the pass, near the `PassDef` function.

## VM Execution

After both pipelines complete, the resulting bundle AST is executed by the virtual machine in `src/virtual_machine.cc`. The VM interprets the operation blocks produced by `expr_to_opblock` and subsequent passes.
