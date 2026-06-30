---
name: finalize-pr
description: 'Finalize a rego-cpp PR for release. Use when: wrapping up a branch, preparing a release, cutting a version, or when instructed to finalize a PR. Confirms review/remediation is done, determines branch changes, bumps the version across the library and all wrappers, writes a CHANGELOG entry, updates documentation, runs the editor lens over new comments, and runs format/build/test gates to give CI the best chance of passing.'
---

# Finalize PR

End-to-end preparation of a branch for merge and release. This skill subsumes
and replaces the old `bump-version` skill: version bumping is step 2 here.

## When to Use

- A feature or remediation branch is functionally complete and you want it
  release-ready.
- Cutting a new version (major, minor, or patch).
- Final pass before opening or merging a PR to `main`.

## Prerequisites

This skill assumes the code is already written and working. It does **not**
design features or fix bugs. If review or remediation is still outstanding, run
the `code-review` skill (and apply fixes) first.

## Procedure

Run the steps in order. Each step is gated on the previous one. Stop and ask the
user if any step reveals unfinished work.

### Step 0 — Confirm review and remediation are complete

Before changing anything, confirm with the user:

> "Has this branch been code-reviewed and have all agreed remediations been
> applied? finalize-pr assumes the code is final."

Do not proceed until the user confirms. If they have not reviewed, recommend the
`code-review` skill first.

### Step 1 — Determine what changed in the branch

Establish the merge base and the diff to be released.

```bash
git fetch origin main
git merge-base HEAD origin/main          # base commit
git diff --stat $(git merge-base HEAD origin/main)..HEAD
git log --oneline $(git merge-base HEAD origin/main)..HEAD
```

Group changed files by subsystem (parser, builtins, VM, C/C++ API, wrappers,
build, docs). Summarize the user-visible effect of each group — this summary
feeds the version decision (Step 2) and the CHANGELOG (Step 3). Record the diff
file list; the editor lens (Step 5) needs it.

### Step 2 — Bump the version

Decide the new version from the change summary, then **confirm the proposed
number with the user before writing any files**:

- **major**: breaking API/ABI or behavioural changes.
- **minor**: new features, new builtins, backward-compatible additions.
- **patch**: bug fixes and hardening only.

Update the version string in **all** of the following. Missing any one creates a
mismatch between the library and its wrapper packages.

| File | Field / Pattern | Example |
|------|----------------|---------|
| `VERSION` | Entire file contents | `1.6.0` |
| `ports/rego-cpp/vcpkg.json` | `"version": "X.Y.Z"` | `"version": "1.6.0"` |
| `wrappers/python/setup.py` | `VERSION = "X.Y.Z"` | `VERSION = "1.6.0"` |
| `wrappers/rust/regorust/Cargo.toml` | `version = "X.Y.Z"` | `version = "1.6.0"` |
| `wrappers/dotnet/Rego/Rego.csproj` | `<Version>X.Y.Z</Version>` | `<Version>1.6.0</Version>` |
| `examples/rust/Cargo.toml` | `regorust = { version = "X.Y.Z" }` | `regorust = { version = "1.6.0" }` |
| `examples/dotnet/example/example.csproj` | `Version="X.Y.Z"` | `Version="1.6.0"` |
| `examples/dotnet/MyPolicy/MyPolicy.csproj` | `Version="X.Y.Z"` | `Version="1.6.0"` |

Then verify nothing references the old version and refresh the Rust lockfile:

```bash
grep -rn '"OLD_VERSION"' wrappers/ examples/ ports/ VERSION
cargo update -p regorust --manifest-path wrappers/rust/regorust/Cargo.toml  # if a lockfile exists
```

**OPA version sync (only if `REGOCPP_OPA_VERSION` in the root `CMakeLists.txt`
changed in this branch):**

| File | Field / Pattern | Example |
|------|----------------|---------|
| `.github/copilot-instructions.md` | `targets Rego vX.Y.Z` | `targets Rego v1.17.1` |

`REGOCPP_OPA_VERSION` tracks the compatible OPA release and moves independently
of the rego-cpp version.

**Common mistakes:**
- Forgetting a wrapper version — they live in different directories and formats.
- Stale `Cargo.lock` after editing `Cargo.toml`, which fails CI.

### Step 3 — Write the CHANGELOG entry

Prepend a new entry to `CHANGELOG` using the existing format: a
`## YYYY-MM-DD - Version X.Y.Z` header, a one-paragraph summary, then sections
(e.g. **Security / Hardening**, **Features**, **Fixes**) drawn from the Step 1
summary. Describe user-visible effects, not internal refactors. Match the prose
style of prior entries (concrete, specific, present tense).

### Step 4 — Update documentation

Update docs to reflect the changes where appropriate:

- `README.md` — supported features, examples, OPA target version.
- `include/rego/rego.hh` / `rego_c.h` Doxygen comments for changed public API.
- Wrapper READMEs (`wrappers/*/README.md`) when their surface changed.
- `doc/` Doxygen sources if structure changed.

Skip docs that are unaffected. Do not invent documentation for unchanged code.

### Step 5 — Run the editor lens

Invoke the `editor-lens` agent over the branch diff from Step 1 to police every
comment the PR added or changed. The lens:

- removes internal codenames / process leakage and historical commentary,
- fixes grammar, and
- enforces the one-line (≤80 char) comment limit, deleting by default.

It applies one-line edits in place automatically and reports any multi-line
comment it wants to keep under "Approval required". Surface that list to the
user and get explicit approval before restoring any multi-line comment.

Pass the lens the diff file list and tell it to read the files directly.

### Step 6 — Best-effort CI pass

Give CI (`.github/workflows/pr_gate.yml`) the best chance of passing. Run from a
configured build directory:

```bash
ninja regocpp_format                       # clang-format 18; CI rejects unformatted code
ninja rego_test rego                       # build must be clean
ctest --output-on-failure                  # full local + OPA conformance suites
```

Also re-run any suites relevant to the change and the fuzzer over **disjoint**
seed ranges (start seeds ≥ count apart; the fuzzer tests `[seed, seed+count)`):

```bash
for t in file_to_rego rego_to_bundle; do
  for s in 1 1001 2001; do ./tools/rego_fuzzer $t -c 1000 -f -s $s || break; done
done
```

Report the gate results. If anything fails, stop and surface it — do not paper
over a failure to finish the skill.

## Completion

Summarize for the user: new version, CHANGELOG entry, docs touched, editor-lens
edits (and any pending approvals), and the gate results. Do not commit, tag, or
push unless the user asks — and commits are signed (`git commit -s -S`).
