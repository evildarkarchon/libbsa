## ADDED Requirements

### Requirement: Observable policy behavior uses executable tests
Policy-sensitive tests SHALL exercise observable runtime, build, or validation behavior whenever the protected policy can be proven without reading implementation source text.

#### Scenario: Behavior policy has an executable oracle
- **WHEN** a policy covers memory bounds, writer publish behavior, validation warning coverage, package-consumer integration, or malformed parser input handling
- **THEN** the regression coverage exercises the relevant libbsa API, internal test helper, generated fixture, CTest target, or package-consumer build path
- **AND** the test asserts returned errors, archive bytes, diagnostics, generated metadata, build success, or other observable outcomes instead of only searching implementation text for private tokens

#### Scenario: Behavior coverage replaces a token check
- **WHEN** an implementation source-token assertion is replaced
- **THEN** the replacement coverage proves the same policy through an executable or structured oracle before the brittle assertion is removed
- **AND** the remaining test name or failure message identifies the policy being protected

### Requirement: Static policy checks remain narrow
Source-text or documentation-text policy checks SHALL remain only for hard boundaries whose protected property is inherently textual or configuration-based.

#### Scenario: Hard source boundary remains textual
- **WHEN** a policy protects public-header dependency isolation, `TES5Edit/` reference-only isolation, Doxygen input/exclusion boundaries, Windows-only preset coverage, or benchmark target gating
- **THEN** a source-text, documentation-text, or configuration-text check MAY remain
- **AND** the check names the boundary it protects rather than standing in for runtime archive behavior

#### Scenario: Documentation coverage is structural
- **WHEN** a policy verifies public documentation coverage
- **THEN** the test prefers stable structure such as documented public type names, public warning-code entries, headings, anchors, or required evidence fields
- **AND** exact prose tokens are used only when the wording itself is the policy

### Requirement: Converted policy tests preserve validation scope
Converting a brittle source-text policy test SHALL preserve the original validation scope and SHALL NOT weaken the underlying policy by deleting coverage without an equivalent replacement.

#### Scenario: Policy inventory drives conversion
- **WHEN** a named policy test file is updated
- **THEN** each removed or narrowed source-token assertion is accounted for as behavior-backed coverage, a retained static boundary, or obsolete coverage made redundant by a stronger test

#### Scenario: Fixture and parser policies use malformed inputs
- **WHEN** a policy protects parser rejection, allocation bounds, sparse payload limits, or malformed archive handling
- **THEN** the test uses generated malformed fixtures, mutated fixture bytes, bounded validation options, or stable malformed manifests
- **AND** failures flow through `result` errors with the expected `libbsa::error_code`
