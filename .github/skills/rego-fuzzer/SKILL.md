---
name: rego-fuzzer
description: 'Pass the rego-cpp Trieste fuzzer for a given pass collection. Use when: verifying that compiler passes are robust to all valid WF inputs, debugging fuzzer failures, fixing generative testing regressions, or validating pass changes against random inputs. The fuzzer generates random ASTs from the Trieste well-formedness chain and checks that each pass handles all structurally valid inputs without crashing or producing malformed output.'
argument-hint: 'Specify the transform to fuzz (file_to_rego, rego_to_bundle, json_to_bundle, bundle_to_json) or say "all" to run all transforms.'
---

# Passing the Rego Fuzzer

Verify that rego-cpp compiler passes are robust to all valid inputs by running the Trieste generative fuzzer.

## When to Use

- After modifying or adding a compiler pass
- After changing a well-formedness (WF) definition
- After adding new AST node types or rewrite rules
- When a CI fuzzer run has failed and you need to reproduce and fix the issue
- As a final validation step before merging pass pipeline changes

## Background

The `rego_fuzzer` tool uses Trieste's generative testing framework. For each pass in a transform pipeline, it:

1. Reads the **input well-formedness definition** for that pass
2. Generates random ASTs that are structurally valid according to that WF
3. Runs the pass on each generated AST
4. Checks that the output conforms to the pass's **output well-formedness definition**

This catches edge cases that hand-written tests miss — any structurally valid input the WF permits can be generated.

## Transforms

The fuzzer is parameterized by a **transform**, which is a named collection of passes:

| Transform | Description | Passes |
|-----------|-------------|--------|
| `file_to_rego` | Parsing through structured AST | 18 passes in `src/file_to_rego.cc` |
| `rego_to_bundle` | Structured AST to executable bytecode | 11 passes in `src/rego_to_bundle.cc` |
| `json_to_bundle` | JSON bundle to internal bundle format | Passes in bundle pipeline |
| `bundle_to_json` | Internal bundle to JSON bundle format | Passes in bundle pipeline |

## Procedure

### Step 1: Build the Fuzzer

The fuzzer binary is built when `REGOCPP_BUILD_TOOLS` is enabled (it is in all standard presets):

```bash
cd build && ninja rego_fuzzer
```

The binary is located at `./build/tools/rego_fuzzer`.

### Step 2: Determine Which Transforms to Test

- If the user specified a transform, use that one.
- If the user said "all", test all four: `file_to_rego`, `rego_to_bundle`, `json_to_bundle`, `bundle_to_json`.
- If the user didn't specify, infer from the files they changed:
  - Changes in `src/file_to_rego.cc` or `src/parse.cc` → `file_to_rego`
  - Changes in `src/rego_to_bundle.cc` → `rego_to_bundle`
  - Changes in `src/bundle_json.cc` or `src/bundle.cc` → `json_to_bundle` and `bundle_to_json`
  - Changes in `include/rego/rego.hh` (WF definitions) → all transforms
  - Changes in `src/internal.hh` → all transforms

### Step 3: Run the Fuzzer

For each transform, run the fuzzer **three times** with count 1000. **Do not provide a seed** — the fuzzer picks a random seed each time, ensuring the three runs cover different inputs. (The fuzzer tests seeds sequentially from the starting seed, so providing consecutive seeds like 1, 2, 3 would result in nearly complete overlap.) Use `--failfast` (`-f`) to stop on the first failure in each run.

```bash
cd build

# Run 1
./tools/rego_fuzzer <transform> -c 1000 -f

# Run 2
./tools/rego_fuzzer <transform> -c 1000 -f

# Run 3
./tools/rego_fuzzer <transform> -c 1000 -f
```

**Passing criterion**: all three runs must produce output containing **no** `Failed pass:` lines. **Do not rely on the exit code alone** — the fuzzer may exit 0 even when a pass fails. Always read the tail of the output (e.g., pipe through `tail -5`) and check for `Failed pass:` or `Failed!` text.

If a run fails, proceed to Step 4 before running additional transforms.

### Step 4: Diagnose Failures

When the fuzzer fails, it produces structured output with the following sections (see [references/example-failure.md](./references/example-failure.md) for a complete annotated example):

