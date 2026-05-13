# Phase 13: host-path-correctness-boundary - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-13
**Phase:** 13-host-path-correctness-boundary
**Areas discussed:** Boundary reuse, Stored path form, Validation preflight, Unicode coverage, TES4 coverage cut, Read-proof strength, Unicode token choice, TES3 migration reach

---

## Boundary reuse

| Question | Options considered | Selected |
|--------|-------------|----------|
| How should the shared host-path boundary be introduced? | Generalize existing helper / Add read-only helper / Thin wrapper over writer helper | Generalize existing helper |
| If generalized, how should error reporting stay shaped? | Context-driven diagnostics / Generic helper messages / Hybrid messages | Context-driven diagnostics |
| How far should the generalized boundary reach inside Phase 13? | All read-side host opens / Only parser and reader opens / Only representative paths | All read-side host opens |
| Should the generalized boundary also get neutral naming now? | Rename to neutral helper / Keep writer-centric names / Partial rename | Rename to neutral helper |
| What should the generalized helper expose to read-side callers? | Layered primitives + helpers / Open primitive only / High-level scenario helpers | Layered primitives + helpers |
| What should the generalized boundary hand back to callers once the host path is resolved? | `std::ifstream` streams / Filesystem path objects only / Custom file wrapper | `std::ifstream` streams |
| When the helper is renamed, how should writer call sites be handled? | Migrate writer call sites too / Keep writer shim aliases / Split the rename later | Migrate writer call sites too |

**User's choice:** One neutrally named shared host-file boundary in `src/detail/`, reused by both read and writer call sites, with context-driven diagnostics, layered helpers, and `std::ifstream`-based call sites.
**Notes:** The user explicitly preferred a real convergence point over parallel helper stacks or transitional aliases.

---

## Stored path form

| Question | Options considered | Selected |
|--------|-------------|----------|
| After `archive_reader::open` succeeds, what host-path form should reader state keep? | Store both UTF-8 + resolved path / Store resolved path only / Keep UTF-8 string only | Store both UTF-8 + resolved path |
| When should the UTF-8 host path be resolved into the internal filesystem path? | At API boundary / Inside shared helper lazily / Per operation | At API boundary |
| How should internal call sites receive that resolved host-path state? | Small internal path struct / Separate string + path params / Resolved path only downstream | Small internal path struct |
| For follow-on extraction reads, where should the filesystem path come from? | Only from stored state / Re-resolve from stored UTF-8 text / Caller text can still flow through | Only from stored state |
| Where should that internal host-path struct live? | Shared detail module / `archive_reader` private state only / Per-subsystem copies | Shared detail module |
| Who should own stream opening once the shared path struct exists? | Each caller opens through helper / Facade opens, callee consumes / Mixed ownership | Each caller opens through helper |
| What role should the original UTF-8 text play after the resolved path is stored? | Diagnostics only / Secondary fallback / Drop original text after open | Diagnostics only |
| Should open, validation, and writer call sites all converge on the same internal path type? | One shared internal path type / Reader/validation type only / Separate types per flow | One shared internal path type |

**User's choice:** Resolve once at the API boundary, keep both original UTF-8 text and resolved `std::filesystem::path`, and pass a single shared internal path value through the generalized boundary.
**Notes:** The user wanted the resolved path to become the only real I/O source after open, with the original text preserved strictly for diagnostics.

---

## Validation preflight

| Question | Options considered | Selected |
|--------|-------------|----------|
| What should happen to `validate_archive`'s separate readability preflight? | Remove it and trust open / Keep it for fast failure / Keep a non-opening preflight | Remove it and trust open |
| If the preflight goes away, how should the public validation contract behave? | Keep current result/report split / Everything becomes a report / Everything becomes result-level | Keep current result/report split |
| How should validation surface unreadable host-path failures once it relies on the shared open path? | Use shared helper/open diagnostics / Wrap with validation-specific wording / Keep only error codes stable | Use shared helper/open diagnostics |
| For `validate_entry_extractability`, what should the host-path behavior be? | Reuse opened reader state only / Re-check before extractability / Separate validation-only extraction path | Reuse opened reader state only |
| Should `validate_archive` still reject an empty host path before it ever touches the shared boundary? | Yes, keep explicit empty-path guard / Let shared boundary handle it / Only `archive_reader::open` guards it | Yes, keep explicit empty-path guard |
| How should unreadable or non-openable host paths map at the public API level after the cleanup? | Keep one stable `io_error` code / Differentiate with new public codes / Use generic validation failure | Keep one stable `io_error` code |
| If setup fails before any archive bytes are parsed, what should validation return? | Direct error, no report / Invalid empty report / Best-effort partial report | Direct error, no report |
| How should Phase 13 tests prove this validation cleanup? | Black-box public behavior only / White-box duplicate-open proof / Mixed black-box + hook | Black-box public behavior only |

