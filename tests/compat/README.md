# Independent archive interoperability tests

This Windows-only suite compares libbsa with the locally supplied BSArch executable.
It implements the additional release requirement in
[ADR-0005](../../docs/adr/0005-releases-require-independent-archive-interoperability-evidence.md).
Ordinary CI remains runnable without game archives. A release requires qualifying
thorough reports for both MSVC Release library lanes, followed by `verify-release`.

The suite records Archive Interoperability evidence and any explicitly accepted
gaps in that evidence. It does not establish Game
Acceptance: BSArch successfully reading an archive is not evidence that a game
engine loads it. Neither a committed baseline nor this document asserts that the
full corpus has passed. The reports record what the tested binaries actually did.

The pinned BSArch v1.0 currently fails to extract `menus/s.txt` from the enrolled
`Fallout - Misc.bsa`, reporting `LibDeflate error: Bad data`. libbsa extracts the
four-byte entry through its existing reference-backed zlib tolerance. The
maintainer approved this exact failure as `BSARCH-001` in
[oracle-failures.json](oracle-failures.json). It can qualify as an **Accepted
Oracle Failure** when freshly reproduced and all remaining required checks pass.
The independent original-content comparison for that entry remains unavailable;
approval does not turn it into a successful comparison. The cause of the
released executable's behavior is unverified, including any codec explanation.
No full 101-archive release qualification is claimed by this document or by the
development profile checks.

## Evidence and required coverage

The thorough run has three independent comparisons:

1. Read every supplied retail archive with libbsa and BSArch and compare every entry.
2. Pack author-owned synthetic sources with each tool and read them with the other,
   comparing decoded contents against the original sources.
3. Repack every retail entry with libbsa, in bounded batches, then inspect the
   generated structures independently and extract them with BSArch.

The bridge links the public `libbsa::libbsa` target, including its DLL in the shared
lane. Python catalog, SHA-256, DDS, and structural checks do not call libbsa helpers.
Controlled synthetic archives are useful here because another implementation
validates them; they do not rely on a libbsa writer/reader roundtrip alone.

The controlled matrix contains **71 cases**: 67 writer-option cases across 12
writable targets and four adapted reader-only BA2 v7/v8 cases. TES3 has one raw
case. Every other writable target has six combinations covering the applicable
compression policies, per-entry overrides, embedded names, payload sharing,
texture chunk sizes, and serial/parallel execution. Cases exercise sharing and
threading enabled and disabled, including interactions. `synthetic.py` is the
executable matrix and records the exact BSArch arguments.

`retail-baseline.json` enrolls **101 required archives**, using relative paths,
full-file SHA-256, sizes, and independently read header facts:

| Retail profile | Archives |
|---|---:|
| TES3 BSA | 1 |
| BSA v103 / v104 / v105 | 6 / 6 / 17 |
| BA2 v1 GNRL | 1 |
| BA2 v2 GNRL / DX10 | 34 / 1 |
| BA2 v3 DX10, method 3 | 15 |
| BA2 v7 DX10 | 9 |
| BA2 v8 GNRL / DX10 | 10 / 1 |

Additional supplied archives are discovered recursively and tested completely.
A missing or changed required archive fails the gate; finding some other archive
with the same format version does not replace it. A baseline-only thorough run
therefore expects 172 cases. These counts describe required work, not passed work.
Shared format versions do not establish coverage of every game edition.

BA2 v7/v8 remain reader-only. Their retail entries are repacked into supported
Fallout 4 v1 outputs, with both source and output profiles recorded. Synthetic
v7/v8 archives and v3/zlib archives use explicitly labeled, independently adapted
BSArch output where native oracle production is unavailable or unestablished.
These recipes transform only small, legal synthetic archives and require BSArch
to read the transformed result. They are not attributed to native BSArch packing.

## What equality means

Every comparison requires the complete canonical Archive Entry Catalog, with no
missing, extra, or duplicate canonical paths. BSA and GNRL entries require exact
decoded lengths and SHA-256 equality, including files whose names end in `.dds`.
Those containers store ordinary file payloads and have no DDS reconstruction
reason to change their bytes.

DX10 extraction reconstructs DDS headers. Its comparison validates independently
parsed dimensions, format identity, mip geometry, array/cubemap state, and ordered
surface data. It permits documented equivalent legacy/DXT10 header forms and
redundant header representations only when the semantic checks agree. It hashes
all surface bytes after the validated header boundary and checks the expected
payload length. Unsupported DDS representations fail explicitly; stripping every
header without checking its meaning would conceal format errors.

