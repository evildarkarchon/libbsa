# Requirements: libbsa v1.1 Hardening

**Defined:** 2026-05-12
**Core Value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

## v1.1 Requirements

Requirements for the v1.1 Hardening milestone. Each maps to exactly one roadmap phase.

### Host Path Reliability

- [x] **HOST-01**: Consumer can open supported archives from Windows host paths containing non-ASCII characters.
- [x] **HOST-02**: Consumer can validate supported archives from Windows host paths containing non-ASCII characters.
- [x] **HOST-03**: Maintainer can verify non-ASCII host-path open and validate coverage through committed regression tests for representative BSA and BA2 families.

### Verification Lanes

- [x] **VER-01**: Maintainer can configure and run a checked-in Windows MSVC Release preset for libbsa builds and automated tests.
- [x] **VER-02**: Maintainer can configure and run a checked-in Windows MSVC ASan preset for libbsa builds and automated tests.
- [x] **VER-03**: Maintainer can rely on `.planning`, `CMakePresets.json`, CI, and policy tests to describe the same supported verification lanes.

### Reader Dispatch Stability

- [x] **DISP-01**: Consumer can list, look up, check, extract, and bulk extract entries through one open-time reader backend selection with unchanged behavior across supported archive families.
- [x] **DISP-02**: Maintainer can add or adjust reader-backend behavior without duplicating archive-family branching across each public reader operation.

### Parser/Preparer Refactor

- [x] **REFA-01**: Maintainer can modify the targeted TES4 parser hotspot through smaller internal helpers with focused regression coverage.
- [x] **REFA-02**: Maintainer can modify the targeted BA2 DX10 preparer/staging hotspot through smaller internal helpers with focused regression coverage.

### Dedupe Hotspot Cleanup

- [x] **DEDU-01**: Consumer can write TES4-family archives with dedupe enabled using faster candidate narrowing while preserving exact stored-byte equality behavior.
- [ ] **DEDU-02**: Consumer can write BA2 GNRL archives with dedupe enabled using stronger staged identity or digest narrowing while preserving exact stored-byte equality fallback behavior.

### DX10 Temp-Staging Cleanup

- [ ] **DX10-01**: Consumer can write BA2 DX10 archives with temporary staging data cleaned up during normal write completion and ordinary failure unwinding.
- [ ] **DX10-02**: Maintainer can verify and document the remaining BA2 DX10 temporary-data lifecycle behavior, including any residual abnormal-termination risk.

## Future Requirements

Deferred beyond v1.1. These are tracked, but not committed to the current roadmap.

### Hardening Expansion

- **CORP-01**: Maintainer can run a documented local real-corpus compatibility workflow before shipping sensitive parser or writer changes.
- **FUZZ-01**: Maintainer can run a maintained fuzzing or hardening harness against archive readers and codecs.

### Public Behavior Expansion

- **LREC-01**: Consumer can request a lenient corrupt-archive recovery mode when strict open fails.
- **ABI-01**: Consumer can rely on a documented stable binary ABI policy for released libbsa packages.

## Out of Scope

Explicitly excluded from v1.1 to prevent scope creep.

| Feature | Reason |
|---------|--------|
| New archive family support | v1.1 is a hardening-only milestone for already-supported families. |
| New public API surfaces | The milestone goal is internal reliability work, not product-surface expansion. |
| Full writer redesign | Targeted seam cleanup is in scope; replacing the writer architecture is not. |
| Broad performance program | Only the named dedupe and BA2 DX10 staging hotspots are in scope. |
| Cross-platform hardening lanes | libbsa remains Windows-only. |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| HOST-01 | Phase 13 | Complete |
| HOST-02 | Phase 13 | Complete |
| HOST-03 | Phase 13 | Complete |
| VER-01 | Phase 14 | Complete |
| VER-02 | Phase 14 | Complete |
| VER-03 | Phase 14 | Complete |
| DISP-01 | Phase 15 | Complete |
| DISP-02 | Phase 15 | Complete |
| REFA-01 | Phase 16 | Complete |
| REFA-02 | Phase 16 | Complete |
| DEDU-01 | Phase 17 | Complete |
| DEDU-02 | Phase 17 | Pending |
| DX10-01 | Phase 17 | Pending |
| DX10-02 | Phase 17 | Pending |

**Coverage:**
- v1.1 requirements: 14 total
- Mapped to phases: 14
- Unmapped: 0

---
*Requirements defined: 2026-05-12*
*Last updated: 2026-05-14 after Phase 15 completion*
