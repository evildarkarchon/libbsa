# Pitfalls Research

**Domain:** C++20 Bethesda BSA/BA2 parser-writer library  
**Researched:** 2026-05-05  
**Confidence:** HIGH for PRD/reference-code risks and compression/DDS API behavior; MEDIUM for community-documented BA2 details that are not official Bethesda specifications.

## Critical Pitfalls

### Pitfall 1: Treating archive version as a cosmetic field

**What goes wrong:**
The library parses `BSA\0` or `BTDX` and then reuses one record layout or one compression path across games. This silently misreads SSE v105 folder records, Starfield BA2 extra headers, Morrowind offset semantics, or FO4/Starfield BA2 variants. Writers then create archives that extract locally but are ignored by or crash the target game.

**Why it happens:**
The formats look similar by magic bytes, and many public format notes focus on Skyrim or Fallout 4 rather than the whole Morrowind-to-Starfield matrix. Nexus' archive compatibility table explicitly shows that BSA/BA2 variants are not interchangeable across games.

**How to avoid:**
- Build a strict format registry keyed by magic + version + BA2 subtype (`GNRL`/`DX10`) + Starfield `CompressionMethod`.
- Model each target game/version as an explicit enum with its own record sizes, offsets, flags, and compression codec.
- Reject unknown versions by default, but return a structured `unsupported_version` error that includes the raw header fields.
- Add one fixture per supported version before implementing writer support for that version.

**Warning signs:**
- Code branches on only `BSA` vs `BTDX`.
- Tests use only Skyrim LE/SSE and FO4 BA2 fixtures.
- Writer API accepts a raw numeric version instead of a target-game preset.
- New fields such as Starfield `Unknown1`, `Unknown2`, or `CompressionMethod` are stored in an anonymous padding blob.

**Phase to address:**
Milestone 1 (auto-detection architecture), Milestone 3 (BA2 GNRL), Milestone 4 (BA2 DDS), then enforce in every writer milestone.

---

### Pitfall 2: Confusing compression flags with actual per-file compression

**What goes wrong:**
The library assumes an archive-level compression flag means every file is compressed, or assumes `PackedSize == 0` means compressed. TES4-family BSA uses an inverted per-file bit (`0x40000000`) relative to the archive default. BA2 uses `PackedSize != 0` for compressed payloads. The result is decompression attempted on raw data, raw reads of compressed data, or corrupted extraction offsets.

**Why it happens:**
Bethesda encodes compression policy differently per family. UESP documents that the BSA compressed-archive flag is only the default and that the file size high bit means the file's compression state is opposite the default. The TES5Edit reference mirrors this by toggling `bCompressed` based on both file size and archive flags.

**How to avoid:**
- Define `is_compressed(record, archive_header)` separately for TES4-family BSA and BA2.
- Strip the TES4 `FILE_SIZE_COMPRESS` bit before using size for I/O.
- Store logical `uncompressed_size`, `stored_size`, and `compression` separately in public metadata.
- Add tests for all four BSA combinations: archive default compressed/uncompressed × per-file override present/absent.

**Warning signs:**
- Public API exposes only one `size` field.
- Code checks only `archiveFlags & COMPRESS`.
- BA2 extractor calls decompression when `PackedSize == 0`.
- Writer always compresses when archive default says compress, ignoring per-file override.

**Phase to address:**
Milestone 1 for TES4-family reading, Milestone 3 for BA2 reading, Milestones 5-7 for writer metadata.

---

### Pitfall 3: Mixing up zlib-wrapped deflate, raw deflate, LZ4 frame, and raw LZ4 block

**What goes wrong:**
Archives appear to parse but decompression fails, hangs, or produces subtly corrupted bytes. SSE BSA uses LZ4 frame data, Starfield BA2 v3 can use raw LZ4 block when `CompressionMethod == 3`, while TES4/FO3 and FO4/SF deflate paths need their own zlib/deflate expectations. Using a frame decoder for a block or a block decoder for a frame is not recoverable by guesswork.

