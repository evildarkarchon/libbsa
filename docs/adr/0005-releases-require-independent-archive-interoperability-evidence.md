# ADR-0005: Releases require independent archive interoperability evidence

- Status: Accepted
- Date: 2026-09-05

## Context

Generated fixtures and libbsa roundtrips provide useful structural and regression
coverage, but shared assumptions can let a reader and writer agree on an incorrect
format. Existing local retail tests mostly provide opening checks and limited
size-only extraction checks. Independent comparison is needed to substantiate
Archive Interoperability.

## Decision

Releases require a separate strict local Archive Interoperability run. Its target
coverage includes every supported archive profile and compression route, with
every entry in every supplied retail archive exercised in the thorough run.
Missing required corpus or oracle coverage prevents this run from reporting
success unless the exact oracle failure has separately received the narrow
approval described below. Such approval permits qualification with a reported
exception; it does not supply the missing independent evidence. Sampling alone
and optional maintainer evidence were rejected because
neither establishes the required breadth of release evidence.

Ordinary clean-checkout CI continues to work without retail archives. Retail
archives remain read-only, local-only evidence under the existing fixture policy.
The distinction is between ordinary CI success and satisfaction of the additional
release gate; ordinary CI success alone does not establish Archive Interoperability.

Use the supplied released BSArch executable as an external comparison tool; a
Delphi compiler is not required. Record the executable version and SHA-256 and
establish its capabilities directly. The executable reports v1.0 while the
read-only Pascal reference reports 0.9e, so source behavior cannot automatically
be attributed to that executable.

Archive Interoperability and Game Acceptance are separate claims. This suite
targets independent reading and writing interoperability; acceptance by BSArch
does not establish acceptance by a game's engine.

### Comparison contract

Require exact entry-path coverage and equality of decoded entry contents. For
DX10 textures, require matching texture metadata and every subresource's bytes;
allow only explicitly documented equivalent reconstructed DDS header
representations. Validate archive structure separately. Whole-archive byte
identity is not required because valid compression output and payload layouts
can differ.

### Reader and writer evidence

The thorough suite covers three directions:

1. Extract every supplied retail archive with libbsa and BSArch and compare all
   entries under the comparison contract.
2. Pack controlled, legal synthetic inputs with each tool and extract them with
   the other, comparing against the original inputs.
3. Repack every retail entry with libbsa and independently extract the resulting
   archives with BSArch. All derived retail data remains temporary and local.

An Accepted Oracle Failure can qualify only the specifically reviewed gap in
the first direction. libbsa must still read every entry, all other original
entries must compare independently, and BSArch must independently extract every
repacked entry. There is no whole-archive exclusion.

Fallout 4 BA2 v7/v8 remain reader-only profiles. Exercise their complete read
coverage, then repack their entries into supported Fallout 4 v1 archives and
verify equivalent content. Report the source/output profile change explicitly;
adding v7/v8 writers is outside this test-suite work.

Maintain an explicit required retail coverage matrix and reciprocal synthetic
oracle tests for every supported writable combination. Synthetic oracle evidence
may cover combinations absent from retail releases, but cannot substitute for
missing required retail coverage. Reports distinguish these evidence sources.

Enroll the current 101-archive corpus as the initial required retail baseline,
recording explicit archive identities and coverage. Additional supplied archives
are tested exhaustively as well. Report gaps in game or edition evidence
separately; a shared format version does not establish coverage of every game
edition. Baseline enrollment and full archive fingerprinting are implementation
work, not results already established by the design inventory.

When native oracle production of a required combination is unavailable or
unestablished, allow independently adapted synthetic oracle fixtures. Transform
only legal synthetic archives using small, documented test-only recipes, validate
them independently with BSArch, and label them as adapted fixtures rather than
native BSArch output. Do not use libbsa reader/writer helpers to construct these
expectations. Independent validation of libbsa-written output remains required.

Repack every retail entry once into its corresponding supported profile. Use
controlled synthetic cases for the wider writer-option matrix: compression
overrides, embedded names, payload sharing, texture chunking, and serial/parallel
writer execution, including relevant interactions. Exhaustive retail coverage
does not require a Cartesian product of every retail entry and every writer
option.

### Oracle authority

BSArch is evidence rather than an unconditional specification. Investigate
disagreements against format constraints and documented project decisions.
Unexplained differences block release. Narrowly documented intentional
differences receive explicit verdicts rather than blanket exclusions; existing
Payload Span Exclusivity requirements (ADR-0002) remain in force.

### Accepted oracle failures

The maintainer explicitly approved a narrow release-policy exception for the
released oracle's failure on `menus/s.txt` in `Fallout - Misc.bsa`, and requires
individual review of any future oracle failure. This amends the initial blanket
rule that every oracle extraction failure blocks qualification. It does not
amend the comparison contract or make libbsa output independent evidence about
an original entry the oracle could not extract.

Record each proposed failure in `tests/compat/oracle-failures.json`, with an
individual `approved`, `pending`, or `rejected` review and a separate
`gate_qualifies` decision. Only an approved record with `gate_qualifies: true`
can qualify. Pin the oracle executable and retail archive identities, operation,
allowed exact options, exit status, diagnostics, and affected entries. A fresh
failure must match that record exactly; an unreviewed failure, a changed failure,
or a timeout/resource failure still blocks qualification. A reviewed exception
does not require rebuilding BSArch with a different compiler or codec.

