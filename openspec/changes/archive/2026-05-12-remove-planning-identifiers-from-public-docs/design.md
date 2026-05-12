## Context

The affected surfaces are public library documentation: Doxygen comments in `include/libbsa/archive.hpp` and `include/libbsa/writer.hpp`, published docs under `docs/`, and fixture guidance under `tests/fixtures/README.md`. These files currently mix stable behavior descriptions with internal planning labels such as phase and decision IDs.

The implementation already has durable behavior concepts that can replace those labels: supported archive variants, stable metadata shape, caller-owned extraction sinks, write-call execution controls, generated fixture policy, compatibility evidence policy, and benchmark input policy. The change should rewrite wording around those concepts while preserving the compatibility constraints behind the current comments.

## Goals / Non-Goals

**Goals:**
- Remove planning-era identifiers from consumer-facing headers, docs, and fixture guidance.
- Preserve the behavioral meaning of existing comments and documentation.
- Keep public wording focused on stable libbsa concepts such as archive variants, thread-safety ownership, writer finalization controls, compatibility evidence, fixtures, and benchmark data.
- Add validation that checks the affected public documentation surfaces for forbidden planning identifiers.

**Non-Goals:**
- No public API signature changes.
- No archive parser, writer, compression, DirectXTex, or vcpkg dependency changes.
- No rewrite of planning artifacts or historical OpenSpec/GSD files.
- No broad editorial rewrite beyond the requested public documentation cleanup.

## Decisions

1. Treat planning identifiers as forbidden only in consumer-facing surfaces.

   Rationale: planning artifacts are allowed to retain their own history and traceability. Public headers and docs should explain durable behavior to downstream consumers without requiring knowledge of internal roadmap labels.

   Alternative considered: remove the identifiers repo-wide. That would create unnecessary churn across planning history and archived artifacts without improving the library's public contract.

2. Replace each identifier with the behavior it was standing in for.

   Rationale: simply deleting phrases would risk weakening useful guidance. `D-23` should become the distinct-sink and caller-owned synchronization rule; `Phase 12` writer comments should become write-call execution-control wording; fixture and compatibility references should describe generated synthetic data and optional evidence paths directly.

   Alternative considered: move identifiers into footnotes or parentheticals. That still exposes internal planning labels in public docs and leaves the consumer-facing problem intact.

3. Validate with a narrow documentation policy test.

   Rationale: this is inherently a text-surface contract, so a focused static check is appropriate. The check should scan only public headers, published docs, and fixture guidance, and it should avoid blocking identifiers inside planning artifacts.

   Alternative considered: rely on manual review. Manual review is easy to miss because the identifiers are short and can appear in comments far from the docs that generated the issue.

## Risks / Trade-offs

- [Risk] A rewrite removes compatibility rationale while removing internal labels. → Mitigation: update wording by behavior area and preserve the existing thread-safety, fixture, compatibility, and writer-control meanings.
- [Risk] A broad token scan blocks legitimate planning artifacts. → Mitigation: scope validation to the public surfaces named in the proposal.
- [Risk] The forbidden-token list becomes too narrow. → Mitigation: cover the reported identifier families with patterns for phase labels, milestone labels, and decision IDs instead of only exact strings.
- [Risk] Existing tests still expect `D-23` in public headers. → Mitigation: update those tests to assert durable thread-safety wording such as distinct sinks, caller-owned synchronization, and canonical guidance links.
