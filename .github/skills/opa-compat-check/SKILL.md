---
name: opa-compat-check
description: 'Check OPA Rego version compatibility for rego-cpp. Use when: updating OPA version, checking for new OPA releases, auditing rego-cpp compatibility, planning OPA upgrade work, reviewing OPA release notes for rego-cpp impact. Fetches OPA release notes, compares with current rego-cpp support, and produces an actionable compatibility report.'
argument-hint: 'Optional: specific OPA version to check against (e.g. "1.9.0"). Omit to check latest.'
---

# OPA Rego Compatibility Check

Determine what changes (if any) rego-cpp needs to maintain compatibility with the latest OPA Rego release.

## When to Use

- Checking if a new OPA version has been released since rego-cpp's last update
- Planning work to upgrade rego-cpp to a newer OPA version
- Auditing the current compatibility gap between rego-cpp and OPA
- Reviewing what changed in OPA that affects rego-cpp

## Procedure

Follow these steps in order. Do not skip steps.

### Step 1: Determine Current rego-cpp OPA Version

Read the `REGOCPP_OPA_VERSION` variable from the root `CMakeLists.txt`:

```
grep "REGOCPP_OPA_VERSION" CMakeLists.txt
```

This is the OPA version rego-cpp currently targets. Record it as `CURRENT_VERSION`.

Also read the `VERSION` file in the repo root to get the rego-cpp library version.

### Step 2: Fetch Latest OPA Release Information

Fetch the OPA releases page to find the latest version and release notes:

1. Fetch `https://github.com/open-policy-agent/opa/releases` with query "latest release version"
2. Identify the latest stable release version. Record it as `LATEST_VERSION`.
3. If the user specified a target version, use that instead of the latest.

If `LATEST_VERSION` equals `CURRENT_VERSION`, report that rego-cpp is up to date and stop.

### Step 3: Fetch Release Notes for Each Intermediate Version

For each OPA version between `CURRENT_VERSION` (exclusive) and `LATEST_VERSION` (inclusive):

1. Fetch `https://github.com/open-policy-agent/opa/releases/tag/v{VERSION}` with query "release notes changes new features built-ins deprecations breaking changes Rego language"
2. Also fetch `https://www.openpolicyagent.org/docs/latest/policy-reference/` with query "built-in functions list" to get the current built-in function catalog.

Collect all changes across versions.

### Step 4: Categorize Changes by Impact Area

Sort every change into the categories below. Refer to [change-surface.md](./references/change-surface.md) for details on how each category maps to rego-cpp code.

| Category | What to Look For | rego-cpp Impact |
|----------|-----------------|-----------------|
| **New Built-in Functions** | New entries in OPA's built-in function list | Add implementation in `src/builtins/` |
| **Modified Built-in Semantics** | Changed behavior of existing built-ins | Update existing builtin impl |
| **Deprecated Built-ins** | Functions marked deprecated | Update `is_deprecated()` in `src/builtins.cc` |
| **Removed Built-ins** | Functions removed entirely | Remove from builtin registry |
| **Language Syntax Changes** | New keywords, operators, or grammar rules | Update parser in `src/parse.cc`, tokens in `include/rego/rego.hh` |
| **Evaluation Semantics** | Changes to how policies are evaluated | Update compiler passes or VM in `src/virtual_machine.cc` |
| **Bundle Format Changes** | Changes to OPA bundle structure | Update `src/bundle.cc`, `src/bundle_json.cc`, `src/bundle_binary.cc` |
| **Conformance Test Changes** | New or modified OPA test cases | Automatically picked up when version is bumped |
| **No rego-cpp Impact** | Go runtime changes, CLI changes, server changes, plugin API changes | Document as not applicable |

### Step 5: Cross-reference with rego-cpp Built-in Coverage

For any new or modified built-ins, check whether rego-cpp already implements them:

1. Search `src/builtins/` for the function name
2. Check `src/builtins.cc` lookup dispatch to see if the namespace is routed
3. Check `src/builtins/builtins.hh` for the namespace factory declaration

Mark each new built-in as one of:
- **Already implemented** — no action needed
- **Namespace exists, function missing** — add to existing file
- **New namespace** — new file needed in `src/builtins/`, plus registration

See [builtin-pattern.md](./references/builtin-pattern.md) for the implementation pattern.

### Step 6: Produce the Compatibility Report

Generate a structured report with the following sections:

