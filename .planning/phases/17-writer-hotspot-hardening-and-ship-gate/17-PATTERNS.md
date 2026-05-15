# Phase 17: Writer Hotspot Hardening and Ship Gate - Pattern Map

**Mapped:** 2026-05-14
**Files analyzed:** 18 new/modified files
**Analogs found:** 18 / 18

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/formats/bsa/tes4_bsa_layout.cpp` | format layout service | batch + file-I/O + transform | `src/formats/ba2/ba2_gnrl_layout.cpp` | role-match |
| `src/formats/ba2/ba2_gnrl_prepare.hpp` | model / prepared-entry contract | transform | `src/formats/ba2/ba2_gnrl_prepare.hpp` | exact |
| `src/formats/ba2/ba2_gnrl_prepare.cpp` | preparation service | file-I/O + transform + batch | `src/formats/ba2/ba2_gnrl_prepare.cpp` | exact |
| `src/formats/ba2/ba2_gnrl_layout.cpp` | format layout service | batch + file-I/O + transform | `src/formats/ba2/ba2_gnrl_layout.cpp` | exact |
| `src/formats/ba2/ba2_dx10_writer.cpp` | writer orchestration service | request-response + file-I/O + batch | `src/formats/ba2/ba2_dx10_writer.cpp` | exact |
| `src/formats/ba2/ba2_dx10_snapshot_builder.cpp` | snapshot builder service | file-I/O + transform | `src/formats/ba2/ba2_dx10_snapshot_builder.cpp` | exact |
| `include/libbsa/writer.hpp` | public API contract / documentation | request-response | `include/libbsa/writer.hpp` | exact |
| `docs/target-format-guide.md` | public docs | documentation transform | `docs/target-format-guide.md` | exact |
| `docs/integration-examples.md` | public docs | documentation transform | `docs/integration-examples.md` | exact |
| `tests/unit/tes4_bsa_writer_tests.cpp` | runtime test | file-I/O + request-response | `tests/unit/tes4_bsa_writer_tests.cpp` | exact |
| `tests/unit/ba2_gnrl_writer_tests.cpp` | runtime test | file-I/O + request-response | `tests/unit/ba2_gnrl_writer_tests.cpp` | exact |
| `tests/unit/ba2_dx10_writer_tests.cpp` | runtime test | file-I/O + request-response | `tests/unit/ba2_dx10_writer_tests.cpp` | exact |
| `tests/unit/writer_hotspot_policy_tests.cpp` | source-policy test | file-I/O + static analysis | `tests/unit/parser_preparer_seam_policy_tests.cpp` | role-match |
| `tests/CMakeLists.txt` | test build config | build registration | `tests/CMakeLists.txt` | exact |
| `.planning/REQUIREMENTS.md` | planning status docs | documentation transform | `.planning/REQUIREMENTS.md` | exact |
| `.planning/PROJECT.md` | planning status docs | documentation transform | `.planning/PROJECT.md` | exact |
| `.planning/ROADMAP.md` | planning status docs | documentation transform | `.planning/ROADMAP.md` | exact |
| `.planning/STATE.md` | planning status docs | documentation transform | `.planning/STATE.md` | exact |

## Pattern Assignments

### `src/formats/bsa/tes4_bsa_layout.cpp` (format layout service, batch + file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_gnrl_layout.cpp`

**Imports pattern** (`src/formats/bsa/tes4_bsa_layout.cpp` lines 1-12):
```cpp
#include "formats/bsa/tes4_bsa_layout.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <detail/host_file.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>
```

**Existing TES4 exact-equality authority** (`src/formats/bsa/tes4_bsa_layout.cpp` lines 157-171):
```cpp
result<bool> tes4_stored_payloads_equal(const tes4_prepared_entry& lhs, const tes4_prepared_entry& rhs) {
  if (lhs.stored_size != rhs.stored_size) {
    return false;
  }
  if (!lhs.stream_raw_disk && !rhs.stream_raw_disk) {
    return lhs.stored_payload == rhs.stored_payload;
  }
  if (lhs.stream_raw_disk && rhs.stream_raw_disk) {
    return disk_stored_payloads_equal(lhs, rhs);
  }
  if (lhs.stream_raw_disk) {
    return disk_stored_payload_equals_bytes(lhs, rhs.stored_payload);
  }
  return disk_stored_payload_equals_bytes(rhs, lhs.stored_payload);
}
```

**Current TES4 dedupe hotspot to narrow, not replace** (`src/formats/bsa/tes4_bsa_layout.cpp` lines 218-249):
```cpp
  std::vector<assigned_payload> deduplicated_payloads;
  for (auto& folder : folders) {
    for (auto& entry : folder.entries) {
      if (deduplicate_payloads) {
        // Dedupe compares the complete final stored byte stream. Raw disk sources
        // are compared on demand so the publish path can still stream unique files.
        for (const auto& candidate : deduplicated_payloads) {
          auto duplicate = tes4_stored_payloads_equal(entry, *candidate.entry);
          if (!duplicate) {
            return duplicate.error();
          }
          if (duplicate.value()) {
            entry.payload_offset = candidate.assignment.offset;
            entry.stored_size = candidate.assignment.stored_size;
            entry.owns_payload_bytes = false;
            break;
          }
        }
        if (!entry.owns_payload_bytes) {
          continue;
        }
      }
```