**Why it happens:**
The project has three real compression modes plus a deflate wrapper question. LZ4's official APIs are split: `LZ4F_decompress` is frame-oriented and returns frame hints/errors, while `LZ4_decompress_safe` decodes a raw block when the caller already knows the uncompressed size. libdeflate also distinguishes zlib-wrapped data from raw deflate, with separate APIs and explicit output-buffer size requirements.

**How to avoid:**
- Create a `CompressionCodec` abstraction with explicit values: `zlib_deflate`, `lz4_frame`, `lz4_block`.
- Select codec only from archive family/version/`CompressionMethod`; never auto-probe data in normal paths.
- For LZ4 frame, use frame API (`LZ4F_*`) and check `LZ4F_isError`/`LZ4F_getErrorName`.
- For LZ4 block, use `LZ4_decompress_safe` with the exact uncompressed size from the record/chunk.
- For libdeflate, write a compatibility test that proves the chosen zlib/raw API extracts known TES4/FO3/FO4 data and handles PRD's Fallout vanilla zlib tolerance.

**Warning signs:**
- One function named `decompress_lz4` with no frame/block parameter.
- Code branches on file extension instead of archive version/method.
- Decompression tests only round-trip data compressed by libbsa itself.
- Error handling converts all decompression failures to “bad archive” without codec-specific diagnostics.

**Phase to address:**
Milestone 1 (deflate + SSE LZ4 frame), Milestone 3 (Starfield LZ4 block), Milestone 4 (DDS chunk compression), Milestone 10 (malformed/tolerance hardening).

---

### Pitfall 4: Ignoring known Bethesda-compatible decompression weirdness

**What goes wrong:**
The library rejects vanilla archives that existing tools tolerate, or it truncates/corrupts data while trying to be permissive. TES5Edit explicitly ignores a zlib “Buffer error” for vanilla `Fallout - Misc.bsa`, noting Bethesda probably packed it with an old buggy zlib version.

**Why it happens:**
Parser authors often equate “library decompressor returned warning/error” with “archive is invalid.” Compatibility libraries need to distinguish malicious malformed input from known historical encoder bugs.

**How to avoid:**
- Encode a named compatibility mode for known vanilla zlib buffer-error tolerance.
- Limit tolerance to specific codec/archive families and only after output buffer contents/expected size are validated.
- Return warnings alongside successful extraction so callers can log questionable archives.
- Add a fixture reproducing the vanilla Fallout zlib edge case, or a minimal synthetic fixture if redistribution is not possible.

**Warning signs:**
- All decompressor non-success returns are fatal.
- Tests use only freshly generated compressed data.
- Compatibility notes are in comments only, with no error-code representation.

**Phase to address:**
Milestone 1 for error model shape; Milestone 10 for full compatibility hardening.

---

### Pitfall 5: Mishandling TES4-family embedded filenames

**What goes wrong:**
Extraction includes the embedded filename bytes in the output, skips the wrong number of bytes, or writes archives that crash SSE. UESP notes older tools have corrupted data by prepending/appending embedded names incorrectly. TES5Edit also documents a Skyrim SE engine crash bug when texture data is uncompressed and the filename is embedded, so it avoids setting `ARCHIVE_EMBEDNAME` for SSE texture-only archives.

**Why it happens:**
`ARCHIVE_EMBEDNAME` affects the file data block, not just the filename table. It is easy to treat it as a metadata-only flag or to apply Skyrim LE texture conventions to SSE.

**How to avoid:**
- Parse embedded names as a data-block prefix before uncompressed-size/decompressed payload handling.
- Subtract embedded-name prefix length from stored size before decompression/read.
- Writer must derive flags by target game: texture-only archives may embed names for pre-SSE, but must not create the SSE uncompressed-texture crash combination.
- Include fixtures with `ARCHIVE_EMBEDNAME` set and both compressed/uncompressed entries.

**Warning signs:**
- Extracted files begin with a path-like byte string.
- Writer has a user-facing “embed names” checkbox/API with no target-game guardrails.
- SSE texture writer shares the Skyrim LE flag derivation exactly.

