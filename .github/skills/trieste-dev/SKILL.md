---
name: trieste-dev
description: 'Plan and implement Trieste-based compiler passes and AST transformations for rego-cpp. Use when: adding new compiler passes, modifying AST structure, implementing new Rego language features, debugging pass failures, working with well-formedness definitions, or performing any multi-step implementation that touches the Trieste pass pipeline. Includes the multi-planner approach for complex features.'
argument-hint: 'Describe the feature or pass work to plan or implement.'
---

# Trieste Development Workflow

Plan and implement Trieste-based compiler passes, AST transformations, and language features in rego-cpp.

## When to Use

- Adding or modifying a compiler pass in the file-to-rego or rego-to-bundle pipeline
- Implementing new Rego language syntax (new tokens, grammar rules)
- Changing well-formedness definitions
- Debugging pass failures or well-formedness violations
- Implementing complex multi-step features that touch the AST pipeline
- Any task requiring coordination across parser, passes, built-ins, and VM

## Core Concepts

Trieste is a multi-pass term-rewriting system. Understanding these concepts is mandatory before proceeding:

- **Pass**: A `PassDef` that takes an AST conforming to an input well-formedness (WF) definition and rewrites it to conform to an output WF definition. Passes run repeatedly until no more rules match (fixpoint), unless `dir::once` is specified.
- **Well-formedness (WF)**: A structural specification of valid AST shapes. Each pass declares its output WF. WF definitions are **incremental** — each extends the previous with `|` (choice).
- **Pattern → Effect rules**: Each pass contains rules of the form `Pattern >> Effect`. Patterns match AST subtrees; effects produce replacement subtrees.
- **Driver/Reader/Rewriter**: Trieste helpers that chain passes into pipelines. rego-cpp uses `Reader` for parsing and `Rewriter` for transformation.
- **Generative testing**: Trieste can generate random ASTs from WF definitions to fuzz each pass. This discovers edge cases in rewrite rules.

## Procedure

### Step 0: Understand the Current AST

Before any implementation, you must understand the AST structure at the point you're modifying.

1. **Read the well-formedness definitions** for the passes surrounding your change:
   - File-to-rego passes: defined in `src/file_to_rego.cc` (WF definitions inline with passes)
   - Rego-to-bundle passes: defined in `src/rego_to_bundle.cc`
   - Base WF: `include/rego/rego.hh` → `wf`
   - Bundle WF: `include/rego/rego.hh` → `wf_bundle`
   - Internal WF: `src/internal.hh` → `wf_bundle_input`

2. **Dump the AST** at the relevant pass to see the actual tree shape:
   ```bash
   ./build/tools/rego eval --dump_passes .copilot/pass-debug/ -p <pass_name> '<query>'
   ```
   Or write a minimal `.rego` file and use `--wf` to check well-formedness.

3. **Never assume node structure** — always verify by reading the WF definition. Nodes are typically wrapped (e.g., Array elements inside Term nodes). Use `unwrap()` helpers.

### Step 1: Multi-Planner Analysis

For any non-trivial feature, use the **multi-planner approach** — analyze the problem from multiple perspectives before writing code. This prevents costly rework.

#### Perspective 1: Reference Implementation (OPA)

How does OPA implement this feature?

1. **Check OPA's documentation** for the feature's specification
2. **Inspect OPA's IR output** to see how OPA compiles the feature:
   ```bash
   mkdir -p .copilot/opa-ir-test
   # Create minimal policy exercising the feature
   cat > .copilot/opa-ir-test/policy.rego << 'EOF'
   package test
   # ... minimal example using the feature
   EOF
   /tmp/opa build --bundle .copilot/opa-ir-test --target plan -e test/<entry> -o .copilot/opa-ir-test/bundle.tar.gz
   cd .copilot/opa-ir-test && tar xzf bundle.tar.gz && python3 -m json.tool plan.json
   ```
3. **Test both constant and variable expressions** — OPA's optimizer may fold constants, hiding the general compilation path
4. **Record**: internal built-in names, calling conventions, undefined-handling patterns

#### Perspective 2: AST Pipeline Impact

Where in the rego-cpp pipeline does this feature need to be handled?