**BA2 bucket pattern to copy conceptually** (`src/formats/ba2/ba2_gnrl_layout.cpp` lines 24-34, 110-150):
```cpp
struct dedupe_identity {
  std::uint32_t stored_size{};
  std::uint64_t hash{};

  bool operator<(const dedupe_identity& other) const noexcept {
    if (stored_size != other.stored_size) {
      return stored_size < other.stored_size;
    }
    return hash < other.hash;
  }
};
```
```cpp
  std::map<dedupe_identity, std::vector<payload_assignment>> deduplicated_payloads;
  for (std::size_t index = 0; index < entries.size(); ++index) {
    auto& entry = entries[index];
    auto stored_size = checked_u32(entry.stream_from_disk ? entry.raw_size : entry.stored_payload.size(),
                                   "BA2 GNRL stored payload size");
    if (!stored_size) {
      return stored_size.error();
    }
    if (deduplicate_payloads) {
      // D-23 requires dedupe after raw-vs-compressed routing, so this key is the exact byte span
      // the writer would store in the BA2 payload area rather than the caller's source bytes.
      const dedupe_identity identity{stored_size.value(), entry.payload_hash};
      auto duplicate = deduplicated_payloads.find(identity);
      bool reused_payload = false;
      if (duplicate != deduplicated_payloads.end()) {
        for (const auto& candidate : duplicate->second) {
          auto equal = ba2_gnrl_payloads_equal(entry, entries[candidate.entry_index]);
          if (!equal) {
            return equal.error();
          }
          if (equal.value()) {
            entry.payload_offset = candidate.offset;
            entry.owns_payload_bytes = false;
            reused_payload = true;
            break;
          }
        }
      }
```

**Error handling pattern**: return `result` errors immediately, never throw for caller data (`src/formats/bsa/tes4_bsa_layout.cpp` lines 176-179, 241-244):
```cpp
  auto layout = calculate_table_lengths(folders);
  if (!layout) {
    return layout.error();
  }
```
```cpp
      auto offset = checked_u32(payload_cursor, "TES4 BSA payload offset");
      if (!offset) {
        return offset.error();
      }
```

---

### `src/formats/ba2/ba2_gnrl_prepare.hpp` and `src/formats/ba2/ba2_gnrl_prepare.cpp` (model + preparation service, transform + file-I/O)

**Analog:** `src/formats/ba2/ba2_gnrl_prepare.hpp/.cpp`

**Prepared-entry model pattern** (`src/formats/ba2/ba2_gnrl_prepare.hpp` lines 17-34):
```cpp
struct ba2_gnrl_prepared_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::string source_path;
  // Raw finalization must reuse the prepare-time resolved path so Windows UTF-8 host text is not reinterpreted later.
  detail::host_file_path resolved_source_path;
  std::array<std::byte, 4> extension{};
  std::uint32_t name_hash{};
  std::uint32_t directory_hash{};
  std::uint32_t record_flags{};
  std::uint64_t payload_offset{};
  std::uint32_t packed_size{};
  std::uint32_t raw_size{};
  std::uint64_t payload_hash{};
  bool stream_from_disk{false};
  bool owns_payload_bytes{true};
  std::vector<std::byte> stored_payload;
};
```

**Hash/fingerprint helper pattern** (`src/formats/ba2/ba2_gnrl_prepare.cpp` lines 79-109):
```cpp
std::uint64_t hash_bytes(std::span<const std::byte> bytes) noexcept {
  std::uint64_t hash = 14695981039346656037ULL;
  for (const auto byte : bytes) {
    hash ^= std::to_integer<std::uint8_t>(byte);
    hash *= 1099511628211ULL;
  }
  return hash;
}

result<std::uint64_t> hash_disk_payload(const std::string& host_path, std::uint64_t expected_size) {
  std::uint64_t hash = 14695981039346656037ULL;
  auto source_path = resolve_ba2_gnrl_source_path(host_path);
  if (!source_path) {
    return source_path.error();
  }
  auto hashed = detail::for_each_host_file_chunk(
      source_path.value(),
      expected_size,
      ba2_gnrl_prepare_source_context,
      [&](std::span<const std::byte> chunk) -> result<void> {
        for (const auto byte : chunk) {
          hash ^= std::to_integer<std::uint8_t>(byte);
          hash *= 1099511628211ULL;
        }
        return {};
      });
```

