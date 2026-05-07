# Domain Pitfalls

**Domain:** Reusable C++20 Bethesda BSA/BA2 archive reader/writer library  
**Researched:** 2026-05-07  
**Confidence:** HIGH for project constraints and compression API pitfalls; MEDIUM for undocumented Bethesda compatibility quirks that require fixture confirmation.

## Critical Pitfalls

Mistakes that can corrupt archives, produce game-incompatible output, or force a rewrite.

### Pitfall 1: Treating BSA/BA2 as one archive format with minor version switches

**What goes wrong:**  
The parser grows one large record model with conditional fields. TES3 offsets, TES4-family folder/file blocks, SSE LZ4 payloads, BA2 GNRL records, BA2 DX10 texture chunks, and Starfield v2/v3 headers get mixed together. Fixing one version later breaks another.

**Why it happens:**  
All formats use `.bsa` or `.ba2`, and some fields look superficially similar: magic, version, counts, offsets, packed size, unpacked size. The temptation is to build a single generic `ArchiveEntry` too early.

**Consequences:**
- Version-specific offset bases are misapplied.
- Starfield fields such as `Unknown1`, `Unknown2`, and `CompressionMethod` are dropped or guessed.
- BA2 DDS chunks are treated like normal files instead of texture-streaming mip chunks.
- Write support becomes a minefield because record serialization cannot preserve format-specific invariants.

**Prevention:**
- Implement separate internal format modules: `tes3`, `tes4_bsa`, `ba2_gnrl`, `ba2_dx10`, plus shared narrow utilities for endian reads, paths, hashes, and compression.
- Make auto-detection return a concrete format/version enum before parsing records.
- Preserve unknown-but-observed fields in metadata and writer options rather than discarding them.
- Require a new fixture and serialization test whenever adding a format/version branch.

**Warning signs:**
- Code paths switch on version in many unrelated methods.
- Tests pass for one game but require TODOs/skips for another variant.
- Internal structs contain fields marked “only for BA2”, “only for TES3”, etc.
- Writer APIs cannot express Starfield v3 compression method separately from file extension.

**Phase to address:**  
Milestone 1 must establish the format-registry and module boundary. Milestones 2-4 should add formats only through that boundary. Milestones 5-8 should reuse the same concrete format modules for writers.

**Specific compatibility risk:**  
Archives may parse enough to list files but extract from wrong offsets or write headers that official tools reject.

---

### Pitfall 2: Confusing LZ4 frame data with raw LZ4 block data

**What goes wrong:**  
SSE BSA compressed payloads and Starfield BA2 v3 payloads are both labelled “LZ4” in conversation, but they are not interchangeable. Using `LZ4F_*` frame APIs on raw Starfield blocks, or raw `LZ4_*` APIs on SSE frames, causes failures or silent corruption.

**Why it happens:**  
LZ4 exposes both a frame API and a raw block API. The frame format is self-describing and has magic bytes; raw blocks require the archive record to supply exact compressed and decompressed sizes.

**Consequences:**
- SSE extraction fails when treated as raw blocks.
- Starfield BA2 v3 files fail or decompress to wrong bytes when treated as frames.
- Writer output may be accepted by libbsa round-trip tests but rejected by game engines because the wrong LZ4 container was written.

**Prevention:**
- Create two separate wrappers: `lz4_frame_codec` for SSE BSA and `lz4_block_codec` for Starfield BA2 v3 `CompressionMethod == 3`.
- Route compression by concrete archive type/version/record metadata, never by extension or a generic “lz4” boolean.
- Use `LZ4_decompress_safe` for raw blocks and verify the returned byte count exactly equals the archive record's unpacked size.
- Use `LZ4F_isError` / `LZ4F_getErrorName` around frame operations and test with actual SSE BSA fixtures.

**Warning signs:**
- A function named only `decompress_lz4` handles every LZ4 payload.
- Tests check only “decompression succeeded”, not exact output size and byte equality.
- Starfield v3 `CompressionMethod` is ignored or inferred from archive type.
- Code probes for LZ4 frame magic rather than trusting the parsed archive version and method.

**Phase to address:**  
Milestone 1 for SSE frame support; Milestone 3 for Starfield raw block support; Milestone 6 for BA2 GNRL writing; Milestone 7 for BA2 DDS chunk writing.

**Specific compatibility risk:**  
Silent payload corruption in Starfield BA2 v3 and invalid SSE BSA output.

