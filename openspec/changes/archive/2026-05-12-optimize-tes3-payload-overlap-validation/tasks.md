## 1. Parser Implementation

- [x] 1.1 Add a local TES3 payload span representation that stores archive-absolute start/end offsets after existing hash, path, offset, and archive-bound validations succeed.
- [x] 1.2 Collect only non-empty payload spans so zero-byte TES3 entries do not create artificial overlap intervals.
- [x] 1.3 Replace the insertion-order prior-span scan with a single sort by offset/end followed by adjacent overlap checks that return the existing TES3 payload-overlap `format_error`.
- [x] 1.4 Preserve the existing TES3 data-section-relative offset compatibility comment or update it only if the surrounding logic changes enough to require clarification.

## 2. Test Coverage

- [x] 2.1 Add or extend repository-owned TES3 test helpers so tests can create valid hash-ordered archives with explicit raw payload offsets.
- [x] 2.2 Add a malformed TES3 test where two non-empty payload spans overlap after data-section-relative offset conversion and assert `libbsa::error_code::format_error`.
- [x] 2.3 Add a TES3 test proving adjacent non-overlapping spans and zero-byte entries at payload boundaries still materialize successfully.
- [x] 2.4 Add a malformed TES3 test proving unsorted stored hash records are rejected before payload-overlap validation changes failure precedence.
- [x] 2.5 Add a large non-overlapping TES3 metadata test whose hash order differs from payload-offset order, avoiding wall-clock assertions while covering the ordered-span validation path.

## 3. Verification

- [x] 3.1 Run the focused TES3 reader test target or equivalent CTest filter and fix any failures.
- [x] 3.2 Run the broader available unit/CTest suite for the configured build and confirm no unrelated archive parser regressions.
- [x] 3.3 Confirm `openspec status --change "optimize-tes3-payload-overlap-validation"` reports the change ready for implementation.
