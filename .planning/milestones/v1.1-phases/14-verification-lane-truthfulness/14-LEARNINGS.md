---
phase: 14
phase_name: "verification-lane-truthfulness"
project: "libbsa"
generated: "2026-05-14"
counts:
  decisions: 8
  lessons: 6
  patterns: 7
  surprises: 5
missing_artifacts:
  - "14-UAT.md"
---

# Phase 14 Learnings: verification-lane-truthfulness

## Decisions

### Keep Release Package Proof Inside CTest
Release package-proof ownership remains on the existing `package_consumer_smoke` and `package_consumer_runtime_dll_copy` tests instead of adding a second smoke pipeline.

**Rationale:** The phase goal was to make the supported Release lanes truthful through the normal checked-in `ctest` graph, not through a CI-only or ad hoc package verification path.
**Source:** 14-01-SUMMARY.md

---

### Back ASan With Real MSVC Instrumentation
The supported `windows-msvc-asan-static` lane uses real `/fsanitize=address` compilation and linking through `LIBBSA_ENABLE_MSVC_ASAN`.

**Rationale:** A preset named `asan` is not enough to prove hardening coverage; the repository must contain checked-in compiler/linker evidence that instrumentation is enabled.
**Source:** 14-01-PLAN.md

---

### Disable MSVC STL Annotation Checks For The Supported ASan Lane
The ASan lane keeps real instrumentation but disables STL annotation ODR checks when linking against prebuilt dependencies.

**Rationale:** Full-lane verification found `LNK2038` annotation mismatches against dependencies that were not built with matching MSVC STL annotation settings.
**Source:** 14-01-SUMMARY.md

---

### Treat Runtime DLL Propagation As Part Of The Lane Contract
Runtime DLL copying for ASan and transitive dependencies is part of the supported verification lane contract.

**Rationale:** Catch2 discovery and package-consumer smoke must run from checked-in build outputs without relying on the caller's `PATH`.
**Source:** 14-01-SUMMARY.md

---

### Use Role-Aware Matrix Include Rows In CI
The main Windows CI job uses `matrix.include` rows so workflow output can show both the lane role and the concrete preset.

**Rationale:** The configure/build/test commands stay bound to checked-in preset names while the GitHub Actions UI remains readable for maintainers.
**Source:** 14-02-SUMMARY.md

---

### Keep MSVC ASan As A Separate Top-Level CI Job
The MSVC AddressSanitizer lane remains a separate top-level job that runs `windows-msvc-asan-static` directly instead of becoming a fifth main-matrix row.

**Rationale:** A separate job makes hardening coverage explicit and prevents workflow output from implying ASan coverage through a generic matrix entry.
**Source:** 14-02-SUMMARY.md

---

### Keep One Concrete README Quick Path
README keeps `windows-msvc-debug-static` as the single concrete quick-start command path, then groups the rest of the supported lanes by role.

**Rationale:** Maintainers need one short entry point for the common inner loop while the wider supported matrix remains documented accurately.
**Source:** 14-03-SUMMARY.md

---

### Keep Planning Summaries Layered
`PROJECT.md`, `ROADMAP.md`, and `STATE.md` stay summary-scoped while `14-CONTEXT.md` remains the detailed lane contract.

**Rationale:** Project-wide planning files should stay truthful without duplicating command-level lane detail or rewriting v1.0 history.
**Source:** 14-03-SUMMARY.md

---

## Lessons

### Fast Policy Checks Do Not Prove Full ASan Lane Health
The initial ASan work passed fast policy checks but failed full-lane verification due to MSVC STL annotation mismatches.

**Context:** The failing `windows-msvc-asan-static` verification exposed that textual policy assertions must be paired with real configure/build/ctest execution for hardening lanes.
**Source:** 14-01-SUMMARY.md

---

### ASan Runtime Availability Is A Test Discovery Concern
ASan runtime DLLs must be available before and during Catch2 discovery, not only during the final test executable run.

**Context:** Catch2 pre-test discovery and downstream package-consumer smoke could not run reliably until ASan and transitive runtime DLLs were copied beside built binaries.
**Source:** 14-01-SUMMARY.md

---

### Workflow Topology Assertions Need Durable Anchors
The first workflow-topology assertion approach was too brittle for the main matrix block.

**Context:** The final policy gate used exact role/preset tokens and count-based topology facts, reserving block extraction for the ASan job-separation fact where it mattered.
**Source:** 14-02-SUMMARY.md

---

### Generated State Updates Still Need Human-Facing Truth Checks
GSD state helpers advanced counters and appended decisions, but some human-facing `STATE.md` summary fields stayed stale.

