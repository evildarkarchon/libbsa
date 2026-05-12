## Context

The current policy-test layer grew quickly while libbsa added writer streaming, shared publish helpers, validation diagnostics, package-consumer smoke coverage, target-format rules, benchmark guidance, and documentation policies. Many of those tests still read source or docs and search for exact tokens. Some of those checks protect real static boundaries, but others are standing in for executable behavior that the test suite can now observe directly.

The change must stay focused on test coverage and validation shape. It should not alter public APIs, dependency choices, archive format behavior, or anything under `TES5Edit/`.

## Goals / Non-Goals

**Goals:**
- Classify each named policy test as a hard static boundary, documentation structure check, or behavior claim.
- Convert behavior claims into executable tests that fail because observable libbsa behavior regressed, not because an internal token moved.
- Keep static policy tests where the policy is inherently textual, such as public-header dependency walls, `TES5Edit/` isolation, CMake preset names, and documentation anchor coverage.
- Prefer generated fixtures, public APIs, internal helper tests, package-consumer builds, and malformed-input fixtures over ad hoc source parsing when they can prove the same policy.
- Preserve or improve the failure messages so future refactors can see which policy was violated.

**Non-Goals:**
- No production behavior changes beyond incidental fixes exposed by the new tests.
- No new third-party dependencies, benchmark frameworks, or filesystem portability work.
- No broad rewrite of all documentation tests; exact text checks should be narrowed only where they are brittle and a structural or executable oracle exists.
- No mutation, formatting, staging, or fixture use of `TES5Edit/`.

## Decisions

1. Use a classification pass before rewriting tests.

Each assertion in the named files should be tagged as one of:
- `behavior`: an observable runtime/build outcome, such as publish safety, validation warning emission, malformed parser rejection, package-consumer compileability, or disk-backed writer handling.
- `static-boundary`: a deliberate source or configuration constraint, such as no `TES5Edit` implementation coupling, no DirectXTex in public headers, no private writer helper types in public headers, Windows-only preset naming, and Doxygen input/exclusion boundaries.
- `doc-structure`: documentation coverage where the artifact under test is text, but the assertion can often be made less brittle by checking anchors, enumerated public codes, or generated lists instead of exact prose.

This keeps the implementation from deleting useful policy checks just because they are source-text based.

2. Replace behavior claims with the closest executable oracle.

Memory-bound writer policies should move toward writer execution and staging tests that use disk-backed sources, source mutation, sparse or generated fixtures, and bounded validation/extraction options. Publish policies should use `writer_publish_tests.cpp` plus writer-family save tests rather than searching for helper names. Warning coverage should use `validate_archive`, compatibility matrix rows, and public `compatibility_warning_code` values. Package integration should be proven by the existing package-consumer target or a focused build/smoke path. Parser malformed-input policy should use generated malformed manifests and stable `result` error codes.

3. Keep hard static checks small and explicit.

Some boundaries are not meaningfully executable from a unit test. Examples include public headers not exposing DirectXTex, implementation files not referencing `TES5Edit`, Doxygen excluding private inputs, CI retaining Windows static/shared presets, and benchmark guidance not becoming a default CTest gate. Those tests should remain, but they should be named as boundary checks and avoid overfitting to incidental local implementation tokens.

4. Do not replace one brittle source check with another hidden text check.

When a behavior claim is converted, the replacement should assert behavior through APIs, helper results, generated archive bytes, CTest/package-consumer execution, or structured documents. Text scanning remains acceptable for docs/config boundaries, but the new test should not merely search for a different private symbol.

## Risks / Trade-offs

- Some policy claims, especially memory behavior, are difficult to prove perfectly in portable unit tests -> Use the strongest practical behavioral oracle, then leave a narrow static boundary check only for the unobservable part.
- Converting tests may uncover real production bugs -> Keep the implementation phase prepared for small targeted fixes, but do not widen into unrelated refactors.
- Documentation tests can become vague if exact prose checks are removed wholesale -> Prefer structural checks against headings, public enum values, and required evidence fields.
- Package-consumer and benchmark checks can be expensive if folded into every unit run -> Keep expensive validation behind existing CMake/CTest targets and document which validation command proves the policy.

## Migration Plan

1. Inventory the named policy tests and record which assertions are behavior, static-boundary, or doc-structure.
2. Add or update executable tests for behavior claims before removing the corresponding source-token assertions.
3. Narrow retained source-text checks to hard boundaries with explicit names and failure messages.
4. Run focused unit labels for the touched areas, build or run the package-consumer smoke path where package behavior is changed, and run `openspec validate "replace-brittle-source-policy-tests" --strict`.

## Open Questions

- None.
