## ADDED Requirements

### Requirement: Per-family writer pipeline decomposition
Each archive writer family — TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 — SHALL implement its pre-publish pipeline across at least three dedicated internal translation units beyond the public writer class translation unit: a source-preparation unit, a payload-assignment / deduplication / offset-planning unit (the "layout" unit), and an archive-serialization unit. Helpers owned by one stage MUST NOT be re-defined inside another stage's translation unit.

#### Scenario: TES4 BSA writer pipeline is layered
- **WHEN** the TES4 BSA writer family compiles
- **THEN** source preparation (per-entry payload reading, compression routing, embedded-name prefixing, prepared-folder construction) lives in a dedicated source-preparation translation unit
- **AND** payload deduplication, payload-equality comparison, and offset assignment live in a dedicated layout translation unit
- **AND** archive header, table, name, and payload byte serialization live in a dedicated serialization translation unit
- **AND** the public `tes4_bsa_writer` class translation unit references those stages only through their internal stage-entry-point headers

#### Scenario: BA2 GNRL writer pipeline is layered
- **WHEN** the BA2 GNRL writer family compiles
- **THEN** source preparation, payload deduplication and offset assignment, and archive serialization each live in distinct translation units owned by the BA2 GNRL writer family
- **AND** the BA2 GNRL writer translation unit does not define preparation, dedup, or serialization helpers locally

#### Scenario: BA2 DX10 writer pipeline is layered
- **WHEN** the BA2 DX10 writer family compiles
- **THEN** DDS reading, snapshot directory handling, chunk planning, and prepared-entry construction live in a dedicated source-preparation translation unit
- **AND** payload offset assignment lives in a dedicated layout translation unit
- **AND** archive header, chunk metadata, name, and payload serialization live in a dedicated serialization translation unit

#### Scenario: TES3 BSA writer pipeline is layered
- **WHEN** the TES3 BSA writer family compiles
- **THEN** source preparation, raw-offset assignment, and archive serialization each live in distinct translation units owned by the TES3 BSA writer family
- **AND** the TES3 BSA writer translation unit defers to those stages instead of defining their helpers locally

### Requirement: Writer translation units expose only stage entry points across stage boundaries
Each writer family's per-stage translation units SHALL expose only their stage entry-point function signatures and the prepared-data value types those stages exchange through internal headers under `src/formats/bsa/` or `src/formats/ba2/`. Stage-local helpers (validation utilities, checked arithmetic adapters, writer-internal namespaces) MUST remain hidden in anonymous namespaces inside the owning translation unit.

#### Scenario: Cross-stage call goes through a stage entry point
- **WHEN** a writer family's orchestrator invokes preparation, layout, or serialization
- **THEN** the call site refers to a stage entry-point function declared in an internal stage header
- **AND** the orchestrator does not call any anonymous-namespace helper that lives inside another stage's translation unit

#### Scenario: Stage helpers stay private to their translation unit
- **WHEN** a writer family's serialization translation unit needs a writer-local helper
- **THEN** that helper is defined inside the serialization translation unit's anonymous namespace
- **AND** the helper is not referenced from preparation, layout, or the writer class translation unit

### Requirement: Writer translation unit holds public class plus thin orchestrator only
Each `*_writer.cpp` file (`tes3_bsa_writer.cpp`, `tes4_bsa_writer.cpp`, `ba2_gnrl_writer.cpp`, `ba2_dx10_writer.cpp`) SHALL contain only the public writer class implementation (constructors, `add_file`, `add_bytes`, `write_to`, accessors) and the family's top-level orchestrator function (`write_*_archive`). The orchestrator SHALL delegate preparation, layout, and serialization to their stage translation units and SHALL delegate final host-path publication to `detail::publish_writer_output` from `writer-safe-publish` exactly once per call.

#### Scenario: Orchestrator composes stages and publishes once
- **WHEN** `write_tes4_bsa_archive`, `write_ba2_gnrl_archive`, `write_ba2_dx10_archive`, or `write_tes3_bsa_archive` is invoked
- **THEN** the orchestrator validates inputs and calls each stage entry point in order: preparation, layout, then serialization through the shared publish helper
- **AND** the orchestrator function calls `detail::publish_writer_output` exactly once per invocation
- **AND** the orchestrator does not contain compression routing, deduplication, table sizing, payload offset math, or byte serialization logic of its own

#### Scenario: Writer translation unit stays narrow
- **WHEN** the writer translation unit (`*_writer.cpp`) is reviewed
- **THEN** it contains only the public writer class methods and the orchestrator function
- **AND** it does not redefine source preparation, payload assignment, deduplication, offset planning, or archive serialization logic

### Requirement: Layered split preserves archive byte and behavior compatibility
The writer pipeline decomposition SHALL preserve archive byte layout, error semantics, and BSArchPro compatibility. Reorganizing pre-publish stages MUST NOT change emitted archive bytes, error codes, error messages (beyond preserved format-specific diagnostic prefixes), or `write_to` overwrite semantics for any existing TES3 BSA, TES4 BSA, BA2 GNRL, or BA2 DX10 fixture.

#### Scenario: Round-trip fixtures remain stable
- **WHEN** a writer family's existing round-trip and compatibility fixtures are exercised against the layered translation units
- **THEN** the produced archive bytes are identical to those produced before the split
- **AND** every previously-passing writer test still passes

#### Scenario: Format-specific diagnostics survive the split
- **WHEN** a writer family's preparation, layout, or serialization stage returns an `error`
- **THEN** the orchestrator returns that error to the caller unchanged in code and message
- **AND** the format-specific diagnostic prefix expected by `writer-safe-publish` (e.g., "TES4 BSA writer", "BA2 DX10 writer") remains attached to publish-stage errors

### Requirement: Stage entry points are independently testable
Each writer family's preparation, layout, and serialization stage entry point SHALL be callable from a focused unit test without invoking the writer class or the publish helper. Tests MAY exercise the orchestrator end-to-end, but the stage entry points MUST be reachable directly so future writer changes can land tests at the responsibility boundary they touch.

#### Scenario: Preparation stage is exercised in isolation
- **WHEN** a unit test constructs a minimal writer entry list and calls a writer family's preparation stage entry point directly
- **THEN** the test obtains the same prepared-entry / prepared-folder data the orchestrator would have constructed
- **AND** the test does not need to drive the public writer class, layout stage, serialization stage, or publish helper

#### Scenario: Layout stage is exercised with deduplicate-payload toggling (BA2 GNRL, TES4 BSA)
- **WHEN** a unit test calls a dedup-capable writer family's layout stage entry point against pre-built prepared entries with duplicate payloads
- **THEN** the test observes the family's documented deduplication behavior toggle independently of preparation and serialization