---

### Pitfall 3: Deflate wrapper ambiguity and non-exact decompression validation

**What goes wrong:**  
The code assumes all “deflate” data means zlib-wrapped streams, or all data means raw DEFLATE, and does not validate the exact decompressed size. Bad inputs can be accepted with trailing bytes, short output, or accidental partial streams.

**Why it happens:**  
Libraries expose raw DEFLATE, zlib, and gzip APIs with similar names. libdeflate explicitly documents separate `deflate`, `zlib`, and `gzip` functions and distinct result codes for bad data, short output, and insufficient space.

**Consequences:**
- FO3/Skyrim/BA2 payloads may fail against fixtures depending on wrapper expectation.
- Malformed archives can pass partial decompression and then corrupt extracted files.
- Golden compressed-byte tests become brittle because compression libraries do not promise stable compressed bytes across versions.

**Prevention:**
- Build a deflate wrapper that records the chosen stream flavor per archive family after compatibility verification.
- Always allocate/decompress to the archive record's expected unpacked size and treat `SHORT_OUTPUT`, `INSUFFICIENT_SPACE`, and byte-count mismatch as hard format errors.
- For write tests, compare extracted bytes and game/tool compatibility, not exact compressed byte streams.
- Add fixture cases for uncompressed files (`PackedSize == 0`) and compressed files that expand exactly to the recorded size.

**Warning signs:**
- Tests compare compressed payload bytes generated by libdeflate to a golden compressed blob.
- Decompression APIs are called with an oversized output buffer and actual output size is ignored.
- The parser treats any nonzero decompressor return as a generic I/O error without differentiating corrupt data from size mismatch.

**Phase to address:**  
Milestone 1 for TES4-family deflate; Milestone 3 for BA2 deflate; Milestones 5-6 for writer behavior; Milestone 10 for malformed/truncated archive hardening.

**Specific compatibility risk:**  
Byte-identical extraction will fail, malformed archives may read as valid, and writer output may be needlessly unstable across dependency updates.

---

### Pitfall 4: Reconstructing BA2 DDS files as generic concatenated payloads

**What goes wrong:**  
BA2 DX10 texture archives store DDS texture data without a normal DDS file header and split texture data into mip chunks. Extractors that simply concatenate chunk bytes or write a guessed header produce files that may open in one viewer but fail in game/tool pipelines.

**Why it happens:**  
BA2 GNRL extraction is straightforward, so developers expect BA2 DDS extraction to be similarly “read bytes, maybe decompress, write bytes.” Documentation and community tools emphasize that texture archives are optimized for streaming and require DDS header reconstruction.

**Consequences:**
- Reconstructed DDS files have wrong DXGI format, mip count, cubemap/array metadata, or dimensions.
- Mip ranges are ordered incorrectly or chunk boundaries are lost during write.
- Fallout 4/Starfield texture BA2s load with visual corruption, missing mips, or runtime streaming problems.

**Prevention:**
- Isolate DDS logic behind internal `dds_analyzer` / `texture_layout` types backed by DirectXTex metadata.
- For read support, reconstruct headers from archive metadata and validate extracted DDS through DirectXTex `LoadFromDDSMemory` or metadata loading.
- For write support, use DirectXTex to parse source DDS metadata, then derive chunk records from mip dimensions and target game/version policy.
- Maintain separate fixtures for ordinary 2D textures, normal maps, cubemaps, multiple mip counts, and small textures at or below chunk thresholds.

**Warning signs:**
- BA2 DDS tests only assert that output begins with `DDS `.
- DDS writer has hard-coded header bytes or a hand-maintained DXGI table exposed in public headers.
- Chunk `StartMip`/`EndMip` values are ignored during extraction.
- Texture code is mixed into generic archive-file extraction instead of a BA2 DX10-specific path.

**Phase to address:**  
Milestone 4 for DDS read/reconstruction; Milestone 7 for DDS write/chunking; Milestone 10 for edge-case texture validation.

**Specific compatibility risk:**  
Textures appear extractable but are not valid DDS files or are valid files with wrong mip/cubemap semantics, causing game-visible corruption.

---

### Pitfall 5: Treating archive paths as host filesystem paths or Unicode strings

**What goes wrong:**  
Archive member names are normalized through `std::filesystem::path`, wide strings, locale conversion, or platform separators. Paths change case, separators, encoding, or byte values, so lookups and hashes no longer match game behavior.