Writer structure is inspected separately from extraction. Whole-archive byte
identity is not required: codec output, scheduling, and valid Payload Placement
may differ. A BSArch disagreement must be investigated against the reference and
project decisions. Only an individually approved Accepted Oracle Failure can
qualify with its evidence gap recorded. No disagreement creates an automatic
exclusion or silently overrides Payload Span Exclusivity.

## Local setup

Use the existing MSVC/vcpkg presets with tests enabled. Place read-only retail
archives under `tests/fixtures/local`, or set `LIBBSA_GAME_FIXTURES`. Supply BSArch
at `tests/fixtures/oracle/BSArch.exe`, or set `LIBBSA_BSARCH` / pass `--oracle`.
The executable is ignored by `.gitignore`; do not commit it or retail archives.
`TES5Edit/` remains read-only reference material and is never an output directory.

The committed [oracle identity](oracle.json) pins `BSArch v1.0 x64`:

```text
4c34fe4173a2bd04ba52d5a6357348256ee424573785085fdafaab524cf7b0c2
```

No Delphi compiler is required. The executable's version differs from the Pascal
reference's version, so source code alone does not prove this binary's behavior.
A missing or changed executable fails strict execution. An oracle upgrade needs
an explicit reviewed pin update and renewed capability/interoperability results.

The enrolled baseline contains identities, not retail payload bytes. To propose
a deliberate baseline replacement, write a separate candidate and review the
coverage and identity changes before replacing the required baseline:

```powershell
python tests/compat/runner.py enroll --corpus tests/fixtures/local --output build/compat-baseline-candidate.json
```

Enrollment hashes every archive and can take substantial I/O. It records inputs;
it does not compare their entries or produce a passing release verdict. Never
reenroll merely to hide a missing or modified required archive.

## Required release procedure

Run from a clean checkout at the intended release commit. Complete the ordinary
build, CTest, and package checks as usual, then run the additional strict target
in each Release lane serially:

```powershell
cmake --preset windows-msvc-release-static
cmake --build --preset windows-msvc-release-static --target libbsa_compatibility_check
cmake --preset windows-msvc-release-shared
cmake --build --preset windows-msvc-release-shared --target libbsa_compatibility_check
python tests/compat/runner.py verify-release build/windows-msvc-release-static/tests/compat/thorough-Release.json build/windows-msvc-release-shared/tests/compat/thorough-Release.json
```

The target builds the public-library bridge and runs `runner.py thorough`. Strict
execution immediately refuses a dirty checkout, then reconfigures the selected
CMake build directory and builds its Release bridge and dependencies before
collecting evidence. Supplied binary paths must match that directory's configured
targets and source tree; a direct invocation cannot qualify arbitrary stale
binaries. Missing baseline coverage, unapproved missing oracle coverage, resource
failures, or a comparison failure produces a nonzero verdict. An exact approved
oracle failure can qualify only under the review requirements below.
`verify-release` requires one qualifying static report and one qualifying shared
report for the same clean source, oracle, baseline, corpus, and oracle-failure
registry. It rechecks the current source, tested binaries and runtime DLLs,
CMake/dependency identities, oracle, baseline, corpus, and registry identities.
Required case identities are reconstructed from the controlled matrix and complete
corpus inventory; a report cannot reduce its own required count or substitute a
smaller baseline. The release baseline must match the committed required baseline.
Changing these inputs requires fresh evidence. This is the required maintainer
release procedure; there is no automated release publisher in this repository.

For custom limits, invoke the same runner directly. This example uses the
multi-configuration MSVC static Release output layout; use `libbsa.dll` and the
shared build paths for the shared lane:

```powershell
python tests/compat/runner.py thorough `
  --bridge build/windows-msvc-release-static/tests/compat/Release/libbsa_compat_bridge.exe `
  --library build/windows-msvc-release-static/Release/libbsa.lib `
  --build-dir build/windows-msvc-release-static `
  --report build/windows-msvc-release-static/tests/compat/thorough-Release.json `
  --scratch D:/libbsa-compat-work --cache D:/libbsa-compat-cache `
  --batch-mib 256 --scratch-gib 64 --memory-gib 8 --timeout 3600 --workers 4
```

## Development and resource controls

`compatibility_harness_contracts` tests independent comparison and supervision
behavior without retail inputs. `independent_oracle_synthetic` runs all controlled
cases and is skipped by default when BSArch is absent; its CTest label is
`requires-oracle`. The legacy `requires-game-fixture` tests remain available as
focused corpus/regression checks. Neither optional CTest path replaces the gate.