**Preparation assignment pattern** (`src/formats/ba2/ba2_gnrl_prepare.cpp` lines 199-232, 253-267):
```cpp
  std::uint32_t packed_size = ba2_packed_size_raw;
  bool stream_from_disk = !entry.from_memory && !entry_compressed;
  std::uint64_t payload_hash = 0U;
  if (entry_compressed || entry.from_memory) {
    auto payload = read_source_bytes(entry, source_size);
    if (!payload) {
      return payload.error();
    }
```
```cpp
  return ba2_gnrl_prepared_entry{entry.archive_path_original,
                                 entry.archive_path_canonical,
                                 entry.host_path,
                                 std::move(resolved_source_path),
                                 extension.value(),
                                 detail::hash_fo4(file_name),
                                 detail::hash_fo4(directory),
                                 entry.options.record_flags.value_or(0U),
                                 0U,
                                 packed_size,
                                 raw_size.value(),
                                 payload_hash,
                                 stream_from_disk,
                                 true,
                                 std::move(stored_payload)};
```

**Apply to Phase 17:** Add any explicit BA2 GNRL staged identity/digest evidence in the prepared-entry model and fill it in `prepare_entry`; do not add external digest dependencies.

---

### `src/formats/ba2/ba2_gnrl_layout.cpp` (format layout service, batch + file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_gnrl_layout.cpp`

**Imports pattern** (`src/formats/ba2/ba2_gnrl_layout.cpp` lines 1-12):
```cpp
#include "formats/ba2/ba2_gnrl_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <vector>
```

**Exact equality authority** (`src/formats/ba2/ba2_gnrl_layout.cpp` lines 81-92):
```cpp
result<bool> ba2_gnrl_payloads_equal(const ba2_gnrl_prepared_entry& lhs, const ba2_gnrl_prepared_entry& rhs) {
  if (lhs.stream_from_disk && rhs.stream_from_disk) {
    return compare_disk_payloads(lhs.source_path, rhs.source_path, lhs.raw_size);
  }
  if (lhs.stream_from_disk) {
    return compare_disk_payload_to_bytes(lhs.source_path, rhs.stored_payload);
  }
  if (rhs.stream_from_disk) {
    return compare_disk_payload_to_bytes(rhs.source_path, lhs.stored_payload);
  }
  return lhs.stored_payload == rhs.stored_payload;
}
```

**Dedupe candidate bucket pattern to preserve and strengthen** (`src/formats/ba2/ba2_gnrl_layout.cpp` lines 118-150):
```cpp
    if (deduplicate_payloads) {
      // D-23 requires dedupe after raw-vs-compressed routing, so this key is the exact byte span
      // the writer would store in the BA2 payload area rather than the caller's source bytes.
      const dedupe_identity identity{stored_size.value(), entry.payload_hash};
      auto duplicate = deduplicated_payloads.find(identity);
      bool reused_payload = false;
      if (duplicate != deduplicated_payloads.end()) {
        for (const auto& candidate : duplicate->second) {
          auto equal = ba2_gnrl_payloads_equal(entry, entries[candidate.entry_index]);
          if (!equal) {
            return equal.error();
          }
          if (equal.value()) {
            entry.payload_offset = candidate.offset;
            entry.owns_payload_bytes = false;
            reused_payload = true;
            break;
          }
        }
      }
```

**Disk-source changed rejection pattern** (`src/formats/ba2/ba2_gnrl_layout.cpp` lines 184-192, 222-230):
```cpp
  // Dedupe decisions reuse earlier size/hash metadata; reject any source that no longer ends at that boundary.
  char extra = '\0';
  if (input.get(extra)) {
    return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
  }
  if (input.bad()) {
    return error{error_code::io_error, "BA2 GNRL writer failed while comparing disk source"};
  }
```
```cpp
  // A file that grew after preparation can otherwise compare equal for the prepared prefix and corrupt offsets.
  char lhs_extra = '\0';
  char rhs_extra = '\0';
  if (lhs.get(lhs_extra) || rhs.get(rhs_extra)) {
    return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
  }
```

---

### `src/formats/ba2/ba2_dx10_writer.cpp` (writer orchestration service, request-response + file-I/O + batch)

**Analog:** `src/formats/ba2/ba2_dx10_writer.cpp`

**Imports and state pattern** (`src/formats/ba2/ba2_dx10_writer.cpp` lines 1-17, 21-37):
```cpp
#include "formats/ba2/ba2_dx10_writer.hpp"

#include "formats/ba2/ba2_dx10_layout.hpp"
#include "formats/ba2/ba2_dx10_prepare.hpp"
#include "formats/ba2/ba2_dx10_serialize.hpp"

#include <detail/writer_publish.hpp>
#include <detail/host_file_path.hpp>
```
```cpp
struct ba2_dx10_writer::state {
  ba2_dx10_target target;
  ba2_dx10_writer_options options;
  std::vector<formats::ba2::ba2_dx10_writer_entry> entries;
  std::filesystem::path snapshot_dir;

  state(ba2_dx10_target selected_target, ba2_dx10_writer_options selected_options)
      : target(selected_target), options(selected_options) {}

  ~state() {
    std::error_code fs_error;
    // Snapshot cleanup is best-effort; write_to returns the primary result before state teardown runs.
    if (!snapshot_dir.empty()) {
      std::filesystem::remove_all(snapshot_dir, fs_error);
    }
  }
};
```

