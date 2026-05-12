## 1. Public Documentation Rewrite

- [x] 1.1 Audit `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `docs/thread-safety.md`, `docs/compatibility-evidence.md`, and `tests/fixtures/README.md` for phase labels, milestone labels, and decision IDs.
- [x] 1.2 Rewrite `include/libbsa/archive.hpp` comments so archive variant, metadata, sink, and bulk extraction guidance uses durable behavior names while preserving caller obligations.
- [x] 1.3 Rewrite `include/libbsa/writer.hpp` comments so write-call execution controls and thread-safety guidance no longer expose planning IDs.
- [x] 1.4 Rewrite `docs/thread-safety.md` so concurrency rules are described through caller-owned synchronization, distinct sinks, writer mutation limits, validation concurrency, and benchmark/reporting behavior.
- [x] 1.5 Rewrite `docs/compatibility-evidence.md` and `tests/fixtures/README.md` so compatibility evidence, generated fixtures, and benchmark policies use stable public wording.

## 2. Regression Coverage

- [x] 2.1 Add or update documentation policy tests to scan the affected public surfaces for phase labels, milestone labels, and decision IDs.
- [x] 2.2 Update thread-safety documentation tests to assert durable wording such as distinct sinks, caller-owned synchronization, write-call execution controls, generated fixture policy, and compatibility evidence instead of planning IDs.
- [x] 2.3 Run a focused token scan over the affected public surfaces to confirm the reported planning identifiers are gone.
- [x] 2.4 Build and run the relevant documentation policy tests with the repo's Windows CTest preset.
- [x] 2.5 Run `openspec validate remove-planning-identifiers-from-public-docs --strict` and confirm the change remains apply-ready.
