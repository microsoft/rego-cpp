# rego-cpp Change Surface for OPA Compatibility

When OPA releases a new version, the following areas of rego-cpp may need updates. This document maps OPA change types to specific files and patterns in the rego-cpp codebase.

## 1. Built-in Functions

### Files
- `src/builtins/*.cc` — Individual built-in implementations, one file per OPA namespace
- `src/builtins/builtins.hh` — Factory function declarations for each namespace
- `src/builtins.cc` — Lookup dispatch tree and `BuiltInsDef` manager (including `is_deprecated()`)

### How Built-ins Are Resolved
The `BuiltInsDef::lookup()` function in `src/builtins.cc` uses a binary dispatch tree keyed on the namespace prefix (text before the first `.`). When a new namespace is added, this dispatch tree must be extended. The tree is hand-coded, not auto-generated.

### Adding a New Built-in to an Existing Namespace
1. Add the implementation function in the appropriate `src/builtins/<namespace>.cc`
2. Register it in the namespace's factory function (the function declared in `builtins.hh`)
3. No changes to the dispatch tree needed

### Adding a New Namespace
1. Create `src/builtins/<namespace>.cc`
2. Add factory declaration to `src/builtins/builtins.hh`
3. Add routing entry in the lookup dispatch tree in `src/builtins.cc`
4. Add the source file to `src/CMakeLists.txt`

### Deprecating a Built-in
Add the function name to the `deprecated` vector in `BuiltInsDef::is_deprecated()` in `src/builtins.cc`.

### Marking a Built-in as Unavailable
Use `BuiltInDef::placeholder()` to create an entry that returns a descriptive error message without implementing the function. This is used for built-ins that cannot be supported (e.g., `http.send` requires network access).

## 2. Parser / Language Syntax

### Files
- `src/parse.cc` — Rego lexer and parser (Trieste-based)
- `include/rego/rego.hh` — AST token type definitions (`TokenDef` globals)

### What Triggers Parser Changes
- New keywords (e.g., `every`, `contains`, `in` were added historically)
- New operators
- Grammar rule changes (e.g., new expression forms)
- Changes to import syntax

### Token Definition Pattern
New tokens are added as `inline const auto` globals in `include/rego/rego.hh`:
```cpp
inline const auto NewToken = TokenDef("rego-newtoken", flag::print);
```

## 3. Evaluation / Compiler Passes

### Files
- `src/interpreter.cc` — Pass pipeline management
- `src/virtual_machine.cc` — VM execution engine
- `src/resolver.cc` — Variable resolution and unification
- `src/dependency_graph.cc` — Rule dependency analysis

### What Triggers Evaluation Changes
- Changes to how partial evaluation works
- Changes to conflict resolution between rules
- New evaluation capabilities (e.g., new comprehension types)
- Changes to the `with` keyword behavior

## 4. Bundle Format

### Files
- `src/bundle.cc` — Bundle loading orchestration
- `src/bundle_json.cc` — JSON bundle format
- `src/bundle_binary.cc` — Binary bundle format (rego-cpp specific)
- `src/rego_to_bundle.cc` — Compilation to bundle format

### What Triggers Bundle Changes
- Changes to OPA's bundle manifest format
- New metadata fields in bundles
- Changes to the wasm/plan format (rego-cpp uses its own VM, but tracks format)

## 5. Version and Test Infrastructure

### Files
- `CMakeLists.txt` — `REGOCPP_OPA_VERSION` variable, OPA repo clone
- `README.md` — Version compatibility statement and EBNF grammar
- `CHANGELOG` — Release history
- `VERSION` — rego-cpp semantic version
- `tests/CMakeLists.txt` — Test suite configuration

### Conformance Tests
OPA tests are automatically cloned from the OPA repo at the tag matching `REGOCPP_OPA_VERSION`. When the version is bumped:
- Must `rm -rf build/opa` first — CMake only clones if the directory doesn't exist
- New tests are picked up automatically
- Tests requiring unimplemented built-ins are skipped via `all_builtins_available()`
- Platform-specific tests (e.g., time zones) are skipped based on compiler capabilities

### Running Targeted Tests
The OPA test cases live in subdirectories under `build/opa/v1/test/cases/testdata/v1/`. Run a specific category:
```bash
./tests/rego_test -wf opa/v1/test/cases/testdata/v1/<subdir>
```
Subdirectory names match OPA built-in names with no separators (e.g., `regexfind`, `numbersrangestep`, `stringinterpolation`). Always run targeted tests first during development, then the full suite for final validation.

## 6. Analyzing OPA's IR for New Features

When OPA adds a significant new feature (new syntax, new internal built-in), **inspect the IR plan** OPA produces to understand the implementation pattern. This is critical for ensuring rego-cpp's compilation matches OPA's semantics.

### Setup
```bash
# Download latest OPA binary (always upgrade before analyzing!)
curl -L -o /tmp/opa https://github.com/open-policy-agent/opa/releases/download/v{VERSION}/opa_linux_amd64_static
chmod +x /tmp/opa

# Use .copilot/ in the repo for scratch files
mkdir -p .copilot/opa-ir-test
```

### Build and Inspect IR
```bash
# Create a minimal policy exercising the feature
cat > .copilot/opa-ir-test/policy.rego << 'EOF'
package test
p := <feature expression>
EOF

# Build IR plan
/tmp/opa build --bundle .copilot/opa-ir-test --target plan -e test/p -o .copilot/opa-ir-test/bundle.tar.gz

# Extract and inspect
mkdir -p .copilot/opa-ir-test/output && cd .copilot/opa-ir-test/output
tar xzf ../bundle.tar.gz
python3 -m json.tool plan.json
```

### What to Look For
- **New internal built-in names** in `static.builtin_funcs` (e.g., `internal.template_string`)
- **Calling convention**: argument types, arity, return type
- **Undefined handling patterns**: OPA often wraps potentially-undefined expressions in `BlockStmt` + `Set` patterns (empty set = undefined, set{value} = defined)
- **Constant folding**: OPA's optimizer may fold constant expressions into a single value — test with both constant and variable expressions to see the unoptimized IR

### Example: String Interpolation (`internal.template_string`)
OPA compiles `$"hello {expr} world"` into:
1. `MakeArrayStmt` with capacity = number of chunks + expressions
2. `ArrayAppendStmt` for literal text chunks (as string_index operands)
3. For each `{expr}`:
   - If potentially undefined: wrap evaluation in `BlockStmt` + `MakeSetStmt`/`SetAddStmt`, then append the set
   - If constant: evaluate and `ArrayAppendStmt` the value directly
4. `CallStmt` to `internal.template_string` with the array as sole argument

## 7. Typical No-Impact Changes in OPA

These OPA changes do NOT affect rego-cpp:
- Go runtime performance improvements
- OPA REST API / server changes
- Plugin system / discovery changes
- OPA CLI tool changes (flags, subcommands)
- Logging / telemetry changes
- Wasm compiler changes (rego-cpp has its own VM)
- OPA Docker image changes
- OPA SDK changes (Go-specific)