**User's choice:** Remove the duplicate readability open, preserve the existing public validation contract, and verify the cleanup only through black-box behavior.
**Notes:** The user explicitly did not want test coverage to lock the internal call graph or count opens.

---

## Unicode coverage

| Question | Options considered | Selected |
|--------|-------------|----------|
| What path shape should Phase 13 regression tests prove? | Non-ASCII directory and filename / Non-ASCII directory only / Non-ASCII filename only | Non-ASCII directory and filename |
| How should those representative host-path tests be organized? | Dedicated cross-family suite / Spread into existing suites / Hybrid | Dedicated cross-family suite |
| How broad should the Unicode path token itself be? | One stable curated token / Small script matrix / Random Unicode names | One stable curated token |
| What should be copied into the non-ASCII path during the regression tests? | Archive file only / Archive + manifest bundle / Entire fixture subtree | Archive file only |

**User's choice:** Use one dedicated cross-family suite that copies only the archive-under-test into a deterministic non-ASCII directory and non-ASCII filename.
**Notes:** The user wanted the suite focused on host-archive opening rather than turning it into a broad fixture-relocation exercise.

---

## TES4 coverage cut

| Question | Options considered | Selected |
|--------|-------------|----------|
| For the TES4 side of Phase 13, how broad should the representative proof be? | All three existing TES4 variants / One TES4 fixture only / Two TES4 fixtures | All three existing TES4 variants |

**User's choice:** Include `tes4_v103.bsa`, `tes4_v104.bsa`, and `tes4_v105.bsa` in the dedicated non-ASCII suite.
**Notes:** The user preferred cheap extra semantic coverage over a strict minimum interpretation of the SPEC's singular TES4 wording.

---

## Read-proof strength

| Question | Options considered | Selected |
|--------|-------------|----------|
| For the post-open extraction proof, how much should each representative archive extract in the new suite? | One canonical entry per archive / All manifest entries / One entry for BSA, all for BA2 | One canonical entry per archive |

**User's choice:** Prove one canonical manifest-backed extraction target per archive.
**Notes:** The user wanted the dedicated Phase 13 suite to stay focused on the host-path boundary and not duplicate the full extraction matrices already covered elsewhere.

---

## Unicode token choice

| Question | Options considered | Selected |
|--------|-------------|----------|
| What should the stable curated non-ASCII naming token look like? | Accented Latin + Japanese / Accented Latin only / Cyrillic + Japanese mix | Accented Latin + Japanese |
| Pick the exact curated token for the non-ASCII temp path and filename. | `libbsa-Ångström-日本語` / `libbsa-Übergröße-日本語` / `libbsa-Café-日本語` | `libbsa-Ångström-日本語` |

**User's choice:** Standardize on the exact token ``libbsa-Ångström-日本語``.
**Notes:** The user favored a readable, deterministic, BMP-only token that is obviously non-ASCII during debugging and review.

---

## TES3 migration reach

| Question | Options considered | Selected |
|--------|-------------|----------|
| Should the shared host-file boundary migrate TES3 read-side narrow opens too, even though Phase 13 won't add TES3 non-ASCII regression coverage? | Yes, migrate TES3 too / No, leave TES3 for later / Migrate only TES3 open path | Yes, migrate TES3 too |

**User's choice:** Migrate TES3 read-side narrow opens into the shared boundary too.
**Notes:** The user wanted the codebase to land on one real read-side boundary, even though the locked non-ASCII regression matrix remains focused on the SPEC's representative families.

---

## the agent's Discretion

None.

## Deferred Ideas

None.