```
Testing x1, seed: 1452196526

: unexpected rego-templatestring, expected a rego-STRING, rego-INT, rego-FLOAT,
rego-true, rego-false or rego-null                                              $85
~~~
(rego-templatestring)


============
Pass: index_strings_locals, seed: 1452196526
------------
(top ...)              <-- full input AST (what was fed into the pass)
------------
(top ...)              <-- full output AST (what the pass produced)
============
Failed pass: index_strings_locals, seed: 1452196526
```

The output structure is:

1. **Header**: `Testing xN, seed: S`
2. **WF error message**: Describes the well-formedness violation — which node type was found and what types were expected. Includes a node id (`$NN`), an underline (`~~~`), and the offending node shown as `(rego-X)`.
3. **Pass and seed**: `Pass: <pass_name>, seed: <seed>` — identifies which pass failed.
4. **Input AST**: The full AST that was generated and fed **into** the failing pass (between `---` separators). This is the WF-valid input that triggered the bug.
5. **Output AST**: The full AST the pass **produced** (between `---` and `===` separators). Compare this against the pass's output WF to see exactly what's wrong.
6. **Failure summary**: `Failed pass: <pass_name>, seed: <seed>` — the last line, repeating the identification.

A successful run produces only the header line and exits with code 0:

```
Testing x3, seed: 42
```

To reproduce a failure for debugging, re-run with the exact seed and count 1:

```bash
./tools/rego_fuzzer <transform> -c 1 -s <failing_seed>
```

Add `-l Info` for additional logging if the AST dump is not sufficient:

```bash
./tools/rego_fuzzer <transform> -c 1 -s <failing_seed> -l Info
```

#### How to Read the Failure

1. **Start from the error message** at the top — it tells you the node type that violated the output WF and what was expected instead.
2. **Find the offending node in the input AST** — search for that node type in the input dump. This shows how the fuzzer-generated input contains a structurally valid (per the input WF) combination that the pass doesn't handle.
3. **Check the output AST** — the pass left the offending node unchanged or transformed it incorrectly, violating the output WF.
4. **Read the pass's WF definitions** — the input WF tells you what shapes the pass must be prepared to handle; the output WF tells you what shapes it must produce.

#### Common Failure Categories

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| WF violation after pass X | A rewrite rule in pass X produces output not matching `wf_X` | Add or fix a rewrite rule to handle the input pattern |
| Unhandled node type | A pattern the pass doesn't match but the input WF allows | Add a rewrite rule or error rule for the pattern |
| Crash / assertion failure | Null dereference or out-of-bounds access in a rewrite rule | Add guards or handle the empty-children case |
| Infinite loop (timeout) | Fixpoint pass rules that don't converge | Add `dir::once` or fix the rules so they make progress |

### Step 5: Fix and Re-verify

After fixing a failure:

1. **Re-run with the specific failing seed** to confirm the fix:
   ```bash
   ./tools/rego_fuzzer <transform> -c 1 -s <failing_seed>
   ```

2. **Re-run the full three-pass verification** (Step 3) to ensure no regressions.

3. **Run the standard test suite** to check the fix didn't break deterministic tests:
   ```bash
   cd build && ./tests/rego_test -wf tests/regocpp.yaml
   ```

### Step 6: Report Results

Summarize the results for each transform:

```
Fuzzer results for <transform>:
  Run 1 (seed 1, count 1000): PASS
  Run 2 (seed 2, count 1000): PASS
  Run 3 (seed 3, count 1000): PASS
```

If any failures were found and fixed, include:
- The failing seed(s) and pass name(s)
- A brief description of the root cause
- What was changed to fix it

## Tips

- **Start with a low count** (e.g., `-c 10`) when iterating on a fix to get fast feedback, then scale up to `-c 1000` for the final verification.
- **The seed is deterministic** — the same seed always produces the same random ASTs, making failures reproducible.
- **Error rules are the primary fix** for fuzzer failures. When the fuzzer finds an input your pass doesn't handle, add an error rule that catches the pattern and produces a meaningful `err()` node. This is preferable to trying to handle every exotic WF-valid combination.
- **Read the WF definition** of the failing pass's input — it tells you exactly what shapes the fuzzer might generate.
- **CTest also runs the fuzzer** with the default count (100). To run fuzzer tests via CTest:
  ```bash
  ctest --test-dir build -R rego_fuzzer
  ```