```markdown
# OPA Rego Compatibility Report

## Version Summary
- **rego-cpp version**: {from VERSION file}
- **Current OPA target**: {CURRENT_VERSION}
- **Latest OPA release**: {LATEST_VERSION}
- **Versions to bridge**: {list of intermediate versions}

## Required Changes

### New Built-in Functions
For each new built-in:
- Function signature (name, args, return type)
- Which `src/builtins/` file to modify or create
- Complexity estimate (trivial/moderate/complex)

### Built-in Semantic Changes
For each changed built-in:
- What changed and how
- Which file to modify

### Deprecations
For each deprecated item:
- Add to `is_deprecated()` list in `src/builtins.cc`

### Language Changes
For each syntax/grammar change:
- What changed in the grammar
- Parser modifications needed
- New tokens needed in `include/rego/rego.hh`
- Documentation updates needed in `README.md` grammar section

### Evaluation Changes
For each evaluation behavior change:
- Description of change
- Affected compiler passes or VM behavior

### Bundle Format Changes
For each format change:
- What changed
- Files to update

## Version Bump Checklist
- [ ] Update `REGOCPP_OPA_VERSION` in `CMakeLists.txt`
- [ ] Update OPA version reference in `README.md`
- [ ] Sync `README.md` grammar section with OPA grammar
- [ ] Implement new built-ins (list each)
- [ ] Update `is_deprecated()` for newly deprecated built-ins
- [ ] Apply parser changes (if any)
- [ ] Apply evaluation changes (if any)
- [ ] Run OPA conformance tests: `cmake --preset debug-clang-opa && ninja -C build && ctest --test-dir build -R opa`
- [ ] Update `CHANGELOG` with new version entry
- [ ] Update `VERSION` file

## No Impact (documented for completeness)
- List of OPA changes that don't affect rego-cpp
```

### Step 7: Analyze OPA IR for Complex Features

For any **language syntax changes** or **new internal built-ins** identified in Step 4, use OPA's plan IR to understand the implementation before coding:

1. **Ensure latest OPA binary** is available:
   ```bash
   curl -L -o /tmp/opa https://github.com/open-policy-agent/opa/releases/download/v{LATEST_VERSION}/opa_linux_amd64_static
   chmod +x /tmp/opa
   /tmp/opa version  # verify it matches LATEST_VERSION
   ```

2. **Create minimal test policies** in `.copilot/opa-ir-test/`, one per feature:
   ```bash
   mkdir -p .copilot/opa-ir-test
   cat > .copilot/opa-ir-test/policy.rego << 'EOF'
   package test
   p := <feature expression using both constants and variables>
   EOF
   ```

3. **Build and inspect the IR**:
   ```bash
   /tmp/opa build --bundle .copilot/opa-ir-test --target plan -e test/p -o .copilot/opa-ir-test/bundle.tar.gz
   mkdir -p .copilot/opa-ir-test/output && cd .copilot/opa-ir-test/output
   tar xzf ../bundle.tar.gz && python3 -m json.tool plan.json
   ```

4. **Look for**:
   - New entries in `static.builtin_funcs` — these are internal built-ins rego-cpp must implement
   - The exact calling convention (arg types, arity, return type)
   - How undefined values are handled (e.g., `BlockStmt` + `Set` wrapping patterns)
   - Whether OPA's optimizer folds constant cases differently from variable cases (test both)

5. **Record findings** in the compatibility report under the relevant section.

See [change-surface.md § Analyzing OPA's IR](./references/change-surface.md) for detailed examples.

### Step 8: Validate Findings

If the workspace has a build directory and the OPA test suite available:

1. **Remove the stale OPA clone** before reconfiguring: `rm -rf build/opa`
2. **Reconfigure**: `cmake .. --preset debug-clang-opa` (this re-clones at the new version tag)
3. **Run targeted tests** for specific areas during development:
   ```bash
   ./tests/rego_test -wf opa/v1/test/cases/testdata/v1/<subdir>
   ```
   Subdirectory names match OPA built-in names without separators (e.g., `regexfind`, `numbersrangestep`, `stringinterpolation`).
4. **Run the full conformance suite** for final validation:
   ```bash
   ctest -R rego_test_opa --output-on-failure
   ```

## Important Notes

- OPA release notes are at `https://github.com/open-policy-agent/opa/releases`
- OPA built-in reference is at `https://www.openpolicyagent.org/docs/latest/policy-reference/`
- The OPA conformance test suite is cloned from `https://github.com/open-policy-agent/opa/` at the tag matching `REGOCPP_OPA_VERSION`
- rego-cpp only implements the Rego **language** and **built-in functions**. Changes to OPA's Go runtime, REST API, plugin system, or CLI tool do not apply.
- Some built-ins may be platform-dependent (e.g., time zone functions require `cpp_lib_chrono >= 201907L`). Flag these in the report.
- `http.send` is a stub in rego-cpp — new HTTP-related functionality is typically marked as a placeholder.
