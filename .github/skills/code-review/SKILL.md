---
name: code-review
description: 'Perform a multi-perspective code review of rego-cpp changes. Use when: reviewing a release, auditing a branch diff, evaluating a PR, or performing a pre-merge code review. Launches four parallel constructive review subagents (Security, Performance, Usability, Conservative), synthesises findings, then runs a sequential adversarial gap-analysis pass. Verifies key findings, produces a unified report with severity-ranked findings and actionable remediation recommendations.'
---

# Multi-Perspective Code Review

Perform a structured code review by examining changes from four independent
constructive perspectives, synthesising their findings, then running an
adversarial gap-analysis pass to catch what the constructive reviewers missed.
Cross-check findings against source code and produce a unified report with
actionable recommendations.

## When to Use

- Before tagging a release
- Reviewing a large branch diff or PR
- Auditing a new subsystem (crypto, parsing, VM changes)
- When a single-perspective review would miss cross-cutting concerns

## Background

A single reviewer tends toward their own bias — a security expert over-flags
performance patterns, a performance expert under-flags input validation. This
skill runs four parallel constructive reviews, synthesises their findings,
then passes the result to an adversarial red-team reviewer that focuses on
gaps the constructive reviewers missed. This sequential adversarial step
produces higher-quality findings than running all five in parallel, because
the adversary knows what was already found and can focus on blind spots.

## Perspectives

### Constructive Reviewers (parallel)

| Perspective | Lens | Agent |
|-------------|------|-------|
| **Security** | Defence in depth, memory safety, bounded resources, error handling, adversarial inputs, C API boundaries, fuzz coverage | `security-lens` |
| **Performance** | Allocation minimisation, cache-friendly access, pass count, hot-path awareness, algorithmic complexity | `speed-lens` |
| **Usability** | Correctness, clarity, naming, WF precision, error message quality, one-concept-per-pass, API ergonomics | `usability-lens` |
| **Conservative** | Smallest diff, backwards compatibility, API stability, reuse, no speculative generality, blast radius | `conservative-lens` |

### Adversarial Reviewer (sequential, after synthesis)

| Perspective | Lens | Agent |
|-------------|------|-------|
| **Adversarial** | Red-team attacks, hidden assumptions, untested edge cases, semantic divergence from OPA, consensus blind spots, breaking inputs. **Runs after constructive reviewers and receives their synthesised findings.** | `adversarial-lens` |

## Procedure

### Step 1: Identify the Diff

Determine the commit range or branch diff to review.

```bash
# Example: changes since last release tag
git diff --stat v1.2.0..HEAD
```

Group changed files by subsystem (parser, builtins, VM, C API, build system,
wrappers) to assign review focus areas.

### Step 2: Launch Four Constructive Review Subagents

Spawn four Explore subagents **in parallel**, one per constructive perspective.
**Do NOT include the adversarial reviewer in this step** — it runs later in
Step 4.

Each subagent receives:

1. The same list of changed files and feature summary
2. The perspective-specific review lens (from the table above)
3. Specific files to examine based on the subsystem grouping
4. Instructions to classify findings by severity and provide file/line references

**Each subagent is independent — do not allow one reviewer's output to
influence another.**

**Prompt template for each subagent (Security, Performance, Usability, Conservative):**

> You are performing a {PERSPECTIVE}-focused code review of rego-cpp.
> The changes add: {FEATURE_SUMMARY}.
>
> Your review lens: **{LENS_DESCRIPTION}**
>
> THOROUGHNESS: thorough
>
> === PHASE 1 — INVENTORY ===
> Before writing findings, list every function, rewrite rule, pass definition,
> match arm, and significant code block in the changed files. Number them.
> This ensures systematic coverage.
>
> === PHASE 2 — ASSESS ===
> For each inventory item, check it against your focus areas. If no issues,
> write "No issues." Do not skip items.
>
> Please examine these files and report findings:
> {FILE_LIST_WITH_SPECIFIC_QUESTIONS}
>
> For each finding, classify severity as {SEVERITY_SCALE} and provide the
> file path and approximate line numbers. For bugs, provide a minimal test
> case or execution trace. If you cannot construct one, mark as UNVERIFIED.
> Return a structured report.

Severity scales per perspective:
- **Security**: CRITICAL / HIGH / MEDIUM / LOW / INFO
- **Performance**: HIGH / MEDIUM / LOW impact
- **Usability**: CONCERN / SUGGESTION / POSITIVE
- **Conservative**: BREAKING / HIGH-RISK / MEDIUM-RISK / LOW-RISK / OK

### Step 3: Synthesise Constructive Findings

After all four constructive reviewers return, synthesise their outputs:

1. **Deduplicate** — merge issues raised by multiple reviewers into a single
   entry, noting which perspectives flagged it.
2. **Classify** each unique issue by type: Security, Correctness, Performance,
   Quality.
3. **Assign unified severity** using this scale:

| Unified Severity | Mapping |
|-----------------|---------|
| CRITICAL / HIGH | Security CRITICAL/HIGH, Performance HIGH, Usability CONCERN (correctness bug), Conservative BREAKING |
| MEDIUM | Security MEDIUM, Performance MEDIUM, Usability CONCERN (non-correctness), Conservative HIGH-RISK |
| LOW | Security LOW, Performance LOW, Usability SUGGESTION, Conservative MEDIUM-RISK |

