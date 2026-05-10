# Phase 11: compatibility-warnings-validation-api-and-hardening - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-10
**Phase:** 11-compatibility-warnings-validation-api-and-hardening
**Areas discussed:** Public validation API shape, Typed compatibility warning model, Evidence and hardening proof shape

---

## Public validation API shape

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Primary public entry point | Free function | `validate_archive(path, options = {}) -> result<validation_report>`; small, stateless, and separate from strict reader open. | yes |
| Primary public entry point | Reader static method | `archive_reader::validate(path, options = {})`; familiar but makes reader carry validation concepts. | |
| Primary public entry point | Validator object | `archive_validator{options}.validate(path)`; useful for reusable state but heavier public surface. | |
| Fatal validation failures | Report contains errors | Validation report contains archive-level fatal diagnostics; `result` failures stay for setup/call failures. | yes |
| Fatal validation failures | Result fails for fatal archive errors | Malformed/unsupported archives return `result` errors; report only covers valid archives with warnings. | |
| Fatal validation failures | Always report object | Even missing/unreadable files become report errors, blurring I/O/setup and archive diagnostics. | |
| Report detail | Summary plus diagnostics | Expose validity, parseable metadata, errors, and warnings without duplicating reader APIs. | yes |
| Report detail | Full inspected inventory | Include per-entry validation status and selected metadata. | |
| Report detail | Diagnostics only | Contain only errors/warnings; callers reopen when they want metadata. | |
| Public options | Small policy options | Target-family expectation plus optional extraction/decompression checks. | yes |
| Public options | No public options yet | Validate only with default strict policy. | |
| Public options | Broader options | Target family, strictness, decompression depth, warning filters, and local corpus/reference settings. | |

**User's choice:** The user selected the recommended option for all four questions.
**Notes:** The API should be a small dependency-light public validation surface next to, but not inside, the strict reader facade.

---

## Typed compatibility warning model

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Warning identifiers | Enum codes | Public `enum class compatibility_warning_code` plus warning records. | yes |
| Warning identifiers | String codes | Stable strings such as `ba2.starfield.compat.method0`. | |
| Warning identifiers | Category plus numeric code | Category plus numeric identifier. | |
| Severity | Advisory vs risky | Two-level severity for display, sorting, or escalation. | yes |
| Severity | No severity | Consumers branch only on warning code. | |
| Severity | Three-level severity | More expressive but risks false precision. | |
| Location/context | Archive plus optional entry path | Code, severity, message, and optional archive path. | yes |
| Location/context | Rich location | Include offsets, record/chunk indexes, and format-family context. | |
| Location/context | Message-only context | Put all context into human text. | |
| First warning scenarios | Representative known quirks | At least three warnings across BSA/BA2 families with practical generated evidence. | yes |
| First warning scenarios | Writer-output policy only | Focus on libbsa-produced output target policy checks. | |
| First warning scenarios | Malformed-adjacent warnings | Warn for almost-invalid but still parseable edge cases. | |

**User's choice:** The user selected the recommended option for all four questions.
**Notes:** Tests should assert warning identifiers and severity/path shape, not message text.

---

## Evidence and hardening proof shape

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Evidence catalog | Committed Markdown plus machine check | Human-readable catalog with tests/scripts checking every warning code appears. | yes |
| Evidence catalog | JSON/YAML catalog only | Fully machine-readable catalog under docs or tests. | |
| Evidence catalog | Test assertions only | Each warning test documents its own evidence. | |
| Malformed coverage | Single matrix document/check | Consolidated matrix spanning all supported families and hardening classes. | yes |
| Malformed coverage | Per-family manifests only | Extend current manifests and keep coverage expectations local to each test. | |
| Malformed coverage | Test-name convention | Rely on test names/tags only. | |
| Optional local corpus | Smoke/compare only | Optional `requires-game-fixture` tests validate/load/extract selected local data, skipped by default. | yes |
| Optional local corpus | No optional corpus work yet | Document policy but add no hooks. | |
| Optional local corpus | Broader local corpus runner | Add a general scan/compare harness for arbitrary archive directories. | |
| Sanitizer path | Additive non-Windows preset/docs | Clang/GCC sanitizer preset or documented command path, keeping Windows CI unchanged. | yes |
| Sanitizer path | CI lane now | Add a sanitizer CI job immediately. | |
| Sanitizer path | Docs only | Document suggested commands without committed preset support. | |

**User's choice:** The user selected the recommended option for all four questions.
**Notes:** The proof shape should strengthen traceability without making optional data or unsupported sanitizer tooling part of default acceptance.

---

## the agent's Discretion

- Exact header placement and private helper layout for validation types/functions.
- Exact warning-code names and first three representative warning scenarios, provided each is cataloged and fixture-backed.
- Exact sanitizer preset name and selected CTest labels, provided the path is additive and toolchain-gated.

## Deferred Ideas

None - discussion stayed within phase scope.