```powershell
cmake --build --preset windows-msvc-debug-static --target libbsa_compat_bridge
ctest --preset windows-msvc-debug-static -R "compatibility_harness_contracts|independent_oracle_synthetic"
```

For a dirty checkout investigation, build the desired binaries, then use the
direct command above with `--diagnostic` and a separate report path. Diagnostics
skip strict source qualification and its forced configure/build step. A diagnostic
run can report successful comparisons but always records `release_eligible: false`.
The strict command without that flag refuses a dirty checkout before archive
comparisons. `synthetic --case SUBSTRING`
can narrow a development investigation; `thorough` refuses case filtering and
cannot be made optional. Debug and ASan are useful for controlled interoperability
and malformed-input work; they do not replace the two full Release runs.

| Option | Default | Meaning |
|---|---|---|
| `--scratch` | `LIBBSA_COMPAT_SCRATCH`, otherwise `build/compat-work` | Parent for uniquely owned operation directories. |
| `--cache` | `build/compat-cache` | Retained independent oracle catalog/fingerprint cache. |
| `--batch-mib` | 256 | Decoded source budget per repack batch; an oversized entry forms a streamed singleton batch. |
| `--scratch-gib` | 64 | Private workspace budget; existing files reduce the remaining allowance before each native operation. |
| `--memory-gib` | 8 | Aggregate committed memory cap for each native process tree. |
| `--timeout` | 3600 seconds | Wall-time cap for each native operation. |
| `--workers` | 4 | Requested libbsa retail writer workers; controlled cases have explicit matrix settings. |

Archive jobs run serially; the active native operation can use multiple workers.
Windows Job Objects own processes before their initial thread starts, enforce
memory limits, and terminate descendants on timeout, resource failure, or exit.
Additional native scratch consumption is conservatively measured from volume free
space, so other writers on that volume can also consume the allowance. Batch size is not a cap
on total output: oracle extraction can expand an entire archive and repack outputs
remain until checked. Resource exhaustion reports incomplete coverage rather
than selecting smaller samples.

Only successful independent oracle catalogs/fingerprints are cached, keyed by the full
archive digest, oracle digest, exact options, and comparison identity/schema.
libbsa reading and writing rerun for each candidate, and BSArch freshly validates
new libbsa output. Cache records contain metadata and hashes, not extracted
payload files. Failed extraction results, including any partial catalog from an
Accepted Oracle Failure, are never reused from this cache. Choose a local cache
location with sufficient capacity; cache
location and scratch limits describe different retained data.

Completed and failed operation workspaces are cleaned up. Detailed JSON reports
and local process logs remain alongside the chosen report path; they can contain
retail entry names, identifiers, and DDS header evidence and must stay local.
Publish only the companion `.summary.json`, which omits retail names, paths,
headers, and hashes. Inspect every failed case, fix or explicitly resolve the
underlying disagreement, and rerun; do not count a known production mismatch as
a passing or excluded comparison.

## Reviewing an oracle failure

Each failure requires its own record and review in
[oracle-failures.json](oracle-failures.json). Review states are `approved`,
`pending`, and `rejected`; only `approved` with `gate_qualifies: true` permits
qualification. A record identifies the exact oracle executable and retail
archive by SHA-256, the operation and allowed exact option lists, exit code,
diagnostics, and affected entries. Pending or rejected records do not qualify,
and a changed or additional failure requires a new review. Timeouts and resource
failures never qualify through this mechanism. Rebuilding the external binary
is not a prerequisite for reviewing its known failure.

For `BSARCH-001`, the runner must freshly reproduce the pinned failure on the
four-byte `menus/s.txt` entry. It must independently compare the other 141
original entries, read all 142 entries with libbsa, and have BSArch successfully
extract and verify all 142 entries after repacking. The pinned fingerprint of
libbsa's affected entry is explicitly a regression-only guard. It must never be
described as independently established original content. These requirements
retain the full archive in the run and expose the single unavailable comparison.

A qualifying case reports `accepted_oracle_failure`; a qualifying complete run
containing such a case reports `passed_with_exceptions`. Every other required
check must pass. The failed extraction and its partial catalog are not cached:
the exact failure must be observed anew for each candidate and each Release
lane. Reports bind the registry SHA-256, and `verify-release` rechecks the
current registry. Changing a record or revoking its approval invalidates prior
qualification; obtain fresh evidence under the updated policy.