**Phase to address:**
Milestone 1 (read embedded-name handling), Milestone 5 (write flag derivation), Milestone 10 (SSE crash-bug hardening).

---

### Pitfall 6: Reconstructing BA2 DDS as if chunks were complete DDS files

**What goes wrong:**
Extracted textures are unreadable, have missing mip levels, wrong sRGB/DXGI formats, broken cubemaps, or load in one DDS viewer but fail in-game. BA2 `DX10` texture archives store DDS payload chunks without the original DDS header; readers must synthesize a correct DDS/DX10 header from archive metadata and append chunks in mip order.

**Why it happens:**
Texture BA2 archives are optimized for runtime mip streaming, not for round-tripping original `.dds` files byte-for-byte. Community BA2 docs and bethesda_structs both call out that DX10 extraction requires DDS header reconstruction. Microsoft's DDS docs require a `DDS ` magic, 124-byte base header, optional 20-byte `DDS_HEADER_DXT10`, correct mip/cube/array metadata, and robust pitch/linear-size handling.

**How to avoid:**
- Treat BA2 DDS extraction as “construct DDS file” rather than “dump bytes.”
- Centralize DXGI-to-DDS header mapping and validate with DirectXTex load/save APIs.
- Add cubemap tests for all six faces, including DX10 cubemap flags (`DDS_RESOURCE_MISC_TEXTURECUBE`).
- For formats not representable in legacy `DDS_PIXELFORMAT`, always emit `DX10` extension.
- Verify extracted DDS files load in DirectXTex and compare image metadata/mip dimensions, not just byte count.

**Warning signs:**
- Extracted BA2 DDS files do not begin with `DDS `.
- Implementation copies chunk bytes to disk without consulting width/height/mip/DXGI fields.
- `CubeMaps` is treated as an opaque unknown and never affects DDS caps/DX10 misc flags.
- Tests validate only that extraction returns non-empty bytes.

**Phase to address:**
Milestone 4 (DDS read), Milestone 7 (DDS write), Milestone 10 (edge-case texture validation).

---

### Pitfall 7: Getting BA2 DDS mip chunking wrong on write

**What goes wrong:**
FO4/Starfield loads the archive, but textures stream poorly, show wrong mips, or corrupt only at distance. Texture chunks need correct start/end mip ranges, per-chunk offsets/sizes/compression, and version-specific compression. Archive2 guidance notes texture BA2 stores high-resolution mips separately while lower mips are grouped for streaming; TES5Edit computes variable file-record size from the number of mip chunks and stores the last chunk as all remaining mips.

**Why it happens:**
It is tempting to write one chunk per DDS or one chunk per mip without matching engine/tool conventions. Chunking also changes file-record table size before data offsets can be known, so a simple append-only writer can miscalculate offsets.

**How to avoid:**
- Analyze DDS metadata before reserving BA2 records; do not begin writing data before chunk count is known.
- Implement a deterministic chunking policy aligned with Archive2/BSArchPro behavior, then make alternatives explicit configuration only after compatibility tests exist.
- Force chunk compression where reference behavior requires it, and set `PackedSize` per chunk.
- Round-trip pack/extract DDS and validate every mip's byte span against the source surface layout.

**Warning signs:**
- BA2 DDS writer can accept raw bytes without DDS metadata or DirectXTex analysis.
- File table offsets are patched after writing but chunk-record reservation did not account for variable chunk counts.
- Tests use only one-mip textures.

**Phase to address:**
Milestone 7 primarily; Milestone 4 should expose enough read metadata to design it safely.

---

### Pitfall 8: Assuming archive paths are normal Unicode filesystem paths

**What goes wrong:**
Lookups fail for legacy archives, hashes do not match, or writer output differs from official tools. TES4 hashes require lower-case names and backslash separators; BA2 name tables commonly use `/` as written by Archive2. Existing ecosystem docs warn that Bethesda archive paths do not reliably declare filename encodings and older tools may use the producer machine's code page.

**Why it happens:**
C++20 library authors naturally reach for `std::filesystem::path`/UTF-8 strings. Bethesda archive lookup is byte-oriented and hash-sensitive; normalization choices change identity.

