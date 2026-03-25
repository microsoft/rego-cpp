# rego-cpp Copilot Instructions

## Project Overview

`rego-cpp` is a C++ interpreter for [Rego](https://www.openpolicyagent.org/docs/latest/policy-language/), the policy language of [Open Policy Agent (OPA)](https://www.openpolicyagent.org/). It targets Rego v1.8.0 and is designed for embedding policy evaluation directly into C++ applications, as well as for use from C, Rust, Python, and .NET via language wrappers.

The interpreter is built on top of [Trieste](https://github.com/microsoft/trieste), a term-rewriting framework from Microsoft Research. Evaluation proceeds by parsing Rego source into an AST and then iteratively rewriting it through a series of compiler passes until a final result is produced.

## Repository Structure

```
rego-cpp/
├── include/rego/         # Public headers
│   ├── rego.hh           # C++ API — AST token types, interpreter, built-in system
│   └── rego_c.h          # C API — flat C interface for use by other languages
├── src/                  # Core library implementation
│   ├── interpreter.cc    # Top-level interpreter: manages passes and compilation
│   ├── virtual_machine.cc# Bytecode-like execution engine (op-block evaluation)
│   ├── parse.cc          # Rego lexer and parser (Trieste-based)
│   ├── resolver.cc       # Variable resolution and unification
│   ├── rego.cc           # Main Interpreter/Rewriter entry point
│   ├── rego_c.cc         # C API wrapper over the C++ API
│   ├── bundle.cc         # OPA bundle loading
│   ├── bundle_binary.cc  # Binary bundle format support
│   ├── bundle_json.cc    # JSON bundle format support
│   ├── bigint.cc         # Arbitrary-precision integer arithmetic
│   ├── encoding.cc       # Base64/hex encoding helpers
│   ├── json.cc           # JSON parsing and serialization
│   ├── yaml.cc           # YAML parsing
│   ├── opblock.cc        # Op-block data structures
│   ├── dependency_graph.cc # Rule dependency analysis
│   ├── output.cc         # Query result formatting
│   ├── file_to_rego.cc   # File-based module loading
│   ├── rego_to_bundle.cc # Compilation to bundle format
│   ├── internal.cc / internal.hh # Shared internal utilities
│   └── builtins/         # Standard Rego built-in functions
│       ├── array.cc      # array.*
│       ├── bits.cc       # bits.*
│       ├── core.cc       # print, type_name, etc.
│       ├── crypto.cc     # crypto.*
│       ├── encoding.cc   # base64/hex builtins
│       ├── glob.cc       # glob.*
│       ├── graph.cc      # graph.*
│       ├── graphql.cc    # graphql.*
│       ├── http.cc       # http.send (stub)
│       ├── json.cc       # json.*
│       ├── jwt.cc        # io.jwt.*
│       ├── net.cc        # net.*
│       ├── numbers.cc    # numbers.*
│       ├── objects.cc    # object.*
│       ├── opa.cc        # opa.*
│       ├── regex.cc      # regex.*
│       ├── rego.cc       # rego.*
│       ├── semver.cc     # semver.*
│       ├── time.cc       # time.*
│       ├── units.cc      # units.*
│       └── uuid.cc       # uuid.*
├── tests/                # Test suite
│   ├── main.cc           # Test runner entry point
│   ├── cpp_api.cc        # Unit tests for the C++ API
│   ├── c_api.cc          # Unit tests for the C API
│   ├── builtins.cc       # Built-in function tests
│   ├── test_case.cc/h    # YAML test case infrastructure
│   ├── regocpp.yaml      # rego-cpp–specific YAML test cases
│   ├── bugs.yaml         # Regression tests for bugs
│   ├── bigint.yaml       # Big integer tests
│   ├── cts/              # Conformance test suite cases
│   ├── opa/              # OPA-compatible tests (cloned from OPA repo)
│   ├── aci/              # Azure Container Instances policy tests
│   └── cheriot/          # CHERIoT policy tests
├── tools/                # Command-line tools
│   ├── main.cc           # `rego` CLI: eval, test, inspect subcommands
│   └── fuzzer.cc         # Trieste generative fuzzer (rego_fuzzer)
├── examples/             # Usage examples by language
│   ├── cpp/              # C++ examples (example.cc, custom_builtin.cc)
│   ├── c/                # C examples (example.c, command-line tool)
│   ├── rust/             # Rust examples
│   ├── python/           # Python examples
│   └── dotnet/           # .NET examples
├── wrappers/             # Language binding source
│   ├── rust/             # Rust crate wrapping the C API
│   ├── python/           # Python ctypes/cffi wrapper
│   └── dotnet/           # .NET P/Invoke wrapper
├── cmake/                # CMake package config templates
├── doc/                  # Doxygen documentation sources
├── CMakeLists.txt        # Root build definition
├── CMakePresets.json     # Named build presets
└── VERSION               # Semantic version file (MAJOR.MINOR.PATCH)
```

## Build System

- **Language**: C++20
- **Build tool**: CMake ≥ 3.15 with Ninja
- **Key presets** (defined in `CMakePresets.json`):
  - `debug-clang` / `debug` — Debug build with tests and tools
  - `release-clang` / `release` — Release build
  - `debug-clang-opa` / `release-clang-opa` — Build including OPA compatibility tests
  - `asan-clang` — AddressSanitizer build
- **CMake options** (all default `OFF` unless noted):
  - `REGOCPP_BUILD_TOOLS` — build the `rego` CLI
  - `REGOCPP_BUILD_TESTS` — build the test binaries
  - `REGOCPP_BUILD_DOCS` — build Doxygen documentation
  - `REGOCPP_BUILD_SHARED` — build `rego_shared` as a shared library
  - `REGOCPP_OPA_TESTS` — include OPA conformance tests
  - `REGOCPP_USE_SNMALLOC` — use snmalloc allocator (default `ON`)
  - `REGOCPP_SANITIZE` — sanitizer flags (e.g., `address`)

Typical workflow:

```bash
mkdir build && cd build
cmake .. --preset release-clang
ninja install
ctest
```

After editing C++ source files, **always** run the formatting target before committing:

```bash
ninja regocpp_format
```

CI checks formatting with clang-format 18 and will reject unformatted code.

## Key Dependencies

- **[Trieste](https://github.com/microsoft/trieste)** — term rewriting framework; provides the AST node types (`TokenDef`), well-formedness definitions, logging, JSON/YAML parsers, and the rewriting pass infrastructure. Fetched via `FetchContent`.
- **snmalloc** — high-performance memory allocator (optional, fetched via `FetchContent`).

## API Design

The library exposes two public interfaces:

1. **C++ API** (`include/rego/rego.hh`): The primary interface. Provides access to the `Interpreter` class (via `rego::Interpreter`), AST token type constants (`rego::Module`, `rego::Rule`, etc.), the `BuiltIn` registration system for custom built-ins, `BigInt`, and all core types. Uses the `rego` namespace and inherits Trieste types via `using namespace trieste`.

2. **C API** (`include/rego/rego_c.h`): A flat C interface for interoperability with other languages. Wraps the C++ API using opaque handle types (`regoInterpreter*`, `regoNode*`, etc.). Implemented in `src/rego_c.cc`.

## Coding Conventions

- All source files begin with the Microsoft copyright header.
- The `rego` namespace is used throughout; internal helpers live in anonymous namespaces.
- AST token types are declared as `inline const auto` globals in `rego.hh` using Trieste's `TokenDef`.
- Well-formedness rules for each compiler pass are defined inline alongside the pass logic. **Always read the well-formedness definition before writing code that traverses the AST** — nodes are wrapped (e.g., Array elements live inside Term nodes). Use `unwrap()` helpers, not direct `type()` comparisons.
- Built-in functions are registered via the `BuiltIn` class and grouped by OPA namespace in `src/builtins/`.
- Test cases are expressed as YAML files whenever possible, using the OPA test case format.
- Error messages in built-in functions must match OPA's reference implementation exactly — conformance tests compare error strings literally.
- **Fix root causes, not symptoms.** When a test fails, investigate *why* the incorrect behavior occurs — trace the logic, inspect intermediate state, and identify the underlying defect. Do not apply surface-level patches (e.g., special-casing an output, suppressing an error, or working around stale state) just to make a test pass. A correct fix eliminates the class of bug, not just the one observable instance.
- **Move slow to go fast.** Make small, testable changes one step at a time instead of large simultaneous edits. After each change, compile and run the relevant tests before moving on. Small increments are easier to verify, easier to debug when something breaks, and produce cleaner diffs. Resist the urge to batch multiple logical changes into a single edit.

## Trieste Development Workflow

rego-cpp is built on [Trieste](https://github.com/microsoft/trieste), a multi-pass term-rewriting framework. Understanding the Trieste workflow is essential for any pass or AST work.

### Analysis Perspectives

For non-trivial features (new syntax, new passes, AST restructuring), analyze the problem from **four perspectives** before writing code:

1. **Reference Implementation (OPA)**: Inspect OPA's IR plan output to understand how OPA compiles the feature. Use `opa build --bundle <dir> --target plan` and examine `plan.json` for internal built-in names, calling conventions, and undefined-handling patterns.

2. **AST Pipeline Impact**: Map the feature to specific passes in the two pipelines:
   - **File-to-rego** (18 passes in `src/file_to_rego.cc`): parsing through structured AST
   - **Rego-to-bundle** (11 passes in `src/rego_to_bundle.cc`): structured AST through executable bytecode
   - Identify which passes need modification, new token types, WF changes, and VM changes.

3. **Well-formedness Chain**: Trace the WF definitions from the first affected pass to the last. WF definitions are incremental — each extends the previous with `|`. Verify no downstream pass breaks.

4. **Test Strategy**: Plan verification at each stage — YAML test cases, OPA conformance tests, generative fuzzing (`./build/tools/rego_fuzzer <transform>`), and ASan builds.

### Multi-perspective Planning Process

When planning a non-trivial code change, use four sub-planners running in parallel to generate competing plans, then synthesise the best elements into a final plan.

#### Step 1 — Gather sub-plans

Spawn **four fresh subagents**, each prompted to use one of the following skills. Each subagent receives the same task description and context but plans through a different lens:

| Subagent | Skill | Focus |
|----------|-------|-------|
| Speed Planner | `/plan-speed` | Runtime performance, low allocations, minimal passes, cache efficiency |
| Security Planner | `/plan-security` | Defence in depth, safe error handling, bounded resources, fuzz coverage |
| Usability Planner | `/plan-usability` | Clarity, readability, correctness, consistent naming, one-concept-per-pass |
| Conservative Planner | `/plan-conservative` | Smallest diff, maximum reuse, no speculative generality, backwards compat |

Prompt each subagent with:
> You are planning a change to the rego-cpp project. Use the `/[skill-name]` skill to guide your planning. Here is the task: [task description and relevant context]. Produce a numbered plan following the output format defined in the skill.

#### Step 2 — Evaluate the four plans

Review the four plans yourself and produce a short evaluation covering:

- **Convergence**: where two or more plans agree on the same approach. High convergence suggests a clearly correct design.
- **Unique insights**: ideas that appear in only one plan and are worth incorporating.
- **Conflicts**: where plans disagree. For each conflict, state which perspective you favour and why.
- **Gaps**: anything none of the four plans addressed.

#### Step 3 — Synthesise the final plan

Spawn a **fifth subagent** (the synthesiser). Provide it with:
- The original task description.
- All four sub-plans (labelled by perspective).
- Your evaluation from Step 2.

Prompt the synthesiser with:
> You are producing the final plan for a change to rego-cpp. You have received four sub-plans from different perspectives (Speed, Security, Usability, Conservative) and an evaluation of those plans. Synthesise them into a single coherent, numbered plan that balances all four concerns. Where the evaluation favours one perspective, follow it. Where the evaluation is neutral, prefer the Conservative approach. Output the final plan in the standard format: Goal, Steps (with file paths and descriptions balancing all four perspectives), Rationale (explaining the synthesis), and Trade-offs (any conflicts between perspectives and how they were resolved).

#### Step 4 — Review the synthesised plan

Before presenting the plan, run an iterative review loop:

1. Spawn a subagent to review the synthesised plan. Provide it with the original task description, the four sub-plans, your evaluation, and the synthesised plan. Ask it to check for: logical errors in the step ordering, steps that contradict each other, missing error handling or edge cases, violations of rego-cpp conventions, and anything the synthesis dropped that should have been kept.
2. If the review finds issues, revise the plan yourself and spawn a **different** subagent to review the revised version.
3. Repeat until a review comes back clean (no issues found).

#### Step 5 — Present for approval

Present the reviewed plan to the user along with a brief summary of:
- Key points of agreement across the four sub-planners.
- Notable trade-offs made during synthesis.
- Any minority opinions from individual sub-planners that were overruled.
- Issues caught and resolved during the review loop (if any).

#### When to use multi-perspective planning

Use the full five-step process for design decisions where the shape of the solution is uncertain: new language features, new passes, API changes, AST restructuring, or cross-cutting concerns that touch multiple pipeline stages.

For tasks that are primarily implementation of a well-understood algorithm (e.g. a new built-in function with a clear OPA specification), a single conservative plan with emphasis on incremental testing is sufficient. Use the full process when the design is uncertain, not when the algorithm is known.

### Pass Implementation Pattern

Each pass is a `PassDef` with pattern → effect rewrite rules:

```cpp
PassDef my_pass()
{
  return {
    "my_pass",                        // Name
    wf_my_pass,                       // Output well-formedness
    dir::bottomup | dir::once,        // Traversal direction
    {
      In(Parent) * T(Child)[C] >> [](Match& _) { return NewNode << _(C); },
    }
  };
}
```

Key principles from Trieste:
- **Prefer many small passes over few complex ones** — "there is no downside to having many passes"
- **Implement one pass at a time** and test between changes
- **Add error rules** for invalid inputs that WF allows — generative testing will find them
- **Rule order matters** — first match wins; put specific rules before general ones
- **Operator precedence via separate passes** — higher precedence operators in earlier passes (e.g., `arithbin_first` for ×÷% before `arithbin_second` for +−)

### Incremental Implementation

1. Write test cases first (YAML in `tests/regocpp.yaml` or `tests/bugs.yaml`)
2. Modify the WF definition for the pass output
3. Add rewrite rules (positive rules first, then error rules)
4. Run targeted tests: `./build/tests/rego_test -wf tests/regocpp.yaml`
5. Dump the AST to verify: `./build/tools/rego eval --dump_passes .copilot/pass-debug/ '<query>'`
6. Move to the next pass and repeat

## Running Tests

### Test Driver (`rego_test`)

The test binary `./build/tests/rego_test` accepts YAML test case files or directories as arguments. When given a directory, it recursively discovers all YAML test files within it.

```bash
# Run a specific YAML test file
./build/tests/rego_test -wf tests/regocpp.yaml

# Run all tests in a directory
./build/tests/rego_test -wf tests/cts/

# Run with well-formedness checking disabled (faster, no WF validation)
./build/tests/rego_test tests/bugs.yaml
```

The `-wf` flag enables well-formedness checking at each pass boundary (recommended during development).

### OPA Conformance Tests

OPA test cases are **not** checked into the repo — they live in the build directory, fetched by CMake from the OPA repository. The test root is:

```
build/opa/v1/test/cases/testdata/v1/
```

Each subdirectory under v1 is a separate test suite (e.g., `stringinterpolation`, `aggregates`, `with`). To run individual OPA test suites without running the full (slow) OPA test:

```bash
# Run from the build directory — paths are relative to the working directory
cd build && ./tests/rego_test -wf opa/v1/test/cases/testdata/v1/stringinterpolation

# Run a different OPA suite
cd build && ./tests/rego_test -wf opa/v1/test/cases/testdata/v1/with

# List available OPA test suites
ls build/opa/v1/test/cases/testdata/v1/
```

### CTest

Use `ctest` to run predefined test targets:

```bash
cd build && ctest --output-on-failure        # all tests
ctest -R rego_test_regocpp                   # just rego-cpp tests
ctest -R rego_test_opa                       # full OPA conformance suite
ctest -R "rego_test_regocpp|rego_test_bugs"  # multiple targets
```

When iterating on a specific feature, **prefer running individual OPA subdirectory tests** over the full `rego_test_opa` target — it runs much faster.

### Debugging with lldb

Debug builds (e.g., `build-mbedtls` with `CMAKE_BUILD_TYPE=Debug`) include full debug symbols. Use `lldb` to diagnose test failures, crashes, or incorrect results:

```bash
# Break at a specific function and run a single test case
cd build && lldb ./tests/rego_test -- opa/v1/test/cases/testdata/v1/<suite>/<test>.yaml
(lldb) b <function_name>
(lldb) run

# Useful commands once stopped
(lldb) bt                   # backtrace
(lldb) frame variable       # show local variables
(lldb) p <expr>             # print expression
(lldb) n / s / c            # next / step / continue
```

This is particularly useful for debugging backend-specific failures (e.g., a test passes with OpenSSL but fails with mbedTLS) where the issue is in crypto or encoding logic.

### Generative Fuzzer (`rego_fuzzer`)

The `rego_fuzzer` tool generates random ASTs from the Trieste WF chain and tests that each pass handles all structurally valid inputs. It is parameterized by a transform (`file_to_rego`, `rego_to_bundle`, `json_to_bundle`, `bundle_to_json`), a sample count, and a seed.

```bash
# Basic run (default: 100 samples, random seed)
./build/tools/rego_fuzzer rego_to_bundle

# With specific count and seed, stop on first failure
./build/tools/rego_fuzzer rego_to_bundle -c 1000 -f -s 42

# Reproduce a specific failure
./build/tools/rego_fuzzer rego_to_bundle -c 1 -s <failing_seed>
```

Passing the fuzzer means running with `-c 1000` three times (different seeds) with exit code 0 each time. CTest targets (`rego_fuzzer_*`) run each transform with the default count.

## Investigating New OPA Features

When implementing a new OPA feature (especially new syntax or internal built-ins), **inspect OPA's IR plan output first** to understand the reference implementation:

1. Download the latest OPA binary matching `REGOCPP_OPA_VERSION`
2. Create a minimal policy in `.copilot/opa-ir-test/` that exercises the feature
3. Run: `opa build --bundle <dir> --target plan -e <entrypoint> -o bundle.tar.gz`
4. Extract and inspect `plan.json` — look for new entries in `static.builtin_funcs`, calling conventions, and undefined-handling patterns

This reveals internal built-in names (e.g., `internal.template_string`), argument conventions, and patterns that must be matched for compatibility. Always test with both constant and variable expressions since OPA's optimizer may fold constants.

## Scratch / Temporary Files

Use the `.copilot/` directory at the repo root for all temporary files, downloaded executables, test scripts, and scratch work produced during development. This keeps temporary artifacts visible and inspectable within the workspace rather than scattered in `/tmp`. The `.copilot/` directory is gitignored. Organize by task, e.g.:
- `.copilot/opa-ir-test/` — OPA IR analysis scratch files
- `.copilot/bin/` — downloaded tool binaries (e.g., OPA CLI)
