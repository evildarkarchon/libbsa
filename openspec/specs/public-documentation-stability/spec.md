## Purpose

Ensure libbsa's consumer-facing public documentation uses durable behavior names and keeps internal planning identifiers out of published API, fixture, compatibility, and thread-safety guidance.

## Requirements

### Requirement: Public documentation uses durable behavior wording
Consumer-facing libbsa documentation SHALL describe archive support, API contracts, thread-safety, compatibility evidence, fixture policy, and benchmark policy using stable behavior names rather than internal planning identifiers.

#### Scenario: Public documentation is reviewed for planning identifiers
- **WHEN** public headers, published docs, and fixture guidance are reviewed
- **THEN** those surfaces do not contain phase labels, milestone labels, or decision IDs used as planning shorthand
- **AND** the same surfaces describe the relevant behavior using stable libbsa concepts

#### Scenario: Internal planning history remains outside public docs
- **WHEN** OpenSpec, GSD, archived planning, or roadmap artifacts retain historical planning identifiers
- **THEN** that planning history remains allowed outside consumer-facing public documentation
- **AND** those identifiers are not copied into public headers, published docs, or fixture guidance

### Requirement: Documentation cleanup preserves behavioral guidance
Rewriting public comments and docs SHALL preserve the compatibility rationale and caller obligations previously attached to planning-era identifiers.

#### Scenario: Thread-safety comments are rewritten
- **WHEN** public thread-safety comments remove decision-ID references
- **THEN** they still document caller-owned synchronization, distinct sink requirements, concurrent extraction behavior, writer mutation limits, and canonical thread-safety guidance

#### Scenario: Scope and policy docs are rewritten
- **WHEN** public docs remove phase or milestone references from archive scope, fixture, compatibility, or benchmark wording
- **THEN** they still document the supported behavior, evidence expectations, generated synthetic fixture policy, benchmark data policy, and relevant archive-family constraints

### Requirement: Public documentation stability is regression-tested
The implementation SHALL include focused validation that prevents planning-era identifiers from returning to consumer-facing documentation.

#### Scenario: Forbidden public documentation tokens are introduced
- **WHEN** a public header, published doc, or fixture guidance file includes a phase label, milestone label, or decision ID as public-facing wording
- **THEN** the documentation policy tests fail with the affected file and token family identifiable from the failure context

#### Scenario: Stable replacement wording is asserted
- **WHEN** documentation policy tests validate rewritten thread-safety and public API guidance
- **THEN** they assert stable terms such as distinct sinks, caller-owned synchronization, write-call execution controls, generated fixture policy, and compatibility evidence instead of planning IDs