**How to avoid:**
- Internally store archive paths as normalized byte strings plus a documented display conversion layer.
- Define per-format path normalization: TES4 hash input lower-case + `\`; BA2 output/name-table convention `/` but lookup should accept normalized equivalents.
- Provide explicit legacy-codepage encoding hooks or document exact UTF-8-only limitations before claiming compatibility.
- Add fixtures with mixed separators, case differences, and at least one non-ASCII legacy path byte sequence.

**Warning signs:**
- Hash functions take `std::filesystem::path` directly.
- Code normalizes to platform separators.
- Tests run only on ASCII lowercase paths.
- Public API says “Unicode-safe” without describing archive byte encoding.

**Phase to address:**
Milestone 1 (TES4 hash/lookup API), Milestone 2 (TES3 names), Milestone 3 (BA2 name table), Milestones 5-8 (writer normalization).

---

### Pitfall 9: Wrong hash sorting and table layout in writers

**What goes wrong:**
Written archives contain all bytes but game lookup fails because folder/file records are unsorted or offsets are relative to the wrong base. TES4-family folders/files must be sorted by 64-bit hash. TES3 stores file offsets relative to the data section. BA2 writes file data before the name table and records `FileTableOffset` at save/finalize.

**Why it happens:**
Archive readers can often linearly scan names and still find files, masking writer bugs. Game engines use hash tables and expect exact layout/sort contracts.

**How to avoid:**
- Writer tests must open written archives through libbsa and an independent reference (BSArchPro/official tool where possible), then attempt hash lookup, not just sequential extraction.
- Keep offset base semantics in type-specific serializers; never have a generic “write offset” helper without a base parameter.
- Add golden metadata tests for sorted record order and offset values on small hand-built archives.

**Warning signs:**
- Writer preserves input insertion order.
- TES3 and TES4 records share the same offset serializer.
- `FileTableOffset` is computed before all data/chunks are actually appended.

**Phase to address:**
Milestone 2 (TES3 relative offsets), Milestones 5-8 (all writers), Milestone 10 (compatibility comparison).

---

### Pitfall 10: Whole-archive or whole-file memory assumptions disguised as “streaming”

**What goes wrong:**
The library works for toy fixtures but becomes unusable on 10-50+ GB Starfield archives. It may read full files into memory before decompression, rewriters may extract every preserved file into `std::vector`, and DDS BA2 extraction may allocate all chunks at once. PRD explicitly calls out streaming I/O for large archives and large archive performance risk.

**Why it happens:**
DOM-style APIs are simpler, and compression APIs often require known output sizes. TES5Edit's `ExtractFileData` returns `TBytes`, which is useful reference behavior but not the reusable library surface libbsa needs.

**How to avoid:**
- Design read APIs around caller-provided sinks/readers first; keep `read_file_bytes` as a convenience wrapper with documented size limits.
- For compressed entries, stream compressed bytes into bounded buffers and write decompressed output incrementally where the codec supports it; where one-shot block decompression is required, bound allocation to one chunk/file and validate sizes first.
- Writer `add_file(path)` should defer reading payload until `finalize()`; rewriter should preserve unchanged archive entries by streaming from old archive to new archive.
- Add benchmarks/large synthetic fixtures early enough to catch accidental whole-archive reads.

**Warning signs:**
- Core extractor returns only `std::vector<std::byte>`.
- Repack implementation materializes all input files before writing headers.
- Tests never exceed a few megabytes.
- “Streaming” means `std::ifstream` internally but still returns a full buffer.

**Phase to address:**
Milestone 1 API shape; Milestones 5-8 writer finalization; Milestone 9 performance; Milestone 10 examples/docs.

---

### Pitfall 11: Parallel extraction/packing shares mutable stream state unsafely

**What goes wrong:**
Parallel reads intermittently return bytes from the wrong offset, or parallel packing writes corrupted interleaved data. Bugs appear only under load and are misdiagnosed as decompressor failures.

**Why it happens:**
Archive extraction is seek-heavy. A single mutable file stream with shared `position` cannot be safely used concurrently without locking or independent handles. TES5Edit uses synchronization around stream positioning and releases locks around expensive compression/decompression, which signals the ownership/ordering problem.

**How to avoid:**
- Make archive objects immutable after open, but give each concurrent reader an independent stream handle or a positioned read abstraction.
- For packing, reserve write ranges or serialize physical writes while allowing compression jobs to run independently.
- Document thread-safety at the type level: what can be shared, what requires a clone/reader, what is single-threaded.
- Add stress tests that extract many random entries concurrently and hash outputs against single-threaded extraction.

**Warning signs:**
- Code calls `seekg()` then `read()` on a shared stream from multiple threads.
- Mutex covers decompression as well as I/O, eliminating parallel speedup.
- Thread-safety docs are deferred until after the implementation exists.

**Phase to address:**
Milestone 1 (avoid APIs that preclude safe readers), Milestone 9 (parallel implementation), Milestone 10 (thread-safety docs).

---

### Pitfall 12: Trusting archive sizes, offsets, counts, and string lengths

**What goes wrong:**
Malformed archives cause out-of-bounds reads, enormous allocations, integer overflow, path traversal on extract, or infinite loops. Archive parsers process untrusted local files even when the domain is modding.

**Why it happens:**
Format docs present well-formed layouts; implementation follows them directly. Large 64-bit offsets plus 32-bit sizes and variable-length name tables are easy to combine unsafely.

**How to avoid:**
- Parse with checked arithmetic for every offset + size + header reservation calculation.
- Validate all counts against file length before allocating vectors.
- Validate `FileTableOffset`, BA2 chunk offsets, TES4 folder offsets, and TES3 data-relative offsets before seeking.
- For extraction to disk, sanitize paths and reject absolute paths, drive prefixes, `..`, and control characters by default.
- Add fuzz targets for header/table parsing and malformed fixture tests for truncated tables, oversized counts, negative/overflowing offset math, and unterminated strings.

**Warning signs:**
- `resize(header.file_count)` happens before checking the file length can contain that many records.
- `offset + size` is computed in 32-bit types.
- Extract-all joins archive path directly to destination path.
- Parser uses exceptions/crashes instead of structured format errors on truncated input.

**Phase to address:**
Milestone 1 parser foundation; expand in Milestones 3-4 for BA2; Milestone 10 for fuzzing/hardening.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| One generic `ArchiveRecord` for all formats | Less code in MVP | Hides version-specific sizes, offset bases, compression semantics | Never for serialization; acceptable only as a read-only public metadata projection |
| `std::string` as “path” everywhere | Fast API ergonomics | Encoding/hash bugs and platform separator leakage | Only if documented as archive-byte string wrapper, not filesystem path |
| One-shot `read_file()` as core primitive | Easy tests | Blocks streaming and large archive support | As convenience wrapper layered on streaming API |
| Tests only against archives written by libbsa | Easy CI | Confirms self-consistency, not Bethesda compatibility | Never as sole exit criterion |
| Store unknown fields but never assert values | Avoids decisions | Writers emit incompatible Starfield/BA2 headers | Acceptable in readers; writers need target-specific constants backed by fixtures |
| DirectXTex types in public headers | Speeds DDS implementation | Couples consumers to Windows/DirectX details and hurts portability | Never in public API; acceptable behind private adapter |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| libdeflate | Using raw deflate API for zlib-wrapped Bethesda data or vice versa | Add fixture-backed wrapper selection; preserve exact uncompressed-size contract and report `bad_data` vs `insufficient_space` |
| lz4 | Using `LZ4F_*` for Starfield raw blocks or `LZ4_decompress_safe` for SSE frames | Separate `lz4_frame` and `lz4_block` codecs selected by format/version/method |
| DirectXTex | Treating it as a full archive DDS solution | Use it to validate/analyze DDS metadata; libbsa still owns BA2 chunk records and Bethesda-specific headers |
| BSArchPro/TES5Edit | Translating Pascal classes directly or modifying submodule | Trace behavior into tests and comments; keep TES5Edit read-only and implementation independent |
| Official tools | Assuming Archive2 settings are documented enough | Use official tools for compatibility fixtures, but verify actual bytes/metadata against reference behavior |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Whole archive read | High memory, slow startup, crashes on large BA2 | Parse tables only; stream entry data by offset | Starfield texture/general archives and mod packs > several GB |
| Per-entry open/close with no handle strategy | Slow extract-all due to file handle churn | Use positioned reads or a small pool of independent handles | Thousands of small files |
| Lock around decompression | Parallel mode no faster than single-thread | Lock only around shared I/O or avoid shared streams entirely | Milestone 9 benchmarks |
| DDS chunk coalescing | Texture extraction allocates huge buffers | Process chunks sequentially into sink; reconstruct header first | Large BC7/4K texture packs |
| Hash lookup via linear path scan | Lookup API degrades with archive size | Build hash/path index after table parse | Large BA2/BSA with tens of thousands of entries |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Trusting record counts and string lengths | OOM or out-of-bounds read | Preflight table byte ranges and cap allocations |
| Unsanitized extract paths | Path traversal outside destination | Normalize archive paths and reject absolute/parent traversal by default |
| Integer overflow in offset math | Reads wrong data or bypasses bounds checks | Use 64-bit checked arithmetic and compare against file length |
| Decompression bombs | Huge allocations or CPU burn | Enforce declared uncompressed sizes, caller-configurable max output, and chunked processing |
| Permissive unknown-version writes | Games crash on invalid archives | Reject unknown writer targets; reader-only unknown metadata mode if needed |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| “Supports BSA” without target-game specificity | Consumers create wrong archives for SSE/FO3/etc. | API names target games/formats explicitly |
| Silent compatibility tolerances | Callers cannot warn about suspect vanilla/buggy data | Return success-with-warnings diagnostics |
| Exposing only hash lookup | Users cannot inspect path/name-table issues | Provide list/metadata APIs with raw path bytes and display string |
| No clear unsupported-format errors | Tools show generic failure | Include magic, version, subtype, compression method in errors |
| Hidden memory behavior | Embedders unexpectedly allocate gigabytes | Document which APIs buffer, stream, or mmap/read-positionally |

## "Looks Done But Isn't" Checklist

- [ ] **TES4-family read:** Tests cover archive default compression, per-file compression inversion, embedded names, v103/v104/v105 record differences, and SSE LZ4 frame.
- [ ] **TES3 read/write:** Offsets are proven data-section-relative and hash table order matches reference behavior.
- [ ] **BA2 GNRL read:** Name table at `FileTableOffset`, slash normalization, `PackedSize != 0`, Starfield v2/v3 headers, and `CompressionMethod == 3` LZ4 block are all tested.
- [ ] **BA2 DDS read:** Output starts with valid DDS header, DirectXTex can load it, all mip chunks are present in order, and cubemap/DX10 flags are correct.
- [ ] **Writers:** Output is loadable/extractable by independent tools, not just libbsa; metadata order and offsets are golden-tested.
- [ ] **Streaming:** Large-file tests verify no whole-archive allocation and convenience APIs are clearly labeled.
- [ ] **Compatibility hardening:** Known zlib tolerance, SSE `EMBEDNAME` crash avoidance, sounds-in-compressed warnings, and malformed/truncated archives have tests or tracked fixtures.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Version/layout modeled too generically | HIGH | Split records/serializers by format, migrate tests to target-game fixtures, deprecate ambiguous APIs |
| Wrong compression abstraction | MEDIUM-HIGH | Introduce explicit codec enum, rewrite decompression dispatch, regenerate fixtures for each codec |
| BA2 DDS header/chunk bugs | HIGH | Rebuild DDS metadata mapper, add DirectXTex validation, compare against BSArchPro/Archive2 extraction |
| Whole-buffer API baked into public surface | HIGH | Add streaming primitives underneath and mark buffer APIs convenience-only; may require API break |
| Hash/path normalization wrong | MEDIUM | Add byte-path type and compatibility helpers; update hash tests and lookup APIs |
| Malformed archive crashes | MEDIUM | Add checked parser layer, structured errors, and fuzz/malformed fixture suite |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Version-specific layouts | Milestone 1, 3, 4 | Fixture per magic/version/subtype; unsupported version returns structured error |
| Compression flag semantics | Milestone 1, 3, 5-7 | Matrix tests for BSA compression inversion and BA2 `PackedSize` behavior |
| Codec frame/block/wrapper confusion | Milestone 1, 3, 4 | Codec-specific fixtures; LZ4 frame and LZ4 block tests fail if swapped |
| Bethesda zlib tolerance | Milestone 10, with error model in Milestone 1 | Vanilla/synthetic fixture emits warning but extracts expected bytes |
| Embedded filenames/SSE crash bug | Milestone 1, 5, 10 | Embedded-name extraction fixtures; SSE writer cannot create prohibited flag/data combination |
| BA2 DDS reconstruction | Milestone 4 | DirectXTex loads extracted DDS; metadata/mip/cubemap assertions |
| BA2 DDS chunking | Milestone 7 | Pack/extract every mip and compare to source; game/tool compatibility smoke test |
| Path encoding/hash normalization | Milestone 1-3, 5-8 | Hash golden vectors and lookup fixtures for case/separator/non-ASCII bytes |
| Writer sorting/offset layout | Milestone 5-8 | Golden record-order/offset tests and reference-tool extraction |
| Streaming I/O | Milestone 1 API design; Milestones 5-9 | Large synthetic archive test with allocation budget/benchmark |
| Threaded I/O safety | Milestone 9 | Concurrent random extraction stress test with hashes matching single-threaded output |
| Malformed input hardening | Milestone 1 baseline; Milestone 10 | Truncated/overflow/fuzz tests return errors, not crashes/OOM |

## Sources

- Project context and PRD: `.planning/PROJECT.md`, `docs/PRD.md` (HIGH confidence).
- TES5Edit reference: `TES5Edit/Core/wbBSArchive.pas` lines 32, 290-301, 990-997, 1143-1198, 1506-1517, 1662-1711, 1790-1812, 1863-1885, 1964-2085, 2180-2396 (HIGH confidence for intended BSArchPro-compatible behavior; submodule is read-only reference).
- UESP Skyrim archive format: https://www.uesp.net/wiki/Skyrim_Mod:Archive_File_Format (MEDIUM/HIGH community format reference; documents flags, compression inversion, embedded names, sorting, endian note).
- Nexus Mods Bethesda mod archives: https://wiki.nexusmods.com/index.php/Bethesda_mod_archives (MEDIUM; compatibility matrix and user-facing archive mismatch risks; last edited 2023-04-12).
- STEP Archive2 guide: https://wiki.step-project.com/Guide:Archive2 (MEDIUM; Archive2 settings and texture BA2 streaming/chunking notes; last edited 2021-12-05).
- BA2 archive format notes: https://miere.ru/posts/ba2-archive-format/ (LOW/MEDIUM; useful reverse-engineered BA2 structures, marked work-in-progress).
- dream_archive docs: https://docs.rs/dream_archive/latest/dream_archive/ (MEDIUM; independent library design notes, byte-string path warning, feature support summary).
- LZ4 official/Context7 docs for frame API and block API: `/lz4/lz4` query on LZ4 frame vs raw block APIs (HIGH for API behavior).
- libdeflate/Context7 docs for zlib vs raw deflate APIs and output buffer requirements: `/libdeflater/libdeflater` query (HIGH for API behavior).
- Microsoft DDS Programming Guide: https://learn.microsoft.com/windows/win32/direct3ddds/dx-graphics-dds-pguide and cubemap layout: https://learn.microsoft.com/windows/win32/direct3ddds/dds-file-layout-for-cubic-environment-maps (HIGH for DDS header/cubemap requirements).

---
*Pitfalls research for: libbsa BSA/BA2 archive compatibility*  
*Researched: 2026-05-05*