**Why it happens:**  
The public API wants friendly paths, but Bethesda archive paths are virtual paths. Existing Rust archive libraries explicitly warn that Creation Engine paths are effectively byte strings and older tools often used the system code page of the machine that created the archive.

**Consequences:**
- Non-ASCII filenames cannot be found after round-trip.
- Hashes differ between Windows and future Linux/macOS builds.
- Writer output differs from BSArchPro or official tools because normalization happens at the wrong boundary.
- Extraction can create unsafe host paths if `..`, drive letters, or absolute paths are not sanitized separately.

**Prevention:**
- Store archive paths internally as normalized byte/UTF-8-like virtual path values, not `std::filesystem::path`.
- Normalize only archive rules: forward slashes, no leading slash, lower-case/hash behavior where the format requires it, explicit encoding policy for user-facing text.
- Convert to host filesystem paths only at extraction/write boundaries after rejecting absolute paths, drive roots, `..`, and reserved traversal forms.
- Add tests for separator normalization, case behavior, duplicate paths, extended-byte names, and extraction traversal rejection.

**Warning signs:**
- Public APIs accept only `std::filesystem::path` for archive member names.
- Tests use only ASCII lowercase paths.
- Hash functions accept host paths directly.
- Extracting an archive can write outside the requested destination with crafted member names.

**Phase to address:**  
Milestone 1 must define the archive path type before hash lookup. Milestones 2-8 must use it consistently. Milestone 10 should add malicious path fixtures.

**Specific compatibility risk:**  
Hash lookup failures, duplicate/missing files, cross-platform incompatibility, and path traversal vulnerabilities during extraction.

---

### Pitfall 6: Parsing binary structures with packed C++ structs instead of explicit little-endian readers

**What goes wrong:**  
The parser casts bytes into `#pragma pack` structs or reads host-endian integral fields directly. It works on one compiler and architecture but breaks under different alignment, padding, endian, or integer-width assumptions.

**Why it happens:**  
Archive record tables look like fixed C structs, and casting is faster to write than building binary readers.

**Consequences:**
- BA2 records with 64-bit offsets are misread on alignment-sensitive platforms.
- Integer overflow in offset + size calculations enables out-of-bounds reads.
- Future Linux/macOS support becomes fragile.
- Fuzzing finds parser crashes instead of clean format errors.

**Prevention:**
- Use explicit `read_u16le`, `read_u32le`, `read_u64le`, bounded slice cursors, and checked arithmetic for every offset and size.
- Validate table sizes before reading records: count * record_size must fit in file size and `size_t`.
- Keep on-disk serialization functions distinct from in-memory metadata types.
- Add malformed fixtures for truncated headers, oversized counts, overlapping ranges, and offset+size overflow.

**Warning signs:**
- `reinterpret_cast<Record*>` appears in parser code.
- Struct definitions are both public API and on-disk serialization schema.
- Warnings are disabled around packing/alignment.
- Parser errors are crashes/assertions rather than typed format errors.

**Phase to address:**  
Milestone 1 foundation; every parser milestone; Milestone 10 fuzzing/hardening.

**Specific compatibility risk:**  
Valid archives fail on some toolchains, and malformed archives can trigger undefined behavior.

---

### Pitfall 7: Loading whole archives or whole extraction sets into memory

**What goes wrong:**  
The library parses by reading entire archive files into memory, extracts all files into `std::vector<std::byte>` before writing them, or repacks by expanding every preserved file first.

**Why it happens:**  
Small fixture archives make whole-buffer implementations simple, and whole-buffer compression APIs encourage buffering at file/chunk granularity. Starfield archives and texture BA2s make this strategy fail.

**Consequences:**
- 10+ GB archives exhaust memory or thrash.
- Multi-threading later amplifies memory use because each worker owns large buffers.
- Consumers cannot stream extraction to custom sinks.
- Rewriter APIs become unusable for asset pipelines.

**Prevention:**
- Read headers/tables into bounded metadata, then seek and stream payloads per file/chunk.
- Keep compression wrappers whole-buffer only at payload/chunk granularity; never whole-archive granularity.
- Writer APIs should defer reading source files until finalization and support caller-provided sinks.
- Add tests that use fake large offsets/sizes and bounded-memory extraction sinks, not just tiny files.