**Add-file snapshot reservation pattern** (`src/formats/ba2/ba2_dx10_writer.cpp` lines 54-72):
```cpp
result<void> ba2_dx10_writer::add_file(std::string_view archive_path, std::string_view dds_host_path) {
  if (dds_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 DX10 DDS source host path must not be empty"};
  }

  auto snapshot_dir = formats::ba2::ba2_dx10_ensure_snapshot_directory(state_->snapshot_dir);
  if (!snapshot_dir) {
    return snapshot_dir.error();
  }

  auto entry = formats::ba2::ba2_dx10_make_writer_entry(
      archive_path, dds_host_path, state_->target, state_->snapshot_dir, state_->entries.size());
  if (!entry) {
    return entry.error();
  }
```

**Write pipeline pattern** (`src/formats/ba2/ba2_dx10_writer.cpp` lines 79-85, 96-132):
```cpp
result<void> ba2_dx10_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "BA2 DX10 writer worker_count must be positive"};
  }
  return formats::ba2::write_ba2_dx10_archive(
      state_->target, state_->options, state_->entries, host_path, execution.worker_count);
}
```
```cpp
  auto validated = ba2_dx10_validate_entries(target, entries);
  if (!validated) {
    return validated.error();
  }

  const auto version = ba2_dx10_version_for(target);
  auto prepared = ba2_dx10_prepare_entries(target, options, entries, worker_count);
  if (!prepared) {
    return prepared.error();
  }

  std::uint64_t file_table_offset = 0;
  auto offsets =
      ba2_dx10_assign_payload_offsets(prepared.value(), version, options.deduplicate_payloads, file_table_offset);
```

**Apply to Phase 17:** Add `state` consumed tracking and cleanup helper here. Preserve result-returning invalid_argument behavior for later `add_file`/`write_to` calls and keep cleanup best-effort so primary errors survive.

---

### `src/formats/ba2/ba2_dx10_snapshot_builder.cpp` (snapshot builder service, file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`

**Windows-private RNG/temp reservation imports** (`src/formats/ba2/ba2_dx10_snapshot_builder.cpp` lines 1-18):
```cpp
#include "formats/ba2/ba2_dx10_snapshot_builder.hpp"

#include <detail/archive_path.hpp>
#include <detail/host_file.hpp>

#include "texture/directxtex_analyzer.hpp"

// BCrypt depends on Windows base types; keep both headers private and prevent min/max macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <bcrypt.h>
```

**Snapshot directory reservation pattern** (`src/formats/ba2/ba2_dx10_snapshot_builder.cpp` lines 91-113, 144-153):
```cpp
result<std::filesystem::path> make_unique_snapshot_directory() {
  std::error_code fs_error;
  const auto root = std::filesystem::temp_directory_path(fs_error);
  if (fs_error) {
    return error{error_code::io_error, "BA2 DX10 writer failed to locate snapshot temp root"};
  }

  for (std::uint32_t attempt = 0; attempt < 1024U; ++attempt) {
    auto suffix = make_snapshot_random_suffix();
    if (!suffix) {
      return suffix.error();
    }
    const auto candidate = root / ("libbsa-dx10-snapshot-" + suffix.value());
    fs_error.clear();
    // Directory creation remains the atomic reservation boundary; existing paths are collisions.
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate;
    }
```
```cpp
result<void> ba2_dx10_ensure_snapshot_directory(std::filesystem::path& snapshot_dir_path) {
  if (!snapshot_dir_path.empty()) {
    return {};
  }
  auto snapshot_dir = make_unique_snapshot_directory();
  if (!snapshot_dir) {
    return snapshot_dir.error();
  }
  snapshot_dir_path = std::move(snapshot_dir.value());
  return {};
}
```

**Snapshot-file creation error propagation** (`src/formats/ba2/ba2_dx10_snapshot_builder.cpp` lines 116-125, 185-196):
```cpp
result<void> write_snapshot_file(const std::filesystem::path& snapshot_path, std::span<const std::byte> bytes) {
  std::ofstream output{snapshot_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "BA2 DX10 writer failed to create snapshot temp file"};
  }
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    return error{error_code::io_error, "BA2 DX10 writer failed while writing snapshot temp file"};
  }
  return {};
}
```
```cpp
    auto written = write_snapshot_file(snapshot_path, subresource.bytes);
    if (!written) {
      return written.error();
    }
    entry.subresources.push_back(ba2_dx10_subresource_snapshot{
        subresource.array_index, subresource.face_index, subresource.mip, subresource.bytes.size(), std::move(snapshot_path)});
```

