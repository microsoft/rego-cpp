---
name: multi-perspective-planning
description: 'Run a multi-perspective planning process for non-trivial design decisions. Use when: the shape of a solution is uncertain, planning new language features, new passes, API changes, AST restructuring, or cross-cutting concerns. Spawns four competing sub-planners, synthesises with rebuttal resolution, and stress-tests via adversarial review. Supports checkpointing so planning can be restarted at any step.'
argument-hint: 'Describe the design decision or feature to plan.'
---

# Multi-perspective Planning Process

Run four sub-planners in parallel to generate competing plans, synthesise them with explicit rebuttal resolution, then stress-test the result with an adversarial review loop before presenting for approval.

## When to Use

Use the full process for design decisions where the shape of the solution is uncertain: new language features, new passes, API changes, AST restructuring, or cross-cutting concerns that touch multiple pipeline stages.

For tasks that are primarily implementation of a well-understood algorithm (e.g. a new built-in function with a clear OPA specification), a single conservative plan with emphasis on incremental testing is sufficient. Use the full process when the design is uncertain, not when the algorithm is known.

## Checkpointing

Each step writes a checkpoint file to `.copilot/planning/<task-slug>/` so that planning can be resumed at any point if the session is interrupted or the user wants to restart from a specific step.

### Checkpoint format

Each checkpoint is a markdown file with YAML frontmatter, named `step-<N>-<name>.md`. The frontmatter carries metadata; the body carries the natural-language content (plans, evaluations, rebuttals) in readable markdown.

```markdown
---
task_slug: <task-slug>
step: <N>
step_name: <name>
timestamp: <ISO-8601>
status: completed
---

<step content in markdown>
```

### Checkpoint files by step

| Step | File | Body contents |
|------|------|---------------|
| 0 | `step-0-context.md` | Task description, files read, context summary |
| 1 | `step-1-sub-plans.md` | Each lens plan under its own `## Heading` |
| 2 | `step-2-rebuttals.md` | Identified conflicts and rebuttal arguments (or "No conflicts found") |
| 3 | `step-3-synthesis.md` | Evaluation and synthesised plan |
| 4 | `step-4-adversarial.md` | Each iteration's findings and revisions as separate sections |
| 5 | `step-5-final.md` | Final plan and summary |

### Resuming from a checkpoint

When the user says "resume planning" or "restart from step N":

1. Read the checkpoint directory `.copilot/planning/<task-slug>/`
2. Find the highest completed step checkpoint
3. Load its data and continue from the next step
4. If the user specifies a step number, discard checkpoints for that step and later, then re-run from that step using earlier checkpoint data as input

### Task slug

Derive the task slug from the first invocation: lowercase the task description, replace non-alphanumeric characters with hyphens, truncate to 50 characters. Store the slug in every checkpoint for cross-reference.

### Using checkpoints as subagent context

Because checkpoint files are plain markdown, subagents can read them directly using file tools. Instead of inlining all prior context into a subagent prompt, **point the subagent at the checkpoint files**:

- Tell the subagent the checkpoint directory path (`.copilot/planning/<task-slug>/`).
- Instruct it to read the specific checkpoint files it needs (e.g., "Read `step-0-context.md` for the task description and `step-1-sub-plans.md` for the competing plans").
- This keeps prompts short, avoids token duplication, and ensures subagents always see the full untruncated content.

For example, when spawning the synthesis-lens agent in Step 3, instead of pasting all four plans inline, prompt it with:
> Read `.copilot/planning/<task-slug>/step-0-context.md` for the task description, `step-1-sub-plans.md` for the four competing plans, and `step-2-rebuttals.md` for any rebuttal arguments. Then produce a synthesised plan.

## Procedure

### Step 0 — Gather context

Before spawning any planners, gather sufficient context about the task:

- Read the relevant source files, WF definitions, and existing tests.
- Inspect OPA's reference implementation if the feature involves OPA compatibility.
- Identify affected passes, tokens, and API surfaces.
- Summarise the gathered context into a task description that all subagents will receive.

**Checkpoint**: Write `.copilot/planning/<task-slug>/step-0-context.md` with the task description, files read, and context summary.

### Step 1 — Gather sub-plans

Spawn **four fresh subagents** in parallel using the lens agents. Each agent receives the same task description and context but plans through a different lens:

| Agent | Focus |
|-------|-------|
| `speed-lens` | Runtime performance, low allocations, minimal passes, cache efficiency |
| `security-lens` | Defence in depth, safe error handling, bounded resources, fuzz coverage |
| `usability-lens` | Clarity, readability, correctness, consistent naming, one-concept-per-pass |
| `conservative-lens` | Smallest diff, maximum reuse, no speculative generality, backwards compat |

Prompt each agent with:
> Here is the task: [task description and relevant context]. Produce a numbered plan following the output format defined in your instructions.

**Checkpoint**: Write `.copilot/planning/<task-slug>/step-1-sub-plans.md` with each lens plan under its own heading.