**Warning signs:**
- `open_archive(path)` returns a structure owning all uncompressed file bytes.
- Progress callbacks happen only after every file was already extracted.
- The writer API requires `add_file(path, std::vector<byte>)` for normal disk files.
- Benchmarks are deferred before proving streaming invariants.

**Phase to address:**  
Milestone 1 must establish streaming extraction interfaces. Milestones 5-8 must implement streaming writers. Milestone 9 optimizes after correctness, not before streaming exists.

**Specific compatibility risk:**  
Large Starfield/texture archives become practically unusable even if logically supported.

---

### Pitfall 8: Implementing writers before read compatibility is fixture-proven

**What goes wrong:**  
The project starts emitting archives based on incomplete understanding of sorting, hashes, flags, embedded names, compression decisions, and unknown fields. Later read compatibility fixes require writer rewrites.

**Why it happens:**  
Writers are the visible value, and round-trip tests can pass against the same flawed implementation.

**Consequences:**
- libbsa can read its own archives but official tools, games, or BSArchPro cannot.
- Hash-sorted indexes are wrong, causing game lookup failures.
- File/archive flags are missing or over-set, causing assets not to load or inefficient behavior.
- Embedded-name behavior is wrong for formats where it matters.

**Prevention:**
- Gate each writer phase on read/extract compatibility for that format family.
- Writer exit criteria must include extraction by libbsa, comparison to originals, and compatibility checks against BSArchPro/official tools when available.
- Test hash ordering and serialized table bytes independently of full archive round-trip.
- Preserve format-specific decisions in writer options instead of guessing from file extension alone.

**Warning signs:**
- Round-trip tests are the only writer tests.
- No tests compare produced indexes/hashes/flags to known-good fixtures.
- Writer code constructs records from generic `ArchiveEntry` without format-specific policy.

**Phase to address:**  
Milestones 5-8, with prerequisites from Milestones 1-4. Milestone 10 should audit writer edge cases.

**Specific compatibility risk:**  
Archives are self-consistent but not game-loadable.

---

### Pitfall 9: Error handling that hides corruption or makes recovery impossible

**What goes wrong:**  
Parser and extraction failures collapse into `false`, `nullptr`, generic exceptions, or partial output. Consumers cannot distinguish missing file, unsupported version, corrupt archive, decompression failure, path traversal rejection, and I/O errors.

**Why it happens:**  
Binary parser code often starts as exploratory code. It accumulates asserts and exceptions before public API contracts are defined.

**Consequences:**
- Tools cannot produce actionable diagnostics.
- Bulk extraction continues after corruption and writes bad files.
- Malformed archives crash instead of returning typed failures.
- C++20 API drifts toward C++23-only `std::expected` or exception-heavy behavior.

**Prevention:**
- Define `libbsa::result<T>` or explicit error-code output in Milestone 1.
- Use structured error domains: I/O, unsupported format/version, invalid structure, decompression, texture reconstruction, path normalization/security, and precondition violation.
- Ensure streaming extraction writes to temporary/caller-controlled sinks where partial output can be reported or discarded.
- Document which operations can partially succeed and how callers recover.

**Warning signs:**
- `catch (...) { return false; }` exists in library internals.
- Parser uses `assert` for untrusted archive content.
- API docs do not say whether a failed extraction may have written partial bytes.
- Public headers expose `std::expected` while claiming C++20 support.

**Phase to address:**  
Milestone 1 for result/error model; all parser/writer phases for consistent propagation; Milestone 10 for comprehensive malformed archive handling.

**Specific compatibility risk:**  
Corruption is misreported as unsupported format or successful partial extraction, undermining trust in compatibility tests.

---

### Pitfall 10: Weak fixture design that proves demos, not compatibility

**What goes wrong:**  
Tests use one or two tiny synthetic archives with ASCII names and uncompressed payloads. They pass while real game archives fail on compression, embedded names, non-ASCII bytes, chunked DDS textures, or odd flags.

**Why it happens:**  
Real Bethesda archives are large and may be license-sensitive, so test corpora are hard to assemble. Synthetic fixtures are convenient but can accidentally mirror the implementation's assumptions.

**Consequences:**
- Pitfalls are discovered only after users try real games.
- Writer bugs pass because tests compare libbsa output to libbsa input.
- Edge cases are delayed to “polish” even though they affect core architecture.

