# M001-k9wo8b: Audit and Stabilization Baseline

**Vision:** Make libbsa's current support claims truthful and actionable by auditing all implemented Bethesda archive families, auditing the public API story, and fixing a risk-bounded tranche of the highest-risk bugs or testing gaps with durable proof.

## Success Criteria

- A truthful coverage/gap matrix exists for all current archive families and agreed capability axes.
- The public API capability story is audited against real headers, docs, and package-consumer usage.
- A risk-bounded tranche of highest-risk audit gaps is fixed with durable proof.
- Remaining gaps are explicitly deferred with rationale and future ownership.
- Default build/test/package-consumer verification passes without requiring local copyrighted fixtures.
- TES5Edit remains untouched and read-only.

## Slices

- [x] **S01: S01** `risk:high` `depends:[]`
  > After this: A durable matrix shows every current archive family against reader, writer, round-trip, malformed, validation, compatibility, API, and docs proof, with ranked gaps instead of vague support claims.

- [ ] **S02: S02** `risk:high` `depends:[]`
  > After this: The package-consumer story is audited against the matrix, public headers stay dependency-light C++20, and any immediately justified helper/API/doc gaps are identified or implemented with proof.

- [ ] **S03: Fixture and Round-trip Gap Closure** `risk:high` `depends:[S01]`
  > After this: The highest-risk generated-fixture or round-trip gaps found by the audit are fixed with durable Catch2/policy/fixture proof across the affected archive families.

- [ ] **S04: Error and Validation Stabilization** `risk:medium` `depends:[S01,S03]`
  > After this: High-risk inconsistent result/error/validation behavior discovered by the audit is stabilized and covered by tests, without redesigning the public error model.

- [ ] **S05: Integrated Confidence Pass** `risk:medium` `depends:[S02,S03,S04]`
  > After this: Default build/test/package-consumer verification passes, the matrix is updated with fixed/deferred status, optional compatibility paths remain documented, and remaining gaps are explicitly handed to later milestones.

## Boundary Map

### S01 -> S02

Produces:
- Coverage/gap matrix schema and initial rows for all current archive families and capability axes.
- Ranked gap list with identifiers, risk level, evidence source, and likely remediation path.

Consumes:
- Existing code, tests, docs, generated fixture targets, package-consumer examples, and requirements R001-R009.

### S01 -> S03

Produces:
- Highest-risk fixture, round-trip, or capability gaps selected for M001 remediation.
- Matrix proof references that show which archive families and capability axes need tests or fixes.

Consumes:
- Existing generated fixtures, writer tests, reader tests, compatibility policy docs, and matrix findings.

### S01 and S03 -> S04

Produces:
- Error/validation/compatibility-warning gaps observed by the audit or encountered while fixing coverage gaps.
- Behavior-specific tests that expose inconsistent public result/error behavior.

Consumes:
- Public result/error model, validation API, reader/writer failure paths, and fixed coverage gaps.

### S02 and S03 and S04 -> S05

Produces:
- API audit conclusions, fixed gap proof, stabilized diagnostics, and updated documentation or policy checks.

Consumes:
- All slice outputs to run integrated build/test/package-consumer verification and finalize fixed/deferred matrix status.