1. **Parser changes?** — Does this require new tokens in `include/rego/rego.hh` and rules in `src/parse.cc`?
2. **Which file-to-rego passes are affected?** — Map the feature to specific passes in the 18-pass file-to-rego pipeline (see [pass-pipeline.md](./references/pass-pipeline.md))
3. **Which rego-to-bundle passes are affected?** — Map to the 11-pass rego-to-bundle pipeline
4. **VM changes?** — Does `src/virtual_machine.cc` need new opcodes or evaluation logic?
5. **Built-in additions?** — Any new built-in functions required?
6. **New Term alternative?** — If adding a new node type to `Term`, audit all type-dispatch sites:
   - `src/dependency_graph.cc` — `add_lhs_var` / `add_rhs` must handle the new type
   - `src/resolver.cc` — variable resolution may need a case
   - `src/virtual_machine.cc` — evaluation dispatch
   - `src/encoding.cc` — serialization in `to_key()`
   - `src/opblock.cc` — lowering to opcodes in `term_to_opblock()`

#### Perspective 3: Well-formedness Chain

How do WF definitions need to change?

1. Trace the WF chain from the first affected pass to the last
2. Identify which node types need to be added, modified, or removed at each stage
3. Verify that WF changes are **incremental** — each definition extends the previous
4. Check that no downstream pass is broken by the WF changes

#### Perspective 4: Test Strategy

How will you verify correctness at each stage?

1. **YAML test cases** — Write expected input/output pairs in `tests/regocpp.yaml` or `tests/bugs.yaml`
2. **OPA conformance tests** — Identify which OPA test subdirectories exercise the feature
3. **Generative testing** — Plan to run the Trieste `test` command to check WF validity
4. **Incremental verification** — After each pass modification, run targeted tests before proceeding

### Step 2: Implementation Plan

Based on the multi-planner analysis, create a sequenced implementation plan:

1. **Order changes by pipeline stage** — parser first, then file-to-rego passes in order, then rego-to-bundle passes, then VM
2. **Implement one pass at a time** — never modify multiple passes simultaneously without testing between changes
3. **Write test cases first** — add YAML test cases for the feature before implementing, so you can verify each step
4. **Use smallest possible passes** — prefer adding a new small pass over making an existing pass more complex (Trieste philosophy: "there is no downside to having many passes")

### Step 3: Incremental Implementation

For each pass change:

1. **Read the current pass code** and its surrounding WF definitions
2. **Modify the WF definition** for the pass output if needed (define new node shapes)
3. **Add rewrite rules** using the pattern → effect DSL:
   ```cpp
   // Standard pattern: match context, capture nodes, produce replacement
   In(ParentType) * T(NodeType)[Capture] >> [](Match& _) {
     return NewNode << _(Capture);
   },
   ```
4. **Add error rules** for invalid inputs the WF would allow:
   ```cpp
   // Catch-all for malformed nodes (order matters — put after positive rules)
   T(BadNode)[Node] >> [](Match& _) {
     return err(_(Node), "descriptive error message");
   },
   ```
5. **Run targeted tests** immediately:
   ```bash
   # Run specific test case
   ./build/tests/rego_test -wf tests/regocpp.yaml
   # Or specific OPA subdirectory
   ./build/tests/rego_test -wf opa/v1/test/cases/testdata/v1/<subdir>
   ```
6. **Dump the AST** to verify the transformation:
   ```bash
   ./build/tools/rego eval --dump_passes .copilot/pass-debug/ '<query>'
   ```

### Step 4: Validation

After all passes are implemented:

1. **Run the full rego-cpp test suite**:
   ```bash
   ctest --test-dir build -R "rego_test_regocpp|rego_test_bugs|rego_test_cts|rego_test_cpp_api"
   ```
2. **Run OPA conformance tests** (if applicable):
   ```bash
   ctest --test-dir build -R rego_test_opa --output-on-failure
   ```
3. **Run generative testing** to check WF validity:
   ```bash
   ./build/tools/rego test -f -c 1000
   ```
4. **Run with AddressSanitizer** for memory safety:
   ```bash
   cmake --preset asan-clang && ninja -C build-asan && ctest --test-dir build-asan
   ```

## Key Patterns Reference

### PassDef Structure

```cpp
PassDef my_pass()
{
  return {
    "my_pass",                        // Name (for debugging/logging)
    wf_my_pass,                       // Output well-formedness definition
    dir::bottomup | dir::once,        // Traversal: topdown/bottomup, once/fixpoint
    {
      // Rules (matched in order, first match wins)
      In(Parent) * T(Child)[C] >> [](Match& _) { return _(C); },
    }
  };
}
```