**Prevention:**
- Build a fixture matrix by format/version/feature: compressed/uncompressed, embedded names, hash-only lookup, BA2 name table, BA2 DDS chunks, Starfield v2/v3 headers, extended-byte paths, malformed/truncated cases.
- Store tiny generated fixtures where legal, plus scripts/checksums/instructions for optional local validation against real game archives.
- Add compatibility comparison harnesses that can compare extraction output against BSArchPro without modifying or vendoring TES5Edit.
- Label tests by `unit`, `fixture`, `roundtrip`, `compat`, `malformed`, and `slow` so CI can run the right subset.

**Warning signs:**
- “All tests pass” means only synthetic fixtures.
- No fixture asserts exact record offsets, path hashes, or chunk metadata.
- Test archives do not include compressed files for every compression mode.
- Optional real-archive validation is undocumented.

**Phase to address:**  
Milestone 1 test foundation; expand in each format milestone; Milestone 10 adds fuzz/malformed corpus.

**Specific compatibility risk:**  
The roadmap declares support for formats that are only partially exercised.

---

## Moderate Pitfalls

### Pitfall 1: Leaking DirectXTex, libdeflate, or LZ4 types into public headers

**What goes wrong:** Public API stability and portability become tied to dependency headers and platform details.  
**Prevention:** Keep dependency adapters in `src/` and expose libbsa-native metadata/result types only.  
**Warning signs:** Public headers include `DirectXTex.h`, `lz4.h`, or `libdeflate.h`.  
**Phase to address:** Milestone 1 API boundary; Milestone 4/7 texture APIs.

### Pitfall 2: Inferring behavior from file extension instead of archive metadata

**What goes wrong:** `.dds` or `.ba2` extension drives compression and parsing decisions that should come from magic/version/type/chunk records.  
**Prevention:** Use magic bytes and parsed metadata as authoritative; extensions are hints only for host UX.  
**Warning signs:** Compression selection checks `.dds`, `.ba2`, or `.bsa` strings before parsed format/version.  
**Phase to address:** Milestone 1 detection, Milestone 3 BA2 metadata, Milestones 5-7 writers.

### Pitfall 3: Over-compressing assets that should remain uncompressed or warning-worthy

**What goes wrong:** Game behavior/performance suffers, especially for assets loaded on demand. Known quirks such as sounds-in-compressed archives and SSE embedded-name issues are easy to miss.  
**Prevention:** Record archive/file flag policy and add warning diagnostics before writer phases; validate against BSArchPro quirks.  
**Warning signs:** Writer has one global “compress everything” default with no per-file override or warnings.  
**Phase to address:** Milestones 5-8 writer policy; Milestone 10 compatibility warnings.

### Pitfall 4: Treating unknown fields as disposable

**What goes wrong:** Starfield and BA2 version fields that are not fully understood get zeroed or omitted in rewritten archives.  
**Prevention:** Preserve unknown fields from read metadata where possible and require explicit defaults for new archives.  
**Warning signs:** Fields are named `padding` without fixture evidence that they are always padding.  
**Phase to address:** Milestones 3-4 reads; Milestones 6-7 writes.

## Minor Pitfalls

### Pitfall 1: Using game/tool names inconsistently in API and tests

**What goes wrong:** “FO4 BA2 v1/7/8” and “Starfield v2/v3” become ambiguous in test names and options.  
**Prevention:** Define canonical format enum names and fixture naming conventions early.  
**Phase to address:** Milestone 1.

### Pitfall 2: Delaying documentation of compatibility constraints

**What goes wrong:** Non-obvious constraints discovered from BSArchPro are lost and re-broken later.  
**Prevention:** Add short comments for format compatibility decisions and Doxygen for public APIs as required by project rules.  
**Phase to address:** Every phase; formal API docs in Milestone 10.

### Pitfall 3: Assuming Archive2/official tool behavior is fully documented