### Step 2 — Identify conflicts and run rebuttals

Read all four lens outputs and identify *substantive design conflicts* — cases where two or more lenses propose incompatible approaches ("use A" vs. "use B" where both cannot coexist). Different emphasis on the same approach is not a conflict.

If conflicts are found:
- For each conflict, spawn the disagreeing lens agents in parallel (fresh subagents, by name). Each receives: (a) the specific conflict description, (b) its own original recommendation, (c) the opposing recommendation(s), and (d) instructions to make its strongest case for why its approach should be chosen, directly addressing the opponent's arguments.
- **One rebuttal round only** — no counter-rebuttals. The adversarial review loop (Step 4) catches remaining issues.
- If no substantive conflicts are found, skip this step entirely.

**Checkpoint**: Write `.copilot/planning/<task-slug>/step-2-rebuttals.md` with identified conflicts and rebuttal arguments (or note that no conflicts were found).

### Step 3 — Evaluate and synthesise

Review the four sub-plans yourself and produce a short evaluation covering:

- **Convergence**: where two or more planners agree on the same approach. High convergence suggests a clearly correct design.
- **Unique insights**: ideas that appear in only one plan and are worth incorporating.
- **Conflicts**: where plans disagree. For each conflict, summarise the rebuttal arguments from each side (if rebuttals were run) and state which perspective you favour and why.
- **Gaps**: anything none of the four plans addressed.

Then spawn a **fresh `synthesis-lens` subagent**. Provide it with:
- The original task description.
- All four build sub-plans (labelled by perspective).
- Any rebuttal arguments (labelled by conflict and perspective).
- Your evaluation.

When rebuttals are present, synthesis receives structured arguments for each side rather than inferring them. The synthesis subagent must engage with the specific rebuttal arguments made rather than ignoring them.

**Checkpoint**: Write `.copilot/planning/<task-slug>/step-3-synthesis.md` with the evaluation and synthesised plan.

### Step 4 — Adversarial review loop

Run an iterative adversarial review loop on the draft plan from Step 3:

1. Spawn a subagent using the `adversarial-lens` agent. Provide it with the original task description, context, and the current draft plan. Prompt it with:
   > Here is the plan: [draft plan]. Your job is to break it — find hidden assumptions, untested edge cases, semantic divergence from OPA, stale-state bugs, off-by-one errors, consensus blind spots, and failure modes. Produce an attack report following the output format defined in your instructions. Classify each finding as MUST-ADDRESS, SHOULD-ADDRESS, or ACKNOWLEDGED (risk accepted).

2. Review the adversarial report yourself. For each finding:
   - **MUST-ADDRESS**: revise the plan to include a specific mitigation step.
   - **SHOULD-ADDRESS**: revise the plan if the fix is low-cost; otherwise note the accepted risk.
   - **ACKNOWLEDGED**: no plan change required.

3. If any MUST-ADDRESS or SHOULD-ADDRESS findings required plan changes, spawn a **different** adversarial subagent to review the revised plan. Repeat until the adversarial review finds no new MUST-ADDRESS issues.

4. If the loop has run **5 times** without converging, proceed to Step 5 anyway — present the remaining unresolved findings to the user for decision.

**Checkpoint**: Write `.copilot/planning/<task-slug>/step-4-adversarial.md` after each iteration (overwrite with cumulative iterations as separate sections).

### Step 5 — Present for approval

Present the final plan to the user along with a brief summary of:
- Key points of agreement across the build sub-planners.
- Notable trade-offs made during synthesis.
- Conflicts resolved via rebuttals and which perspective prevailed.
- Any minority opinions from individual sub-planners that were overruled.
- Adversarial attacks that were addressed and how, plus any that were acknowledged but not mitigated (with rationale).
- Number of adversarial review iterations and key issues caught.
- Any unresolved adversarial findings (if the loop hit the 5-iteration cap).

**Checkpoint**: Write `.copilot/planning/<task-slug>/step-5-final.md` with the final plan and summary.

## Execution Rules

- Fresh subagents for each phase (lens, rebuttal, synthesis, adversarial review) — no context contamination.
- Four constructive lens subagents run in parallel; conflict identification, rebuttals, synthesis, adversarial review, and user-facing presentation run sequentially.
- Rebuttal subagents for a single conflict run in parallel with each other (they argue independently).
- Lens phases are independent — do not allow one lens's output to shape another's. Rebuttals are a second pass; the original independent outputs are preserved and forwarded to synthesis alongside rebuttals.
- Synthesis must resolve disagreements explicitly, not average them away. When rebuttals are available, synthesis must engage with the arguments made rather than ignoring them.
- The adversarial review loop is mandatory before presenting to the user. Each iteration uses a fresh adversarial subagent.
- **Always write checkpoints** after completing each step, even if the step was trivial or skipped (write with empty data arrays).
- When resuming, **announce the resume point** to the user: "Resuming planning for '<task-slug>' from Step N."
