## Why

Several policy tests assert that specific source tokens appear or do not appear instead of exercising the behavior those policies are meant to protect. That makes routine refactors noisy while still allowing real regressions to slip through when the expected token remains in place.

## What Changes

- Audit the source-text policy tests named by this change and classify each assertion as either a hard source boundary or behavior that should be exercised directly.
- Replace brittle source-token assertions with executable tests for memory bounds, writer publish behavior, validation warning coverage, package-consumer integration, and malformed parser inputs where the behavior can be observed through libbsa APIs or internal test helpers.
- Keep source-text policy tests only for hard boundaries that are intentionally textual, such as forbidden public-header dependencies, forbidden `TES5Edit/` coupling, and other static constraints that cannot be exercised reliably at runtime.
- Remove or narrow obsolete token checks once equivalent behavior-backed coverage exists, without weakening the policy they were originally protecting.

## Capabilities

### New Capabilities
- `behavior-backed-policy-tests`: Defines how policy-sensitive regression coverage should prefer executable behavior checks while retaining source-text checks for hard static boundaries.

### Modified Capabilities
- `writer-safe-publish`: Clarifies that writer publish safety is proven primarily through helper-level and writer-level behavior, with source-text checks reserved for the minimal delegation boundary that cannot be observed externally.

## Impact

- Affected tests: `tests/unit/bounded_memory_policy_tests.cpp`, `tests/unit/benchmark_policy_tests.cpp`, `tests/unit/validation_policy_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp`.
- Affected specs: one new test-suite policy capability plus a small delta to `writer-safe-publish`.
- Public API impact: none expected.
- Dependency impact: none.
- `TES5Edit/` remains read-only and is not a test fixture mutation target.