The first record, `BSARCH-001`, concerns one four-byte entry. Qualification still
requires independent comparison of the other 141 original entries, libbsa
reading all 142 entries, and independent BSArch validation of all 142 repacked
entries. The affected entry's pinned libbsa content fingerprint is a regression
guard only, explicitly labeled as such; it is not oracle evidence. Its original
independent content comparison remains unavailable. The executable's exact
codec behavior and the cause of its failure have not been verified; codec
explanations remain hypotheses.

Use the case verdict `accepted_oracle_failure` and the qualifying aggregate
verdict `passed_with_exceptions` so consumers can distinguish this outcome from
complete independent comparison. Reports identify the evidence gap and the
reviewed record. Never cache the failed extraction or reuse its partial catalog
to avoid reproducing the failure. Record the registry SHA-256 in release
evidence, and recheck it in `verify-release`: changing or revoking an approval
invalidates earlier qualification and requires fresh evidence.

### Sharing, threading, and byte identity

Exercise sharing and threading as compatibility features, with enabled and
disabled cases for BSArch and the corresponding supported libbsa options.
Include their interaction rather than imposing a blanket `-share:no` or
`-mt:no` policy. Equivalent paths, format-significant metadata, and decoded
contents are required even when sharing or scheduling changes archive bytes.
Comparisons do not require BSArch's payload-sharing layout to match libbsa's.

Restrictions intended to stabilize archive bytes apply only to tests that
explicitly require whole-archive byte identity. Such tests use a separately
documented fixed configuration, with BSArch sharing and threading disabled when
needed; disabling them alone does not prove deterministic output or establish
byte identity between different implementations. This does not relax exact
decoded-content comparisons in interoperability tests.

Record the exact options in capability evidence and cache identities. BSArch
v1.0 help documents sharing and threading as enabled by default; omission must
not be described as disabling either feature.

### Execution and retained evidence

Process retail archives serially, comparing streamed libbsa extraction against
oracle-extracted files and repacking entries in bounded batches where needed.
Serial archive scheduling does not prohibit threading within the active archive
operation; exercise that behavior within the configured resource limits.
Use a configurable scratch location and resource limits and clean completed
workspaces. Continue after isolated failures to produce a complete report.
Resource failures make coverage incomplete rather than silently reducing it.

Cache only independently derived oracle catalogs and content fingerprints, keyed
by the full retail archive SHA-256, oracle executable SHA-256, oracle options,
and comparison-schema version. Revalidate these identities and rerun libbsa
reading and writing for each release candidate. Cached oracle evidence does not
replace fresh BSArch validation of newly written libbsa archives. Detailed
reports stay local; publish only a summary without retail content. Retain failed
retail payloads only on explicit request.

Keep BSArch local and ignored, with `tests/fixtures/oracle` as the default location
and support for an external path. Commit the expected executable identity and
setup instructions. A missing or changed executable fails the strict run.
Unavailable required oracle coverage also fails, subject only to exact,
currently approved Accepted Oracle Failures. Upgrades require an explicit
identity-pin update and renewed capability checks; an exception pinned to the
previous binary does not transfer.

### Release procedure

Provide a strict local release-check command and document it as a required
release step. Evidence identifies the exact clean source revision, tested
binaries, dependency configuration, corpus, oracle, and oracle-failure registry.
There is no automated release publisher today; introducing one is outside this
work, so enforcement
initially consists of the command's strict verdict and the required maintainer
procedure.

Both MSVC Release package lanes, static and shared, must pass the full corpus
gate, including the current approval requirements for any reported Accepted
Oracle Failure. They may share verified successful oracle evidence under the
cache contract above; accepted failures must be freshly reproduced in each lane.
The compatibility harness must exercise the actual public library in each lane,
including the DLL in the shared lane, rather than substituting implementation
sources compiled directly into the test executable.
Debug and ASan coverage focuses on controlled interoperability and malformed
inputs rather than repeating the full retail repack workload across all presets.

## Consequences

Release preparation requires local data and additional processing beyond ordinary
CI. The required coverage matrix and comparison rules must be explicit so skipped
or unsupported comparisons cannot silently count as proof. The existing fixture
documentation's description of external evidence as optional smoke/compare only
must be updated when the new suite is implemented.

## Design evidence

The read-only inventory found 101 retail archives: 30 BSA and 71 BA2, about
163 GiB in total. Header/subtype groups were TES3 BSA (1), BSA v103 (6), v104 (6),
v105 (17), BA2 v1 GNRL (1), v2 GNRL (34), v2 DX10 (1), v3 DX10 with method 3
(15), v7 DX10 (9), v8 GNRL (10), and v8 DX10 (1). This is an inventory, not a
completed extraction or interoperability result.

The supplied BSArch v1.0 x64 executable has SHA-256
`4C34FE4173A2BD04BA52D5A6357348256EE424573785085FDAFAAB524CF7B0C2`.
Small disposable synthetic probes established native Starfield v2/zlib and
v3/LZ4 production and successful extraction of independently adapted v3/zlib
GNRL and DX10 archives. Native v3/zlib production remains unestablished. These
probes used omitted sharing flags and `-mt:no`; they establish neither the full
sharing/threading matrix nor the future suite's completion.

DDS probes preserved texture payload bytes while reconstructing header flags,
depth, and mip-count representations. This supports explicitly checked semantic
DDS comparison rather than blanket header-byte equality or blanket header
exclusion.