**Context:** The execution metadata patch corrected the note, milestone-plan count, latest-execution line, and percent before the docs commit.
**Source:** 14-02-SUMMARY.md

---

### Unsupported-Lane Exclusion Wording Is Not The Same As Absence
Truthful docs may mention unsupported environments such as WSL explicitly while still preserving a Windows-only support boundary.

**Context:** The policy suite initially used an over-strict absence assertion; the final check split workflow Windows-hosting facts from fixture-policy exclusion wording.
**Source:** 14-03-SUMMARY.md

---

### Chronology Matters When Updating Project Summaries
The truthful Release/ASan verification matrix had to be described as a v1.1 Phase 14 outcome, not backfilled into v1.0 history.

**Context:** Plan 03 explicitly corrected project and roadmap summaries without moving detailed lane-contract prose out of the phase context.
**Source:** 14-03-PLAN.md

---

## Patterns

### Shared Verification-Matrix Contract Helper
Model supported verification lanes once in `validation_policy_tests.cpp` and assert each surface independently against that contract.

**When to use:** Use when multiple repo surfaces must agree on the same build/test/support contract without allowing one stale document to validate another.
**Source:** 14-01-SUMMARY.md

---

### CTest-Owned Package Proof
Keep install/export and downstream package-consumer smoke inside the normal CTest graph through `package_consumer_smoke` and `package_consumer_runtime_dll_copy`.

**When to use:** Use when a supported lane must prove downstream package usability as part of the same command maintainers run locally.
**Source:** 14-01-SUMMARY.md

---

### Runtime DLL Mirroring For Runnable Verification Lanes
Copy target, ASan, and transitive runtime DLLs into test and package-consumer output directories before discovery and smoke execution.

**When to use:** Use for Windows verification lanes where tests or downstream consumers must run from build outputs without external `PATH` assumptions.
**Source:** 14-01-SUMMARY.md

---

### Repo-Reading Policy Tests For CI Truthfulness
Use Catch2 policy tests that read workflow YAML directly and assert the supported topology locally.

**When to use:** Use when CI shape is part of the product's verification contract and drift should fail before GitHub execution.
**Source:** 14-02-SUMMARY.md

---

### Role-Aware CI Names With Preset-Bound Commands
Expose human-facing lane roles in job names while keeping configure/build/test commands bound only to concrete preset names.

**When to use:** Use when CI output must be understandable but execution must remain identical to local maintainer commands.
**Source:** 14-02-SUMMARY.md

---

### Quick-Path Plus Role-Matrix Documentation
Document one concrete quick path first, then summarize the wider supported matrix by Debug, Release package-proof, and ASan hardening roles.

**When to use:** Use for maintainer docs that need to serve both fast local setup and accurate support-matrix communication.
**Source:** 14-03-SUMMARY.md

---

### Summary-Layered Planning Updates
Keep project and roadmap documents summary-level while phase context owns detailed commands and lane contracts.

**When to use:** Use when a phase changes cross-project claims but the detailed operational contract belongs in the phase artifact.
**Source:** 14-03-SUMMARY.md

---

## Surprises

### MSVC ASan Hit STL Annotation Link Mismatches
The new ASan lane failed with `LNK2038` `annotate_string`, `annotate_vector`, and `annotate_optional` mismatches.

**Impact:** The phase needed a blocking follow-up fix that preserved real instrumentation while disabling incompatible STL annotation checks for the supported lane.
**Source:** 14-01-SUMMARY.md

---

### Package Consumer Smoke Needed Explicit ASan Runtime Propagation
The downstream package-consumer smoke executable could not run reliably until ASan and transitive runtime DLLs were copied beside it.

**Impact:** The runtime-copy helper and smoke flow had to be extended so the ASan lane was actually runnable, not merely configurable.
**Source:** 14-01-SUMMARY.md

---

### Workflow Matrix Parsing Was More Brittle Than Expected
The first workflow-topology assertion approach was too brittle for the main matrix block.

**Impact:** The final policy design shifted to exact token and count-based checks for the matrix, reducing false failures while preserving topology coverage.
**Source:** 14-02-SUMMARY.md

---

### State Metadata Drifted Despite Helper Updates
The GSD state helpers updated some fields but left several human-facing summary fields stale.

**Impact:** Plan execution needed a manual metadata correction pass before the final docs commit could truthfully describe the phase status.
**Source:** 14-02-SUMMARY.md

---

### WSL Mention Was A Truthful Exclusion, Not A Scope Violation
An over-strict policy assertion treated mentioning WSL as a failure even though explicitly excluding unsupported WSL was truthful documentation.

**Impact:** The policy suite was corrected to test Windows-hosted workflow facts separately from fixture-policy exclusion wording.
**Source:** 14-03-SUMMARY.md