---

### `include/libbsa/writer.hpp` (public API contract / documentation, request-response)

**Analog:** `include/libbsa/writer.hpp`

**Public docs style** (`include/libbsa/writer.hpp` lines 397-406):
```cpp
/// Public writer for creating new BA2 DX10/DDS texture archives.
///
/// Entries are added from DDS host files with explicit archive-internal paths and
/// finalized to a host-path archive. The public DX10 contract is compressed-only
/// at archive level: callers do not choose raw, per-entry, or per-chunk overrides
/// because uncompressed texture archives are not a stable compatibility target.
///
/// Thread-safety: separately constructed or moved-to writer objects may be used
/// concurrently, but mutation is not concurrent with other mutation or
/// `write_to` on the same writer object. See `docs/thread-safety.md`.
class ba2_dx10_writer {
```

**Add-time snapshot documentation pattern** (`include/libbsa/writer.hpp` lines 441-445):
```cpp
  /// Adds a DDS host-file payload with an explicit archive-internal texture path.
  ///
  /// The DDS file is validated and snapshotted at add time; expected caller-data
  /// failures are reported through `result<void>`.
  LIBBSA_API result<void> add_file(std::string_view archive_path, std::string_view dds_host_path);
```

**Write-to documentation pattern** (`include/libbsa/writer.hpp` lines 447-459):
```cpp
  /// Finalizes the writer state into a new BA2 DX10 archive at `host_path`.
  ///
  /// Existing destinations fail unless `ba2_dx10_writer_options::overwrite_existing`
  /// was enabled. Publication uses a writer-owned temporary directory beside `host_path`,
  /// overwrites only supported regular-file destinations, rejects detectable reparse points,
  /// and returns validation, compression, or I/O failures as structured errors.
  LIBBSA_API result<void> write_to(std::string_view host_path) const;
```

**Apply to Phase 17:** If documenting consumed BA2 DX10 writer behavior in the public API, follow the tight Doxygen style above and do not change public signatures.

---

### `tests/unit/writer_hotspot_policy_tests.cpp` (source-policy test, file-I/O + static analysis)

**Analog:** `tests/unit/parser_preparer_seam_policy_tests.cpp`

**Imports and source-root helpers** (`tests/unit/parser_preparer_seam_policy_tests.cpp` lines 1-23):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <span>
#include <string>
#include <string_view>

namespace {

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.is_open());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}
```

**Reusable token assertion helpers** (`tests/unit/parser_preparer_seam_policy_tests.cpp` lines 24-48):
```cpp
std::string function_body(std::string_view source, std::string_view signature, std::string_view next_signature) {
  const auto start = source.find(signature);
  REQUIRE(start != std::string_view::npos);

  const auto body_start = source.find('{', start);
  REQUIRE(body_start != std::string_view::npos);

  const auto end = source.find(next_signature, body_start);
  REQUIRE(end != std::string_view::npos);
  return std::string{source.substr(body_start, end - body_start)};
}