**What goes wrong:** Writer policy follows incomplete community docs without empirical validation.  
**Prevention:** Treat community docs as hypotheses; verify with fixtures, official tool output, and BSArchPro behavior.  
**Phase to address:** Writer milestones 5-8.

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Single `ArchiveEntry` record for every format | Fast first parser | Version quirks leak everywhere; writer rewrite likely | Only as a public read-only view backed by format-specific internals |
| Whole-archive byte vector | Simpler parsing and fuzzing | Fails large archives, blocks streaming API | Tiny unit tests only, never primary API |
| Golden compressed bytes | Easy snapshot tests | Breaks when libdeflate/lz4 output changes despite equivalent data | Never for dependency-generated compression; compare decompressed bytes/metadata |
| Host filesystem path as archive key | Friendly API | Encoding/separator/hash bugs | Host I/O boundary only |
| Treating `PackedSize == 0` as error | Simplifies compressed path | Uncompressed files become unreadable | Never |
| Deferring malformed-input tests to the end | Faster feature demos | Parser architecture may rely on UB/assertions | Add malformed smoke tests from Milestone 1, expand in Milestone 10 |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| TES5Edit / BSArchPro | Editing, formatting, compiling, or staging submodule files | Read-only behavioral reference; compare outputs externally; never modify `TES5Edit/` |
| libdeflate | Ignoring wrapper distinction and actual output size | Use selected raw/zlib API per verified format; require exact decompressed size; map result codes |
| official lz4 | Using one LZ4 API for both SSE and Starfield | Separate frame and raw block adapters with format-driven routing |
| DirectXTex | Exposing DirectXTex types in libbsa API | Translate to internal/public libbsa metadata; keep DirectXTex behind texture boundary |
| vcpkg/CMake | Letting dependency headers leak through transitive public targets | Link privately where possible; keep public headers dependency-light |
| BSArchPro compatibility checks | Comparing only file lists | Compare extracted bytes, metadata, flags, hashes, ordering, and known warning behavior |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Full archive read into RAM | Huge memory spikes, slow open | Metadata-only open plus seeked payload reads | Large FO4/Starfield archives and texture packs |
| Extract-all before write | Repacking requires temporary copies of every file | Deferred source reads and streaming writer sinks | Any multi-GB archive rewrite |
| Per-byte I/O abstractions | Correct but very slow extraction | Buffered readers/writers and chunk-level operations | Thousands of small files or compressed texture chunks |
| Parallel compression before isolation | Data races and nondeterministic corruption | Immutable work items, per-thread compressor/decompressor instances, deterministic final table assembly | Milestone 9 multi-threading |
| Hash lookup without normalized cached keys | Repeated path normalization/hash cost | Normalize once at parse/add time; cache format-specific hashes | Large file tables |

## Security / Robustness Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Trusting offsets/counts from archive headers | OOB reads, crashes, huge allocations | Checked arithmetic, file-size bounds, count limits, typed format errors |
| Extracting paths verbatim | Path traversal / overwrite outside destination | Reject absolute paths, drive roots, `..`, and unsafe separators before host write |
| Unsafe LZ4 fast decompression | Reads past input on malformed archives | Use safe decompression APIs only for untrusted archive data |
| Assertions on archive content | Release behavior differs; debug crashes | Return typed `invalid_archive` errors for untrusted input |
| Partial output on failure | Consumers use corrupted extracted files | Document partial-write semantics and prefer temp/sink-controlled writes |

## "Looks Done But Isn't" Checklist