#### Convergence
Where two or more perspectives agree on the same finding. High convergence
indicates high confidence.

### Step 4: Adversarial Gap-Analysis Pass

After synthesising the four constructive reviewers' findings, spawn a **fresh**
adversarial subagent (`adversarial-lens`). Provide it with:
- The full list of changed files and feature summary
- The **complete synthesised findings from Step 3** so it knows what was
  already found

**Prompt template for the Adversarial subagent:**

> You are a red-team adversary performing gap-analysis on a rego-cpp code
> review. Four other reviewers (Security, Performance, Usability, Conservative)
> have already reviewed the changes. Their findings are listed below under
> EXISTING FINDINGS. Your job is to find what they MISSED.
>
> The changes add: {FEATURE_SUMMARY}.
>
> Do NOT re-report existing findings. Only report NEW issues. Focus on:
> - Code sections in NO existing finding (overlooked)
> - Issue categories not represented
> - Cross-component interactions no single lens would catch
> - Shared assumptions across the other reviewers that may be wrong
> - Untested edge cases and semantic divergence from OPA
> - Breaking inputs (provide concrete Rego policies / JSON data)
>
> THOROUGHNESS: thorough
>
> Please examine these files:
> {FILE_LIST_WITH_SPECIFIC_QUESTIONS}
>
> EXISTING FINDINGS:
> {SYNTHESISED_FINDINGS_FROM_STEP_3}
>
> For each attack, classify confidence as HIGH (proven with a test case),
> MEDIUM (likely based on code analysis), or LOW (theoretical). Provide
> concrete adversarial inputs wherever possible. Return a structured attack
> report.

Severity scale for Adversarial:
- **HIGH**: Concrete breaking input or proven semantic divergence from OPA
- **MEDIUM**: Likely failure based on code analysis, no concrete input yet
- **LOW**: Theoretical concern, conditions for failure are speculative

Merge the adversarial reviewer's new findings into the synthesised list using
the same unified severity scale, then proceed to verification.

### Step 5: Verify Key Findings

Identify the highest-severity findings (CRITICAL / HIGH) and **spot-check
them against source code**. Launch a verification subagent:

> For each claim below, read the relevant code and report whether the claim
> is CONFIRMED, PARTIALLY CONFIRMED, or REFUTED. Provide exact code evidence.
> {LIST_OF_CLAIMS_TO_VERIFY}

This step prevents false positives from propagating into the final report.
Mark each issue as:
- **Verified** — reproduced or confirmed by code inspection
- **Likely** — strong evidence but not yet reproduced
- **Unverified** — plausible but needs investigation

Downgrade or remove issues that cannot be substantiated after reasonable
investigation.

### Step 6: Produce the Report

Produce a unified report with these sections:

#### Findings by Severity
A single table combining all verified findings. Each finding gets: number,
description, originating perspective(s), verification status, file path and
line references.

#### Positive Highlights
Things the code does well, called out by any perspective. This provides
balanced feedback and reinforces good patterns.

#### Recommendations
Ordered by priority. Split into:
- **Before release**: correctness bugs, UB, security issues
- **After release**: performance optimisation, tech debt, hardening

#### Trade-offs
Where perspectives conflict (e.g., security wants more validation, performance
wants less overhead), state the conflict and the recommended resolution.

### Step 7: Calibrate Against Existing Test Coverage

Before finalising recommendations, check whether existing test suites
(OPA conformance tests, regocpp.yaml, fuzzer) already cover the flagged
scenarios. The OPA test suite is comprehensive — findings about "missing
test coverage" must be verified against:

```bash
# List OPA test suites
ls build/opa/v1/test/cases/testdata/v1/

# Check specific suite coverage
grep 'note:' build/opa/v1/test/cases/testdata/v1/{suite}/*.yaml
```

Drop or downgrade recommendations that duplicate existing OPA coverage.

## Chunking Large Targets

If the code under review exceeds ~300 lines, split it into logical segments
(by file, pass, or function group) and run Steps 2–5 independently on each
segment. After all segments are reviewed, merge and deduplicate findings
across segments before the final report.

This ensures every section receives full attention. Do not skip this for
large targets — reviewer quality degrades significantly when a single prompt
must cover hundreds of lines.

## Guardrails

- Each reviewer subagent must be a fresh instance — no context contamination
  between reviewers.
- The adversarial reviewer runs AFTER the constructive reviewers, not in
  parallel. It receives the synthesised findings to avoid duplicate work and
  focus on gaps.
- Do not skip the verification step for CRITICAL/HIGH issues. Unverified
  critical findings must be explicitly marked as such.
- Prefer concrete code suggestions over vague recommendations.

## Output Format

The final report should be a structured markdown document (presented in chat,
not saved to a file unless requested) with the sections described in Step 6.

## Reference

- [Example remediation plan from v1.3.0 review](./references/v1.3.0-remediation-plan.md) —
  a concrete example of findings and the resulting fix plan.