void require_all_tokens(std::string_view text, std::span<const std::string_view> tokens) {
  for (const auto token : tokens) {
    INFO("missing token: " << token);
    REQUIRE(text.find(token) != std::string_view::npos);
  }
}
```

**Role-based policy test style** (`tests/unit/parser_preparer_seam_policy_tests.cpp` lines 106-137):
```cpp
TEST_CASE("parser_preparer_seam_policy requires dedicated BA2 DX10 preparer seams",
          "[unit][parser_preparer_seam_policy]") {
  const auto root = source_root();
  const auto prepare = read_text_file(root / "src/formats/ba2/ba2_dx10_prepare.cpp");
  const auto snapshot_header = read_text_file(root / "src/formats/ba2/ba2_dx10_snapshot_builder.hpp");
  const auto chunk_header = read_text_file(root / "src/formats/ba2/ba2_dx10_chunk_assembler.hpp");

  constexpr auto prepare_evidence = std::to_array<std::string_view>({
      "#include \"formats/ba2/ba2_dx10_snapshot_builder.hpp\"",
      "#include \"formats/ba2/ba2_dx10_chunk_assembler.hpp\"",
      "ba2_dx10_build_writer_entry_snapshot",
      "ba2_dx10_assemble_chunk",
      "ba2_dx10_assemble_planned_entry",
  });
  require_all_tokens(prepare, prepare_evidence);
```

**Apply to Phase 17:** Create a dedicated policy file rather than expanding unrelated policy suites. Assert semantic tokens: keyed TES4 bucket before `tes4_stored_payloads_equal`, explicit BA2 GNRL final-stored key/digest plus `ba2_gnrl_payloads_equal`, BA2 DX10 consumed-state + cleanup tokens, docs tokens for normal/failure/destructor/abnormal termination risk. Avoid freezing optional helper names unless names are introduced as deliberate contracts.

---

### Runtime writer tests (`tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`)

**Analogs:** same files

**TES4 dedupe runtime pattern** (`tests/unit/tes4_bsa_writer_tests.cpp` lines 867-892):
```cpp
TEST_CASE("TES4 BSA writer shares offsets for opt-in identical final stored bytes",
          "[unit][tes4_bsa_writer]") {
  const auto source_bytes = bytes_from_text("identical source bytes stored twice with opt-in dedupe");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.deduplicate_payloads = true;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};

  REQUIRE(writer.add_bytes("Meshes/Dedupe/First.nif", source_bytes).has_value());
  REQUIRE(writer.add_bytes("Meshes/Dedupe/Second.nif", source_bytes).has_value());

  const auto archive = output_path("dedupe-enabled-shared-offsets.bsa");
  auto written = writer.write_to(archive.string());
  REQUIRE(written.has_value());
```

**TES4 mismatch regression pattern** (`tests/unit/tes4_bsa_writer_tests.cpp` lines 926-954):
```cpp
TEST_CASE("TES4 BSA writer does not dedupe matching source bytes with different compression encodings",
          "[unit][tes4_bsa_writer]") {
  const auto source_bytes = bytes_from_text("same source but raw and compressed stored bytes differ");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.deduplicate_payloads = true;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
```

**BA2 GNRL disk-source rejection pattern** (`tests/unit/ba2_gnrl_writer_tests.cpp` lines 148-191):
```cpp
TEST_CASE("BA2 GNRL dedupe disk comparisons reject source size changes",
          "[unit][ba2_gnrl_writer][bounded_memory_policy][dedupe]") {
  const std::vector<std::byte> expected{std::byte{0x44}, std::byte{0x45}, std::byte{0x44}, std::byte{0x55}};

  SECTION("disk-to-memory source grows beyond the prepared payload") {
    auto grown = expected;
    grown.push_back(std::byte{0x50});
    const auto source = output_path("dedupe-source-grew.bin");
    write_binary_file(source, grown);

    auto equal = libbsa::formats::ba2::ba2_gnrl_payloads_equal(
        disk_stage_entry(source, static_cast<std::uint32_t>(expected.size())), memory_stage_entry(expected));

    REQUIRE_FALSE(equal.has_value());
    CHECK(equal.error().code == libbsa::error_code::io_error);
  }
```

**BA2 GNRL dedupe runtime pattern** (`tests/unit/ba2_gnrl_writer_tests.cpp` lines 820-848):
```cpp
TEST_CASE("BA2 GNRL writer shares offsets for byte-identical stored payloads when dedupe is enabled",
          "[unit][ba2_gnrl_writer]") {
  auto options = overwriting_raw_options();
  options.deduplicate_payloads = true;
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
  const std::vector<std::byte> bytes{std::byte{0x53}, std::byte{0x48}, std::byte{0x41}, std::byte{0x52},
                                     std::byte{0x45}, std::byte{0x44}};
```

**BA2 DX10 snapshot-directory discovery pattern** (`tests/unit/ba2_dx10_writer_tests.cpp` lines 84-85, 120-135):
```cpp
constexpr std::string_view snapshot_directory_prefix = "libbsa-dx10-snapshot-";
```
```cpp
/// Lists BA2 DX10 snapshot temp directories without querying metadata for unrelated temp entries.
std::set<std::filesystem::path> snapshot_directories() {
  std::set<std::filesystem::path> paths;
  for (const auto& entry : std::filesystem::directory_iterator{std::filesystem::temp_directory_path()}) {
    const auto name = entry.path().filename().string();
    if (name.rfind(snapshot_directory_prefix, 0U) != 0U) {
      continue;
    }

    std::error_code fs_error;
    if (entry.is_directory(fs_error)) {
      paths.insert(entry.path());
    }
  }
  return paths;
}
```

**BA2 DX10 teardown cleanup test to extend for immediate cleanup** (`tests/unit/ba2_dx10_writer_tests.cpp` lines 752-786):
```cpp
TEST_CASE("BA2 DX10 writer state removes snapshot temp directory on teardown",
          "[unit][ba2_dx10_writer][bounded_memory_policy][cleanup]") {
  const auto before = snapshot_directories();
  std::set<std::filesystem::path> staged_snapshot_dirs;

  {
    const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
    const auto& source_case = valid_source_case(manifest, "bc1_unorm");
    libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
```

**BA2 DX10 publish failure pattern** (`tests/unit/ba2_dx10_writer_tests.cpp` lines 897-915):
```cpp
TEST_CASE("BA2 DX10 writer refuses to overwrite existing output by default and preserves bytes",
          "[unit][ba2_dx10_writer][publish][overwrite]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto output = writer_test_dir() / "dx10-overwrite-default.ba2";
  const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
  write_binary_file(output, sentinel);
```

---

### `tests/CMakeLists.txt` (test build config, build registration)

**Analog:** `tests/CMakeLists.txt`

**Add new test source in the main Catch2 executable list** (`tests/CMakeLists.txt` lines 65-116):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/archive_reader_dispatch_tests.cpp
  unit/archive_reader_dispatch_policy_tests.cpp
  unit/bulk_extraction_tests.cpp
  unit/writer_execution_options_tests.cpp
  unit/bsa_writer_execution_tests.cpp
  unit/ba2_writer_execution_tests.cpp
  unit/writer_ownership_tests.cpp
  unit/writer_stage_tests.cpp
  unit/bounded_memory_policy_tests.cpp
```

**Catch2 discovery / labels pattern** (`tests/CMakeLists.txt` lines 300-305):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Release package-proof registration pattern** (`tests/CMakeLists.txt` lines 307-330):
```cmake
add_test(
  NAME package_consumer_smoke
  COMMAND ${CMAKE_COMMAND}
    -DLIBBSA_BUILD_DIR=${CMAKE_BINARY_DIR}
    -DLIBBSA_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/package-consumer-prefix
    -DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/package-consumer
    -DCONSUMER_BUILD_DIR=${CMAKE_BINARY_DIR}/package-consumer-build
    -DCONFIG=$<CONFIG>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/package-consumer/smoke.cmake
)
```

---

### Docs and planning files (`docs/target-format-guide.md`, `docs/integration-examples.md`, `.planning/*.md`)

**Analogs:** same files plus `tests/unit/validation_policy_tests.cpp`

**Target-format writer publication docs pattern** (`docs/target-format-guide.md` lines 66-72):
```markdown
## writer output publication safety

Public `write_to` calls serialize completed archives into a writer-owned temporary directory beside the requested destination, then publish the completed file to the final host path. This same-directory temporary output keeps normal local-filesystem publication on the destination volume and avoids exposing a partial archive at the destination path.

When `overwrite_existing` is false, libbsa uses no-overwrite publication: the call fails if the destination exists before writing or appears before final publication. When `overwrite_existing` is true, the existing destination is expected to be a regular file. Existing directories, other non-regular paths, and detectable Windows reparse point destinations are refused before replacement when the shared writer publish helper can identify them.

Publication relies on Windows host-filesystem rename/replace behavior. Local NTFS paths are the intended baseline; a network filesystem, reparse-point provider, or other filesystem redirector can expose provider-specific atomicity, durability, or permission failures. Writer publication failures are returned as `io_error` results with the writer-specific diagnostic prefix, and libbsa cleans writer-owned temporary output on a best-effort basis.
```

**Integration example docs style** (`docs/integration-examples.md` lines 53-60):
```markdown
## `example_create_ba2_dx10`

```cpp
libbsa::result<void> example_create_ba2_dx10(std::string_view dds_host_path,
                                             std::string_view output_host_path);
```

Create a `ba2_dx10_writer` for a texture target and add DDS host files with archive virtual texture paths. The writer validates and snapshots DDS input at add time, then `write_to` uses target metadata to select the compressed BA2 DX10 route.
```

**Planning requirement status pattern** (`.planning/REQUIREMENTS.md` lines 32-40, 72-87):
```markdown
### Dedupe Hotspot Cleanup

- [ ] **DEDU-01**: Consumer can write TES4-family archives with dedupe enabled using faster candidate narrowing while preserving exact stored-byte equality behavior.
- [ ] **DEDU-02**: Consumer can write BA2 GNRL archives with dedupe enabled using stronger staged identity or digest narrowing while preserving exact stored-byte equality fallback behavior.

### DX10 Temp-Staging Cleanup

- [ ] **DX10-01**: Consumer can write BA2 DX10 archives with temporary staging data cleaned up during normal write completion and ordinary failure unwinding.
- [ ] **DX10-02**: Maintainer can verify and document the remaining BA2 DX10 temporary-data lifecycle behavior, including any residual abnormal-termination risk.
```

**Roadmap phase closure pattern** (`.planning/ROADMAP.md` lines 123-132, 136-142):
```markdown
### Phase 17: Writer Hotspot Hardening and Ship Gate
**Goal**: The remaining high-cost and fragile writer staging paths are hardened under existing semantics, and the milestone closes with verified ship-ready evidence rather than a broad redesign.
**Depends on**: Phase 16
**Requirements**: DEDU-01, DEDU-02, DX10-01, DX10-02
**Success Criteria** (what must be TRUE):
  1. Consumer can write TES4-family archives with dedupe enabled using faster candidate narrowing while exact stored-byte equality behavior stays unchanged.
  2. Consumer can write BA2 GNRL archives with dedupe enabled using stronger staged identity or digest narrowing while exact stored-byte equality fallback behavior stays unchanged.
  3. Consumer can write BA2 DX10 archives with temporary staging data cleaned up during normal completion and ordinary failure unwinding.
  4. Maintainer can verify and document the remaining BA2 DX10 temporary-data lifecycle behavior, including any residual abnormal-termination risk, before shipping the milestone.
```

**Planning-policy test pattern** (`tests/unit/validation_policy_tests.cpp` lines 399-424):
```cpp
TEST_CASE("validation_policy verification matrix contract keeps planning summaries truthful",
          "[unit][validation_policy][doc_structure]") {
  const auto root = source_root();
  const auto project = read_text_file(root / ".planning/PROJECT.md");
  const auto roadmap = read_text_file(root / ".planning/ROADMAP.md");
  const auto state = read_text_file(root / ".planning/STATE.md");

  require_all_tokens(project,
                     {"debug",
                      "Release package-proof lanes",
                      "MSVC AddressSanitizer hardening lane",
                      "Windows-only"});
```

## Shared Patterns

### Result/error contract
**Source:** `src/formats/ba2/ba2_dx10_writer.cpp`, `src/detail/writer_publish.hpp`
**Apply to:** all writer implementation changes
```cpp
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "BA2 DX10 writer worker_count must be positive"};
  }
```
```cpp
  auto written = std::forward<WriteTemp>(write_temp)(temp_path);
  if (!written) {
    cleanup_writer_publish_directory(temp_dir.value());
    return written.error();
  }
```

### Best-effort cleanup preserves primary errors
**Source:** `src/detail/writer_publish.cpp` lines 115-119 and `src/detail/writer_publish.hpp` lines 45-58
**Apply to:** BA2 DX10 snapshot cleanup on success/failure
```cpp
void cleanup_writer_publish_directory(const std::filesystem::path& temp_dir) noexcept {
  std::error_code fs_error;
  // Cleanup is best-effort so callers see the primary writer or publish failure.
  std::filesystem::remove_all(temp_dir, fs_error);
}
```
```cpp
  if (!written) {
    cleanup_writer_publish_directory(temp_dir.value());
    return written.error();
  }
```

### Exact equality remains the dedupe authority
**Source:** `src/formats/bsa/tes4_bsa_layout.cpp` and `src/formats/ba2/ba2_gnrl_layout.cpp`
**Apply to:** TES4 and BA2 GNRL dedupe narrowing
```cpp
auto duplicate = tes4_stored_payloads_equal(entry, *candidate.entry);
if (!duplicate) {
  return duplicate.error();
}
if (duplicate.value()) {
  entry.payload_offset = candidate.assignment.offset;
```
```cpp
auto equal = ba2_gnrl_payloads_equal(entry, entries[candidate.entry_index]);
if (!equal) {
  return equal.error();
}
if (equal.value()) {
  entry.payload_offset = candidate.offset;
```

### Windows host-file boundary for disk-backed writer paths
**Source:** `src/formats/bsa/tes4_bsa_layout.cpp` lines 50-60 and `src/formats/ba2/ba2_gnrl_prepare.cpp` lines 44-54
**Apply to:** any new disk-read/digest helper
```cpp
constexpr detail::host_file_context tes4_dedupe_source_context{
    "TES4 BSA writer failed to open disk source",
    "TES4 BSA writer failed to inspect disk source",
    "TES4 BSA writer failed while reading disk source",
    "TES4 BSA disk source changed during dedupe preparation",
    "TES4 BSA disk source"};

/// Resolves TES4 dedupe disk sources once so equality checks reuse the shared host-file contract.
result<detail::host_file_path> resolve_tes4_dedupe_source_path(std::string_view host_path) {
  return detail::resolve_host_file_path(host_path);
}
```

### Source-policy tests should assert roles, not incidental names
**Source:** `tests/unit/parser_preparer_seam_policy_tests.cpp`
**Apply to:** `tests/unit/writer_hotspot_policy_tests.cpp`
```cpp
constexpr auto snapshot_role_evidence = std::to_array<std::string_view>({
    "ba2_dx10_validate_texture_format_for_target",
    "ba2_dx10_ensure_snapshot_directory",
    "ba2_dx10_build_writer_entry_snapshot",
    "loading, validating, and snapshotting DDS source bytes",
    "Snapshot files intentionally outlive the source path",
});
require_all_tokens(snapshot_header, snapshot_role_evidence);
```

## No Analog Found

No files lack an analog. `tests/unit/writer_hotspot_policy_tests.cpp` is new, but `tests/unit/parser_preparer_seam_policy_tests.cpp`, `tests/unit/bounded_memory_policy_tests.cpp`, and `tests/unit/validation_policy_tests.cpp` provide strong source-policy analogs.

## Metadata

**Analog search scope:** `src/formats/bsa`, `src/formats/ba2`, `src/detail`, `include/libbsa`, `tests/unit`, `tests/CMakeLists.txt`, `docs`, `.planning`
**Files scanned:** 22 direct reads, plus phase context/research/spec and project instructions
**Pattern extraction date:** 2026-05-14
