---
name: code-review
description: 'Perform a multi-perspective code review of rego-cpp changes. Use when: reviewing a release, auditing a branch diff, evaluating a PR, or performing a pre-merge code review. Launches five parallel review subagents (Security, Performance, Usability, Conservative, Adversarial), verifies key findings, synthesises a unified report with severity-ranked findings, and produces actionable remediation recommendations.'
---

# Multi-Perspective Code Review

Perform a structured code review by examining changes from five independent
perspectives, cross-checking findings against source code, and producing a
unified report with actionable recommendations.

## When to Use

- Before tagging a release
- Reviewing a large branch diff or PR
- Auditing a new subsystem (crypto, parsing, VM changes)
- When a single-perspective review would miss cross-cutting concerns

## Background

A single reviewer tends toward their own bias — a security expert over-flags
performance patterns, a performance expert under-flags input validation. This
skill runs five parallel reviews — four constructive perspectives and one
adversarial red team — then synthesises findings where multiple perspectives
converge or provide unique insight.

## Perspectives

| Perspective | Lens | Agent |
|-------------|------|-------|
| **Security** | Defence in depth, memory safety, bounded resources, error handling, adversarial inputs, C API boundaries, fuzz coverage | `plan-security` |
| **Performance** | Allocation minimisation, cache-friendly access, pass count, hot-path awareness, algorithmic complexity | `plan-speed` |
| **Usability** | Correctness, clarity, naming, WF precision, error message quality, one-concept-per-pass, API ergonomics | `plan-usability` |
| **Conservative** | Smallest diff, backwards compatibility, API stability, reuse, no speculative generality, blast radius | `plan-conservative` |
| **Adversarial** | Red-team attacks, hidden assumptions, untested edge cases, semantic divergence from OPA, consensus blind spots, breaking inputs | `plan-adversarial` |

## Procedure

### Step 1: Identify the Diff

Determine the commit range or branch diff to review.

```bash
# Example: changes since last release tag
git diff --stat v1.2.0..HEAD
```

Group changed files by subsystem (parser, builtins, VM, C API, build system,
wrappers) to assign review focus areas.

### Step 2: Launch Five Review Subagents

Spawn five Explore subagents **in parallel**, one per perspective. Each
subagent receives:

1. The same list of changed files and feature summary
2. The perspective-specific review lens (from the table above)
3. Specific files to examine based on the subsystem grouping
4. Instructions to classify findings by severity and provide file/line references

**Prompt template for each constructive subagent (Security, Performance, Usability, Conservative):**

> You are performing a {PERSPECTIVE}-focused code review of rego-cpp.
> The changes add: {FEATURE_SUMMARY}.
>
> Your review lens: **{LENS_DESCRIPTION}**
>
> THOROUGHNESS: thorough
>
> Please examine these files and report findings:
> {FILE_LIST_WITH_SPECIFIC_QUESTIONS}
>
> For each finding, classify severity as {SEVERITY_SCALE} and provide the
> file path and approximate line numbers. Return a structured report.

Severity scales per perspective:
- **Security**: CRITICAL / HIGH / MEDIUM / LOW / INFO
- **Performance**: HIGH / MEDIUM / LOW impact
- **Usability**: CONCERN / SUGGESTION / POSITIVE
- **Conservative**: BREAKING / HIGH-RISK / MEDIUM-RISK / LOW-RISK / OK

**Prompt template for the Adversarial subagent:**

> You are a red-team adversary reviewing rego-cpp changes.
> The changes add: {FEATURE_SUMMARY}.
>
> Your review lens: **Attack the implementation. Find hidden assumptions,
> untested edge cases, semantic divergence from OPA, consensus blind spots,
> and inputs that break the new code.**
>
> THOROUGHNESS: thorough
>
> Please examine these files and report attacks:
> {FILE_LIST_WITH_SPECIFIC_QUESTIONS}
>
> For each attack, classify confidence as HIGH (proven with a test case),
> MEDIUM (likely based on code analysis), or LOW (theoretical). Provide
> concrete adversarial inputs (Rego policies, JSON data) wherever possible.
> Identify any shared assumptions across the other review perspectives that
> may be wrong. Return a structured attack report.

Severity scale for Adversarial:
- **HIGH**: Concrete breaking input or proven semantic divergence from OPA
- **MEDIUM**: Likely failure based on code analysis, no concrete input yet
- **LOW**: Theoretical concern, conditions for failure are speculative

### Step 3: Verify Key Findings

After collecting all five reports, identify the highest-severity findings and
**spot-check them against source code**. Launch a verification subagent:

> For each claim below, read the relevant code and report whether the claim
> is CONFIRMED, PARTIALLY CONFIRMED, or REFUTED. Provide exact code evidence.
> {LIST_OF_CLAIMS_TO_VERIFY}

This step prevents false positives from propagating into the final report.
Mark any unverifiable claims as such.

### Step 4: Synthesise the Report

Produce a unified report with these sections:

#### Convergence
Where two or more perspectives agree on the same finding. High convergence
indicates high confidence.

#### Findings by Severity
A single table combining all verified findings, normalised to a unified
severity scale:

| Unified Severity | Mapping |
|-----------------|---------|
| CRITICAL / HIGH | Security CRITICAL/HIGH, Performance HIGH, Usability CONCERN (correctness bug), Conservative BREAKING, Adversarial HIGH |
| MEDIUM | Security MEDIUM, Performance MEDIUM, Usability CONCERN (non-correctness), Conservative HIGH-RISK, Adversarial MEDIUM |
| LOW | Security LOW, Performance LOW, Usability SUGGESTION, Conservative MEDIUM-RISK, Adversarial LOW |

Each finding gets: number, description, originating perspective(s), verification
status, file path and line references.

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

### Step 5: Calibrate Against Existing Test Coverage

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

## Output Format

The final report should be a structured markdown document (presented in chat,
not saved to a file unless requested) with the sections described in Step 4.

## Reference

- [Example remediation plan from v1.3.0 review](./references/v1.3.0-remediation-plan.md) —
  a concrete example of findings and the resulting fix plan.