- [ ] **TES4-family read:** Can extract compressed and uncompressed files, validates embedded-name behavior, and matches BSArchPro bytes.
- [ ] **TES3 read/write:** Offset math is data-section-relative and tested separately from TES4 offsets.
- [ ] **BA2 GNRL read:** Parses file name table at `FileTableOffset`, handles `PackedSize == 0`, Starfield v2 unknown fields, and v3 `CompressionMethod`.
- [ ] **BA2 DDS read:** Reconstructed DDS loads through DirectXTex and preserves mip/cubemap metadata, not merely a `DDS ` magic.
- [ ] **Writer support:** Produced archives are tested outside libbsa self-round-trip against known-good behavior or tools.
- [ ] **Compression:** Deflate, LZ4 frame, and LZ4 block all have separate exact-size tests and malformed-input failures.
- [ ] **Paths:** Tests include mixed separators, case behavior, extended-byte names, duplicate/conflicting names, and traversal rejection.
- [ ] **Streaming:** API can extract a single large file to a sink without loading the whole archive or all files.
- [ ] **Errors:** Missing file, unsupported version, corrupt record, decompression failure, and unsafe path return distinct errors.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Unified format model | HIGH | Freeze public view, split internal modules, add adapter layer, migrate tests by format |
| LZ4 API confusion | MEDIUM | Add explicit codec wrappers, route by parsed metadata, regenerate affected fixtures, audit writer output |
| DDS reconstruction wrong | HIGH | Replace hand header logic with DirectXTex-backed metadata, build mip/cubemap fixture suite, revalidate BA2 DX10 writer |
| Path normalization wrong | HIGH | Introduce archive path type, deprecate path-based APIs, recompute hash tests, add encoding/traversal fixtures |
| Whole-archive memory design | HIGH | Refactor open/extract/write APIs around readers/sinks, add bounded-memory tests before performance work |
| Weak fixture corpus | MEDIUM | Create fixture matrix, mark unsupported gaps explicitly, add optional real-archive compatibility harness |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Unified format model | Milestone 1 | Format enum + separate parser modules; no cross-format on-disk record struct |
| LZ4 frame/block confusion | Milestones 1, 3, 6, 7 | SSE frame fixture and Starfield raw block fixture both pass exact byte extraction |
| Deflate wrapper/size ambiguity | Milestones 1, 3, 5, 6, 10 | Exact-size decompression tests and malformed compressed-data tests |
| BA2 DDS reconstruction | Milestones 4, 7, 10 | Reconstructed DDS files load via DirectXTex and match source texture metadata |
| Archive path normalization | Milestone 1; expanded through 8 and 10 | Hash lookup tests for normalized/extended-byte paths and traversal rejection tests |
| Packed-struct parsing | Milestone 1; every parser phase | Checked little-endian reader tests plus malformed count/offset fixtures |
| Whole-archive memory loading | Milestones 1, 5-9 | Extraction/writer tests using streaming sinks and bounded memory expectations |
| Writer before read proof | Milestones 5-8 | Writer phases blocked until read fixtures for same format are green |
| Error model collapse | Milestone 1 and 10 | Public `result`/error taxonomy and distinct failure tests |
| Weak fixtures | Milestone 1 onward | Fixture matrix coverage report by format/version/feature |

## Sources

- Project context and requirements: `J:\libbsa-gsd\.planning\PROJECT.md`, `J:\libbsa-gsd\docs\PRD.md`, `J:\libbsa-gsd\AGENTS.md`.
- LZ4 official documentation via Context7 `/lz4/lz4`: `LZ4_decompress_safe` requires exact compressed size and destination capacity and returns negative errors; frame API (`LZ4F_*`) produces self-describing frames with magic bytes.
- libdeflate official header: raw DEFLATE, zlib, and gzip APIs are separate; decompression reports `LIBDEFLATE_BAD_DATA`, `LIBDEFLATE_SHORT_OUTPUT`, and `LIBDEFLATE_INSUFFICIENT_SPACE`; compressed output is not stable across library versions. https://github.com/ebiggers/libdeflate/blob/master/libdeflate.h
- DirectXTex official documentation via Context7 `/microsoft/directxtex`: `LoadFromDDSMemory`, `GetMetadataFromDDSMemory`, `TexMetadata`, `ScratchImage`, and DDS header helpers support DDS metadata validation/reconstruction.
- `ba2` Rust documentation: BA2 variants, Starfield LZ4 introduction, texture chunking, and byte-string path warning. https://docs.rs/ba2/latest/ba2/ and https://docs.rs/ba2/latest/ba2/fo4/index.html
- `dream_archive` Rust documentation: byte-string archive paths, explicit encoding helpers, streaming/deferred builder behavior, BA2 DX10 DDS reconstruction, and unsupported console/XMem scope. https://docs.rs/dream_archive/latest/dream_archive/
- Bethesda Structs documentation: BSA v105 uses LZ4; BTDX has GNRL and DX10, and DX10 extraction requires rebuilding DDS headers. https://bethesda-structs.readthedocs.io/en/latest/bethesda_structs.archive.html
- BA2 format notes: little-endian records, no padding/alignment, name table, GNRL records, and texture chunk records. https://miere.ru/posts/ba2-archive-format/
- GECK BSA documentation: archive/file flags, compression tradeoffs, BSA loading behavior, and asset-type flag expectations. https://geckwiki.com/index.php/BSA_Files
- STEP Archive2 guide: official Archive2 context, BA2 general vs DDS settings, max chunk count, and texture BA2 streaming rationale. https://stepmodifications.org/wiki/Guide:Archive2

---
*Pitfalls research for: libbsa reusable C++20 Bethesda BSA/BA2 archive library*  
*Researched: 2026-05-07*