### Traversal Directions

| Direction | Meaning |
|-----------|---------|
| `dir::bottomup` | Process children before parents |
| `dir::topdown` | Process parents before children |
| `dir::once` | Single traversal (combine with above) |
| *(no once)* | Repeat until fixpoint (no rules match) |

### Pattern DSL Quick Reference

| Pattern | Meaning |
|---------|---------|
| `T(Foo)` | Match a node of type `Foo` |
| `T(Foo)[X]` | Match `Foo`, bind to variable `X` |
| `T(Foo) / T(Bar)` | Match `Foo` or `Bar` |
| `A * B` | Match `A` followed by `B` (siblings) |
| `P << C` | Match children `C` inside parent `P` |
| `In(P)` | Parent context is `P` (not part of match) |
| `Any` | Match any single node |
| `Any++[X]` | Match one or more remaining nodes, bind to `X` |
| `End` | Assert no more siblings |
| `_(X)` | In effect: get single node bound to `X` |
| `_[X]` | In effect: get all nodes bound to `X` (NodeRange) |
| `*_[X]` | In effect: get children of nodes bound to `X` |

### Well-formedness DSL

```cpp
inline const auto wf_my_pass =
    wf_previous_pass                          // Inherit from previous pass
  | (NewNode <<= ChildA * ChildB)            // NewNode has exactly ChildA then ChildB
  | (Container <<= Element++)                // Container has 0+ Elements
  | (Container <<= Element++[1])             // Container has 1+ Elements
  | (Wrapper <<= (ChoiceA | ChoiceB))        // Wrapper has one of ChoiceA or ChoiceB
  | (Parent <<= Name * Body)[Name]           // [Name] = Name is stored in symbol table
  ;
```

### Creating AST Nodes

```cpp
// Node with children
NewNode << child1 << child2

// Node with string content (location)
TokenType ^ "string content"

// Splice children from a matched range
Container << *_[MatchVar]    // all children of matched nodes
Container << _[MatchVar]     // all matched nodes themselves

// Empty node (remove from tree)
return {};
```

## Common Mistakes

1. **Not reading the WF definition first** — The #1 source of bugs. Nodes are wrapped in unexpected ways.
2. **Modifying multiple passes without testing between** — Errors compound and become impossible to diagnose.
3. **Comparing `child->type()` directly** — Use `unwrap()` helpers; nodes are wrapped in Term/Scalar layers.
4. **Forgetting error rules** — Generative testing will generate inputs that your positive rules don't handle. You must add error rules for these cases.
5. **Wrong traversal direction** — `bottomup` processes children first (useful when collapsing); `topdown` processes parents first (useful when pushing structure down).
6. **Rule ordering** — Rules are matched in order. If a general rule comes before a specific one, the specific rule will never fire.
7. **Missing `dir::once`** — Without it, the pass runs to fixpoint. This is correct for most passes but causes infinite loops if rules don't converge.
8. **Creating parallel paths instead of reusing the standard pipeline** — When adding a new compound node type (e.g., `TemplateString`), prefer routing its sub-expressions through the existing `Group → Literal → Expr` pipeline rather than creating a custom parallel path (e.g., `TemplateString <<= (TemplateLiteral | Expr)++`). The standard pipeline already handles `with`/`as`, `some`, comprehensions, and other features. Creating a parallel path means manually replicating all of that machinery. In the parser, use `m.term()` to separate groups naturally and `m.in(NodeType)` to detect context on closing delimiters, rather than `m.push(Brace)` which creates a separate nesting scope. Convert specialized tokens (e.g., `TemplateLiteral`) to standard types (e.g., `Scalar << String << JSONString`) as early as possible (in the `prep` pass) to minimize WF cascading.
9. **Not auditing `dependency_graph.cc` when adding new Term alternatives** — The dependency graph in `src/dependency_graph.cc` has explicit `if (lhs == Type)` cases for every node type that can appear as a Term child. When adding a new Term alternative, you must add a corresponding case there. Missing cases cause "Unable to unify due to cycle" errors. Also audit `resolver.cc` and `virtual_machine.cc` for similar type-dispatch patterns.
