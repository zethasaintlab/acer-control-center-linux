# Engineering Workflow & Agent Collaboration

**Status:** Current workflow guidance  
**Applies to:** Hydrogen and any delegated project agents  
**Project:** Acer Control Center — Linux Edition

## 1. Operating model

The primary agent owns the task outcome and coordinates investigation, implementation and review. Specialized agents are optional collaborators, not a mandatory chain of approvals.

Work outcome-first:
- Understand the desired result and relevant constraints.
- Inspect the actual repository and current state.
- Choose the most suitable investigation and implementation method.
- Make the smallest coherent change that achieves the objective.
- Validate the result and report evidence.

Do not prescribe commands, file-by-file edits or tool sequences unless the task is risky, reproducibility requires them, or the user explicitly requests that level of detail.

## 2. Source of truth

Use this precedence:
1. Actual current checkout, Git state, code and tests.
2. Current model-specific hardware audit and runtime evidence.
3. docs/PROJECT_STATE.md for reconciled status and pending work.
4. docs/DESIGN_SYSTEM.md for product UX and design contract.
5. Architecture decisions in docs/ARCHITECTURE_ADR.md.
6. Historical reports, previous agent conversations and older screenshots.

A handoff is context, not proof. If the local workspace differs from the public branch or project-state document, preserve the local work and reconcile the difference before editing.

## 3. Agent configuration and delegation

- Keep subagents model-agnostic. Use model inheritance unless a measured task-specific reason requires an override.
- Do not hard-code a particular Gemini model or effort level in repository instructions.
- hardware-auditor is read-only and focuses on exact-model capability evidence.
- security-reviewer is read-only and reviews privilege boundaries, inputs, D-Bus/Polkit, service and packaging security.
- ui-critic is read-only and evaluates task flow, information architecture, actual implementation and visual/accessibility quality.
- The primary agent remains responsible for integrating findings and implementation.

Delegate when it provides independent expertise or parallel review. Do not delegate merely to create process overhead.

## 4. Prompt and task sizing

Use the smallest brief that carries the context the agent needs.

| Work type | Prompt style |
|---|---|
| Small follow-up to an existing result | A few direct instructions: decision, next outcome and relevant constraint |
| Normal engineering task | Objective, context, acceptance result and safety boundary |
| New project stage or major redesign | A clear stage brief with current state, goals, constraints and completion criteria |
| Hardware, firmware, security or destructive operation | Detailed scope, exact authorization, prohibited actions, abort conditions and recovery expectations |

Do not repeat master instructions in every follow-up. Trust agents to choose routine tools and methods. Ask for a plan only when scope, risk or architecture justifies it; do not turn every task into a planning exercise.

## 5. Typical delivery loop

1. **Reconcile:** inspect branch, HEAD, working tree and relevant docs.
2. **Frame:** identify the user outcome, constraints and acceptance conditions.
3. **Investigate:** gather decisive source, runtime and documentation evidence.
4. **Decide:** explain material trade-offs and select an approach.
5. **Implement:** keep changes cohesive and preserve existing architecture unless a change is justified.
6. **Verify:** run relevant tests/build/static checks; use mocks for hardware behavior.
7. **Review:** inspect the diff for correctness, safety, regressions and scope creep.
8. **Handoff:** summarize what changed, evidence, tests, limitations and next decision.

This is a guide, not a rigid checklist. Skip irrelevant stages and combine work when appropriate.

## 6. Hardware investigation lifecycle

Classify evidence carefully:
- Static AML/driver evidence identifies a possible implementation.
- A standard kernel node proves that an interface is exposed, not that every value is safe.
- A runtime query proves only that a particular query returned a result; include any mailbox writes or side effects.
- Physical verification requires observed behavior on the target laptop.

Never infer physical support from a mock, similar SKU, product family or firmware method name. Do not invoke firmware methods or make hardware writes without separate, explicit authorization for the exact operation.

For fan and battery work, consult docs/HARDWARE_AUDIT_A715_59G.md and docs/PROJECT_STATE.md before changing capability labels or enabling a control.

## 7. Design workflow

Consult docs/DESIGN_SYSTEM.md. The next UI phase starts with task ownership, navigation and screen composition, not CSS/QML color edits. DAMX informs functional workflow exploration; Endfield research informs visual language. Neither replaces direct user-task reasoning or accessibility review.

A substantial redesign should show structural differences: information architecture, navigation, screen composition, component behavior and state design. Keep hardware and telemetry values truthful throughout prototypes.

## 8. Verification and reporting

- Run only checks relevant to the change.
- Report exact tests and commands actually run.
- Distinguish source review, unit tests, GUI tests, mocks, CI and physical tests.
- Do not claim a build or CI pass if no such run occurred.
- Keep test output and generated assets in appropriate project-local paths.
- A task completion report should be concise but sufficient for another agent to continue without repeating completed work.

## 9. Session handoff

When changing models or starting a new session:
- Read docs/PROJECT_STATE.md and docs/WORKFLOW.md.
- Inspect the actual checkout before assuming the public branch matches.
- Preserve local uncommitted work.
- Continue the pending stage only after reconciling its authorization and evidence status.
- Do not restart completed investigations merely because conversational context changed.
