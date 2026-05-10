# Phase 12: performance-concurrency-documentation-and-polish - Pattern Map

**Mapped:** 2026-05-10
**Files analyzed:** 34
**Analogs found:** 31 / 34

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/archive.hpp` | public API | request-response, file-I/O | `include/libbsa/archive.hpp` | exact |
| `include/libbsa/writer.hpp` | public API | file-I/O, batch | `include/libbsa/writer.hpp` | exact |
| `include/libbsa/libbsa.hpp` | public API umbrella | request-response | `include/libbsa/libbsa.hpp` | exact |
| `src/archive.cpp` | facade/controller | request-response, file-I/O, batch | `src/archive.cpp` | exact |
| `src/detail/parallel_work.hpp` | utility | batch, event-driven | None; borrow `src/detail/payload_stream.hpp` style | none |
| `src/detail/parallel_work.cpp` | utility | batch, event-driven | None; borrow `src/detail/payload_stream.cpp` style | none |
| `src/detail/payload_stream.hpp` | utility | file-I/O, streaming | `src/detail/payload_stream.hpp` | exact |
| `src/detail/payload_stream.cpp` | utility | file-I/O, streaming | `src/detail/payload_stream.cpp` | exact |
| `src/formats/bsa/tes3_bsa_writer.cpp` | writer/service | file-I/O, batch | `src/formats/bsa/tes3_bsa_writer.cpp` | exact |
| `src/formats/bsa/tes4_bsa_writer.cpp` | writer/service | file-I/O, batch | `src/formats/bsa/tes4_bsa_writer.cpp` | exact |
| `src/formats/ba2/ba2_gnrl_writer.cpp` | writer/service | file-I/O, batch | `src/formats/ba2/ba2_gnrl_writer.cpp` | exact |
| `src/formats/ba2/ba2_dx10_writer.cpp` | writer/service | file-I/O, batch | `src/formats/ba2/ba2_dx10_writer.cpp` | exact |
| `src/formats/ba2/ba2_publish.hpp` | utility | file-I/O, rollback | `src/formats/ba2/ba2_publish.hpp` | exact |
| `src/detail/atomic_file_ops.hpp` | utility | file-I/O, publish | `src/detail/atomic_file_ops.hpp` | exact |
| `tests/unit/bulk_extraction_tests.cpp` | test | request-response, file-I/O, batch | `tests/unit/archive_reader_tests.cpp` | role-match |
| `tests/unit/parallel_writer_tests.cpp` | test | file-I/O, batch | `tests/unit/tes4_bsa_writer_tests.cpp` | role-match |
| `tests/unit/bounded_memory_policy_tests.cpp` | test | transform, policy | `tests/unit/validation_policy_tests.cpp` | role-match |
| `tests/unit/thread_safety_docs_policy_tests.cpp` | test | transform, policy | `tests/unit/validation_policy_tests.cpp` | role-match |
| `tests/unit/docs_policy_tests.cpp` | test | transform, policy | `tests/unit/validation_policy_tests.cpp` | role-match |
| `tests/unit/public_include_boundary_tests.cpp` | test | compile-time contract | `tests/unit/public_include_boundary_tests.cpp` | exact |
| `tests/unit/payload_stream_tests.cpp` | test | streaming, file-I/O | `tests/unit/payload_stream_tests.cpp` | exact |
| `tests/CMakeLists.txt` | config | build orchestration | `tests/CMakeLists.txt` | exact |
| `CMakeLists.txt` | config | build orchestration | `CMakeLists.txt` | exact |
| `CMakePresets.json` | config | build orchestration | `CMakePresets.json` | partial |
| `tests/package-consumer/main.cpp` | test/example | request-response | `tests/package-consumer/main.cpp` | exact |
| `tests/package-consumer/smoke.cmake` | config/test harness | build orchestration | `tests/package-consumer/smoke.cmake` | exact |
| `benchmarks/libbsa_benchmarks.cpp` | benchmark tool | batch, file-I/O | `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` | partial |
| `benchmarks/README.md` | documentation | batch guidance | `tests/fixtures/README.md` | role-match |
| `docs/Doxyfile.in` | config | docs generation | None; use research plus `CMakeLists.txt` target style | none |
| `docs/api-mainpage.md` | documentation | transform | `docs/compatibility-evidence.md` | role-match |
| `docs/thread-safety.md` | documentation | event-driven, request-response | `docs/compatibility-evidence.md` | role-match |
| `docs/integration-examples.md` | documentation | request-response, file-I/O | `tests/package-consumer/main.cpp` | role-match |
| `docs/target-format-guide.md` | documentation | transform | `docs/compatibility-evidence.md` | role-match |
| `tests/fixtures/README.md` | documentation | fixture policy | `tests/fixtures/README.md` | exact |

## Pattern Assignments

### `include/libbsa/archive.hpp` (public API, request-response/file-I/O)

**Analog:** `include/libbsa/archive.hpp`

**Imports pattern** (lines 1-12):
```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <libbsa/result.hpp>
```

**Public sink contract to preserve** (lines 158-168):
```cpp
/// Synchronous sink used by archive extraction APIs.
///
/// Implementations must return the number of bytes accepted from `bytes`. The
/// extractor treats partial acceptance as `error_code::io_error` so callers
/// never observe ambiguous partial-success extraction.
class payload_sink {
 public:
  virtual ~payload_sink() = default;

  /// Writes a chunk of payload bytes and returns the accepted byte count.
  virtual result<std::size_t> write(std::span<const std::byte> bytes) = 0;
};
```

**Reader member API shape** (lines 171-207):
```cpp
/// Public archive reader for supported Bethesda archive files.
///
/// Use `open()` for fallible construction. A successfully opened reader exposes
/// archive metadata, deterministic entry listings, canonical path lookup, and
/// synchronous extraction for the archive variants implemented by libbsa.
class archive_reader {
 public:
  /// Attempts to open an archive from a host path string.
  static result<archive_reader> open(std::string_view host_path);

  [[nodiscard]] result<archive_metadata> metadata() const;
  [[nodiscard]] result<std::vector<entry_metadata>> entries() const;
  [[nodiscard]] result<std::optional<entry_metadata>> find(std::string_view path) const;
  [[nodiscard]] result<bool> contains(std::string_view path) const;
  [[nodiscard]] result<void> extract(std::string_view path, payload_sink& sink) const;
  [[nodiscard]] result<std::vector<std::byte>> extract_bytes(std::string_view path) const;
```

**Apply to Phase 12:** Add dependency-light bulk extraction option/request/result structs near `payload_sink`/`archive_reader`. Use Doxygen comments. Use `std::function` only if planner accepts the header dependency; otherwise prefer a small abstract sink-factory interface or templated public overload. Do not expose threads, private readers, private codecs, DirectXTex, LZ4, libdeflate, or TES5Edit types.

---

### `include/libbsa/writer.hpp` (public API, file-I/O/batch)

**Analog:** `include/libbsa/writer.hpp`

**Options struct pattern** (lines 64-82):
```cpp
/// Options controlling TES4-family write-new archive finalization.
struct tes4_bsa_writer_options {
  /// Archive-wide compression behavior used by entries whose policy is `inherit`.
  archive_compression_policy compression_policy{archive_compression_policy::target_default};

  /// Emits target-compatible embedded file-name payload prefixes when true.
  bool embed_file_names{false};

  /// Shares identical stored payload regions only when explicitly enabled.
  bool deduplicate_payloads{false};

  /// Allows `write_to` to replace an existing host-path archive when true.
  bool overwrite_existing{false};
};

/// Options controlling TES3/Morrowind write-new archive finalization.
struct tes3_bsa_writer_options {
  /// Allows `write_to` to replace an existing host-path archive when true.
  bool overwrite_existing{false};
};
```

**Writer finalization method pattern** (lines 188-192):
```cpp
/// Finalizes the writer state into a new archive at `host_path`.
///
/// Existing destinations fail unless `tes4_bsa_writer_options::overwrite_existing`
/// was enabled, and compression or I/O failures are returned as structured errors.
result<void> write_to(std::string_view host_path) const;
```

**BA2 writer overload shape** (lines 282-287):
```cpp
/// Finalizes the writer state into a new BA2 GNRL archive at `host_path`.
///
/// Existing destinations fail unless `ba2_gnrl_writer_options::overwrite_existing`
/// was enabled, and validation, compression, or I/O failures are returned as
/// structured errors.
result<void> write_to(std::string_view host_path) const;
```

**Apply to Phase 12:** Add one shared `write_execution_options`/`write_finalization_options` public struct with `std::uint32_t worker_count{1}`. Add overloads such as `write_to(std::string_view, write_execution_options) const` for TES3, TES4, BA2 GNRL, and BA2 DX10 while preserving the existing one-argument overload as the serial default.

---

### `include/libbsa/libbsa.hpp` (public umbrella)

**Analog:** `include/libbsa/libbsa.hpp`

**Umbrella include pattern** (lines 1-7):
```cpp
#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>
#include <libbsa/validation.hpp>
#include <libbsa/version.hpp>
#include <libbsa/writer.hpp>
```

**Apply to Phase 12:** If all new public bulk/parallel types live in existing `archive.hpp` and `writer.hpp`, no include addition is needed. If a new public header is introduced, add it here and to the public file set in `CMakeLists.txt`.

---

### `src/archive.cpp` (reader facade/controller, request-response/batch)

**Analog:** `src/archive.cpp`

**Imports pattern** (lines 1-21):
```cpp
#include <libbsa/archive.hpp>

#include "formats/ba2/ba2_format_detector.hpp"
#include "formats/ba2/ba2_dx10_parser.hpp"
#include "formats/ba2/ba2_dx10_reader.hpp"
#include "formats/ba2/ba2_gnrl_parser.hpp"
#include "formats/ba2/ba2_gnrl_reader.hpp"
#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes3_bsa_parser.hpp"
#include "formats/bsa/tes3_bsa_reader.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_reader.hpp"

#include <detail/byte_vector.hpp>

#include <cstddef>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>
```

**Single-entry extraction dispatch** (lines 222-245):
```cpp
result<void> archive_reader::extract(std::string_view path, payload_sink& sink) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  auto found = state_->metadata.variant == archive_variant::tes3
                    ? formats::bsa::find_tes3_bsa_entry(state_->entries, path)
                    : state_->metadata.type == archive_type::ba2
                          ? (state_->is_ba2_dx10 ? formats::ba2::find_ba2_dx10_entry(state_->entries, path)
                                                 : formats::ba2::find_ba2_gnrl_entry(state_->entries, path))
                          : formats::bsa::find_tes4_bsa_entry(state_->entries, path);
  if (!found) {
    return found.error();
  }
  if (!found.value()) {
    return error{error_code::not_found, "archive path was not found"};
  }
  if (state_->metadata.variant == archive_variant::tes3) {
    return formats::bsa::extract_tes3_bsa_payload(state_->host_path, *found.value(), sink);
  }
  if (state_->metadata.type == archive_type::ba2) {
    return state_->is_ba2_dx10 ? formats::ba2::extract_ba2_dx10_payload(state_->host_path, *found.value(), sink)
                               : formats::ba2::extract_ba2_gnrl_payload(state_->host_path, *found.value(), sink);
  }
  return formats::bsa::extract_tes4_bsa_payload_from_file(state_->host_path, *found.value(), sink);
}
```

**Bounded convenience extraction pattern** (lines 248-267):
```cpp
result<std::vector<std::byte>> archive_reader::extract_bytes(std::string_view path) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }

  auto found = find(path);
  if (!found) {
    return found.error();
  }
  if (!found.value()) {
    return error{error_code::not_found, "archive path was not found"};
  }

  // Keep the convenience API bounded by the parser-derived size for exactly one entry.
  vector_payload_sink sink{found.value()->raw_size};
  auto extracted = extract(path, sink);
  if (!extracted) {
    return extracted.error();
  }
  return std::move(sink).finish();
}
```

**Apply to Phase 12:** Implement `extract_entries` by reusing `find`/format extraction rather than duplicating parser routing. Fill output records by request/archive order, not completion order. Per-entry failures become per-entry result records; setup failures such as closed reader or invalid `worker_count` should fail the outer `result`.

---

### `src/detail/parallel_work.hpp` and `src/detail/parallel_work.cpp` (utility, batch/event-driven)

**Analog:** `src/detail/payload_stream.hpp` and `src/detail/payload_stream.cpp`

**Internal utility header style** (`src/detail/payload_stream.hpp` lines 1-8):
```cpp
#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <span>

namespace libbsa::detail {
```

**Small result-returning helper contract** (`src/detail/payload_stream.hpp` lines 31-35):
```cpp
/// Copies all remaining payload bytes using bounded chunks.
///
/// Partial sink acceptance is reported as `io_error` so callers never observe an
/// ambiguous partial-success extraction or packing operation.
result<void> transfer_payload(payload_source& source, payload_sink& sink, std::size_t chunk_size);
```

**Result/error implementation style** (`src/detail/payload_stream.cpp` lines 8-38):
```cpp
result<void> transfer_payload(payload_source& source, payload_sink& sink, std::size_t chunk_size) {
  if (chunk_size == 0) {
    return libbsa::error{libbsa::error_code::invalid_argument, "payload chunk size must be nonzero"};
  }

  std::vector<std::byte> scratch(chunk_size);
  while (source.remaining() > 0) {
    const auto requested = std::min(chunk_size, source.remaining());
    auto read = source.read(std::span<std::byte>{scratch.data(), requested});
    if (!read) {
      return read.error();
    }
    if (read.value() > requested) {
      return libbsa::error{libbsa::error_code::io_error, "payload source exceeded requested chunk size"};
    }
    if (read.value() == 0) {
      return libbsa::error{libbsa::error_code::io_error, "payload source made no progress"};
    }

    const auto bytes = std::span<const std::byte>{scratch.data(), read.value()};
    auto written = sink.write(bytes);
    if (!written) {
      return written.error();
    }
    if (written.value() != bytes.size()) {
      return libbsa::error{libbsa::error_code::io_error, "payload sink accepted a partial chunk"};
    }
  }

  return {};
}
```

**Apply to Phase 12:** If adding a worker helper, keep it in `src/detail`, return `result<void>` or aggregate caller-owned per-index records, validate `worker_count != 0`, and avoid global thread pools. Use deterministic index assignment and store results by original index.

---

### `src/detail/payload_stream.hpp` and `src/detail/payload_stream.cpp` (utility, streaming file-I/O)

**Analog:** same files.

**Bounded source/sink abstractions** (`src/detail/payload_stream.hpp` lines 10-29):
```cpp
/// Bounded synchronous source for archive payload bytes.
class payload_source {
 public:
  virtual ~payload_source() = default;

  /// Reads up to `destination.size()` bytes and returns the number produced.
  virtual result<std::size_t> read(std::span<std::byte> destination) = 0;

  /// Returns the exact number of bytes still available from this payload.
  [[nodiscard]] virtual std::size_t remaining() const noexcept = 0;
};

/// Bounded synchronous sink for archive payload bytes.
class payload_sink {
 public:
  virtual ~payload_sink() = default;

  /// Writes `bytes` and returns the number accepted by the sink.
  virtual result<std::size_t> write(std::span<const std::byte> bytes) = 0;
};
```

**Apply to Phase 12:** Prefer extending this helper family for disk-backed writer streaming. Add file-backed source and output sink helpers here only if they stay generic and do not introduce format-specific state.

---

### Writer implementation files (writer/service, file-I/O/batch)

**Analogs:**
- `src/formats/bsa/tes3_bsa_writer.cpp`
- `src/formats/bsa/tes4_bsa_writer.cpp`
- `src/formats/ba2/ba2_gnrl_writer.cpp`
- `src/formats/ba2/ba2_dx10_writer.cpp`

**Current disk-source whole-vector anti-pattern to replace** (`tes3_bsa_writer.cpp` lines 161-180):
```cpp
result<std::vector<std::byte>> read_source_bytes(const tes3_writer_entry& entry) {
  if (entry.from_memory) {
    return entry.memory_bytes;
  }

  // Disk sources stay path-backed until finalization so repeated write_to calls
  // can observe the current source bytes without making add_file consume I/O.
  std::ifstream input{entry.host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "TES3 BSA writer failed to open disk source"};
  }
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  if (input.bad()) {
    return error{error_code::io_error, "TES3 BSA writer failed while reading disk source"};
  }
  return bytes;
}
```

**Current final whole-archive byte-vector anti-pattern to replace** (`tes4_bsa_writer.cpp` lines 522-604):
```cpp
result<void> write_archive_bytes(std::span<const prepared_folder> folders,
                                 std::uint32_t version,
                                 std::uint32_t archive_flags,
                                 std::uint32_t file_flags,
                                 std::uint32_t total_folder_name_length,
                                 std::uint32_t total_file_name_length,
                                 std::uint32_t file_count,
                                 const std::filesystem::path& output_path) {
  detail::binary_writer writer;
  const std::byte magic[] = {std::byte{'B'}, std::byte{'S'}, std::byte{'A'}, std::byte{0}};
  auto written = writer.write_bytes(magic);
  ...
  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "TES4 BSA writer failed to create temporary output"};
  }
  const auto bytes = writer.bytes();
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    return error{error_code::io_error, "TES4 BSA writer failed while writing temporary output"};
  }
  return {};
}
```

**Deterministic preparation/order pattern** (`ba2_gnrl_writer.cpp` lines 267-333):
```cpp
result<std::vector<prepared_entry>> prepare_entries(ba2_gnrl_target target,
                                                    const ba2_gnrl_writer_options& options,
                                                    std::span<const ba2_gnrl_writer_entry> entries) {
  const bool archive_compressed = archive_default_compressed(options.compression);
  std::vector<prepared_entry> prepared;
  prepared.reserve(entries.size());

  for (const auto& entry : entries) {
    auto payload = read_source_bytes(entry);
    if (!payload) {
      return payload.error();
    }
    ...
    prepared.push_back(prepared_entry{entry.archive_path_original,
                                      entry.archive_path_canonical,
                                      extension.value(),
                                      detail::hash_fo4(file_name),
                                      detail::hash_fo4(directory),
                                      entry.options.record_flags.value_or(0U),
                                      0U,
                                      packed_size,
                                      raw_size.value(),
                                      true,
                                      std::move(stored_payload)});
  }

  // D-17 keeps Phase 8 deterministic with a canonical-path fallback because traced BA2 writer evidence does not
  // prove a stricter hash sort requirement. Records and final filename-table entries stay paired by this order.
  std::sort(prepared.begin(), prepared.end(), [](const prepared_entry& lhs, const prepared_entry& rhs) {
    return lhs.archive_path_canonical < rhs.archive_path_canonical;
  });
  return prepared;
}
```

**Payload offset/dedupe pattern** (`ba2_dx10_writer.cpp` lines 426-477):
```cpp
result<void> assign_payload_offsets(std::span<prepared_entry> entries,
                                    std::uint32_t version,
                                    bool deduplicate_payloads,
                                    std::uint64_t& file_table_offset) {
  std::uint64_t record_bytes = 0;
  ...
  std::map<dedupe_key, payload_assignment> deduplicated_payloads;
  for (auto& entry : entries) {
    for (auto& chunk : entry.chunks) {
      // D-20 requires DX10 dedupe to prove both byte identity and chunk metadata identity. Two
      // chunks only share storage when the final stored bytes, raw size, packed size, and explicit
      // compression route all match; texture dimensions and mip identity remain separate records.
      dedupe_key key{chunk.stored_payload, chunk.raw_size, chunk.packed_size, chunk.compression};
      if (deduplicate_payloads) {
        const auto duplicate = deduplicated_payloads.find(key);
        if (duplicate != deduplicated_payloads.end()) {
          chunk.payload_offset = duplicate->second.offset;
          chunk.owns_payload_bytes = false;
          continue;
        }
      }
      chunk.payload_offset = cursor;
      chunk.owns_payload_bytes = true;
      ...
    }
  }
  return {};
}
```

**Safe publish pattern to preserve** (`tes3_bsa_writer.cpp` lines 380-466):
```cpp
result<void> write_tes3_bsa_archive(const tes3_bsa_writer_options& options,
                                    std::span<const tes3_writer_entry> entries,
                                    std::string_view output_host_path) {
  auto validated_host_path = validate_host_path(output_host_path, "TES3 BSA output host path");
  if (!validated_host_path) {
    return validated_host_path.error();
  }
  ...
  auto temp_dir = make_unique_publish_directory(output_path);
  if (!temp_dir) {
    return temp_dir.error();
  }
  const auto temp_path = temp_dir.value() / output_path.filename();

  auto written = write_archive_bytes(prepared.value(), temp_path);
  if (!written) {
    cleanup_publish_directory(temp_dir.value());
    return written.error();
  }
  ...
  auto published = detail::publish_file_without_replace(temp_path, output_path);
  if (!published) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "TES3 BSA writer failed to publish output host path"};
  }
  cleanup_publish_directory(temp_dir.value());
  return {};
}
```

**Apply to Phase 12:** Keep deterministic preparation, sorting, offset assignment, and safe-publish sequencing. Replace disk-backed payload/final archive `std::vector<std::byte>` materialization with temp payload files or streamed output. In parallel mode, compute/compress independent payload records concurrently, then assign offsets and write final output serially in deterministic order.

---

### `src/detail/atomic_file_ops.hpp` and `src/formats/ba2/ba2_publish.hpp` (publish/rollback utility)

**Analog:** same files.

**No-replace/replace helper pattern** (`atomic_file_ops.hpp` lines 19-57):
```cpp
/// Publishes a completed temporary regular file to its final host path without replacing an existing path.
/// The operation fails if another actor already owns the destination at publish time.
inline result<void> publish_file_without_replace(const std::filesystem::path& temp_path,
                                                  const std::filesystem::path& output_path) {
#if defined(_WIN32)
  // MoveFileEx without MOVEFILE_REPLACE_EXISTING gives Windows' atomic no-replace publish semantics.
  if (!MoveFileExW(temp_path.c_str(), output_path.c_str(), MOVEFILE_WRITE_THROUGH)) {
    return error{error_code::io_error, "failed to publish host path without replacing an existing file"};
  }
#else
  std::error_code fs_error;
  // POSIX hard-link creation is atomic with respect to the destination name and fails if it already exists.
  std::filesystem::create_hard_link(temp_path, output_path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "failed to publish host path without replacing an existing file"};
  }
  std::filesystem::remove(temp_path, fs_error);
#endif
  return {};
}

/// Replaces an existing host file with a completed temporary regular file using the platform's atomic
/// replacement primitive. If the primitive fails, the destination name remains owned by the original file.
inline result<void> replace_file_atomically(const std::filesystem::path& temp_path,
                                           const std::filesystem::path& output_path) {
```

**Injectable rollback helper pattern** (`ba2_publish.hpp` lines 11-24):
```cpp
/// Restores a reserved output backup after a publish failure and returns the publish error.
/// The rename operation is injectable so unit tests can exercise the rollback behavior without
/// compiling environment-variable-controlled fault injection into the production library target.
template <typename RenameFile>
result<void> restore_backup_after_publish_failure(const std::filesystem::path& backup_path,
                                                  const std::filesystem::path& output_path,
                                                  RenameFile&& rename_file) {
  std::error_code rollback_error;
  std::forward<RenameFile>(rename_file)(backup_path, output_path, rollback_error);
  if (rollback_error) {
    return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path and failed to restore backup"};
  }
  return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path"};
}
```

**Apply to Phase 12:** Reuse or generalize these helpers for all streaming writer flows. Preserve no-replace semantics and rollback tests when parallel compression or temp payload creation fails.

---

### Extraction implementation patterns (bounded streaming)

**Analogs:**
- `src/formats/bsa/tes4_bsa_reader.cpp`
- `src/formats/ba2/ba2_dx10_reader.cpp`

**Fixed raw extraction chunk size and stream limits** (`tes4_bsa_reader.cpp` lines 18-37):
```cpp
constexpr std::size_t extraction_chunk_size = 64U * 1024U;

result<void> validate_stream_limits(std::uint64_t offset, std::uint64_t size, std::string_view description) {
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, std::string{description} + " offset exceeds stream limits"};
  }
  if (size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
    return error{error_code::format_error, std::string{description} + " size exceeds stream limits"};
  }
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()) - size) {
    return error{error_code::format_error, std::string{description} + " span exceeds stream limits"};
  }
  return {};
}
```

**Raw range streaming pattern** (`tes4_bsa_reader.cpp` lines 84-118):
```cpp
result<void> stream_payload_range(std::ifstream& input,
                                  std::uint64_t offset,
                                  std::uint64_t size,
                                  std::string_view description,
                                  payload_sink& sink) {
  auto limits = validate_stream_limits(offset, size, description);
  if (!limits) {
    return limits.error();
  }
  ...
  std::vector<std::byte> buffer(extraction_chunk_size);
  std::uint64_t remaining = size;
  while (remaining != 0U) {
    const auto chunk_size = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, buffer.size()));
    input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(chunk_size));
    ...
    auto written = write_all(sink, std::span<const std::byte>{buffer.data(), chunk_size});
    if (!written) {
      return written.error();
    }
    remaining -= chunk_size;
  }
  return {};
}
```

**Compressed per-entry/per-chunk boundary** (`tes4_bsa_reader.cpp` lines 202-217):
```cpp
const auto compressed_payload_offset = payload_offset + 4U;
if (compressed_payload_offset < payload_offset) {
  return error{error_code::format_error, "TES4 BSA compressed payload offset overflows"};
}
auto compressed_payload =
    read_bytes_at(input, compressed_payload_offset, payload_size - 4U, "TES4 BSA compressed payload");
if (!compressed_payload) {
  return compressed_payload.error();
}

auto decoded = detail::decompress_payload_exact(compression_method_for(entry.compression), compressed_payload.value(),
                                                static_cast<std::size_t>(expected_size));
if (!decoded) {
  return decoded.error();
}
return write_in_chunks(sink, decoded.value());
```

**DX10 one-chunk-at-a-time extraction** (`ba2_dx10_reader.cpp` lines 212-239):
```cpp
// D-13/D-30: write the reconstructed DDS header first and keep later work bounded to one chunk.
auto wrote_header = write_all(sink, header.value());
if (!wrote_header) {
  return wrote_header.error();
}

std::ifstream input{std::string{host_path}, std::ios::binary};
if (!input) {
  return error{error_code::io_error, "failed to open BA2 archive host path for DX10 extraction"};
}

// D-12: DirectXTex validation is intentionally not called here; tests validate returned DDS bytes.
// D-15: entry.texture->chunks is already ordered by the parser's logical_texture_segment/
// source_chunk_index mapping, so extraction reads parser-validated source chunks instead of raw order.
// D-31: chunk compression comes from parsed metadata, never the texture's archive path or extension.
for (const auto& chunk : entry.texture->chunks) {
  if (chunk.compression == entry_compression::none) {
    auto streamed = stream_raw_chunk(input, chunk, sink);
    if (!streamed) {
      return streamed.error();
    }
    continue;
  }
  auto extracted = extract_compressed_chunk(input, chunk, sink);
  if (!extracted) {
    return extracted.error();
  }
}
```

**Apply to Phase 12:** Bounded extraction tests should prove raw paths stream through fixed chunks. Compressed tests should explicitly accept per-entry/per-chunk buffers and reject archive-wide buffering claims.

---

### Public/error model patterns

**Analog:** `include/libbsa/result.hpp`

**Stable error categories** (lines 10-28):
```cpp
/// Stable error categories returned by libbsa public APIs.
///
/// The enum intentionally stays small and format-neutral so callers can branch
/// on durable categories without depending on parser implementation details.
enum class error_code {
  unsupported,
  invalid_argument,
  not_found,
  io_error,
  format_error,
};

/// Structured error value returned by result-producing libbsa APIs.
///
/// `code` is stable and suitable for tests and programmatic handling. `message`
/// is diagnostic text for humans and should not be compared exactly by tests.
struct error {
  error_code code;
  std::string message;
};
```

**C++20 result pattern** (lines 31-37, 90-119):
```cpp
/// C++20-compatible result type used by libbsa public APIs.
///
/// This is a deliberately small expected-like type. I/O and format failures are
/// represented as `error` values. Calling `value()` on an error result or
/// `error()` on a successful result is a programmer mistake and throws
/// `std::logic_error`.
template <typename T>
class result {
```

```cpp
/// C++20-compatible result specialization for operations that return no value.
template <>
class result<void> {
 public:
  /// Creates a successful void result.
  result() noexcept = default;

  /// Creates a failed void result containing `err`.
  result(error err) : error_(std::move(err)), has_value_(false) {}
  ...
  [[nodiscard]] const libbsa::error& error() const {
    if (has_value()) {
      throw std::logic_error("libbsa::result<void> has no error");
    }
    return error_;
  }
```

**Apply to Phase 12:** New public bulk result records should carry `libbsa::error` or `result<void>`-like state, not exception text or `std::expected`. Tests should assert `error_code`, not exact messages.

---

### Test patterns for public API and extraction

**Analogs:**
- `tests/unit/archive_reader_tests.cpp`
- `tests/unit/public_include_boundary_tests.cpp`
- `tests/unit/payload_stream_tests.cpp`

**Public reader compile contract** (`archive_reader_tests.cpp` lines 30-43):
```cpp
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().metadata()),
                             libbsa::result<libbsa::archive_metadata>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().entries()),
                             libbsa::result<std::vector<libbsa::entry_metadata>>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().find("meshes/example.nif")),
                             libbsa::result<std::optional<libbsa::entry_metadata>>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().contains("meshes/example.nif")),
                             libbsa::result<bool>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().extract(
                                 "meshes/example.nif", std::declval<libbsa::payload_sink&>())),
                             libbsa::result<void>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().extract_bytes(
                                 "meshes/example.nif")),
                             libbsa::result<std::vector<std::byte>>>);
```

**Public boundary forbidden-token check** (`public_include_boundary_tests.cpp` lines 190-216):
```cpp
TEST_CASE("public_include_boundary excludes private Phase 2 implementation names", "[unit][public-api]") {
  constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                     "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                     "TES5Edit", "std::expected", "bethesda_hash",
                                                                     "compression_router", "archive_path_key"});
  const auto include_dir = std::filesystem::path{LIBBSA_SOURCE_DIR} / "include" / "libbsa";

  for (const auto& entry : std::filesystem::directory_iterator{include_dir}) {
    if (entry.path().extension() != ".hpp") {
      continue;
    }
    ...
      for (const auto token : forbidden_tokens) {
        INFO("public boundary token: " << token << " in " << entry.path().string());
        REQUIRE(line.find(token) == std::string::npos);
      }
```

**Partial sink failure test** (`payload_stream_tests.cpp` lines 104-116):
```cpp
TEST_CASE("payload_stream rejects partial sink writes and zero chunk size", "[unit][malformed][payload-stream]") {
  auto bytes = payload_bytes();
  memory_source source{bytes};
  partial_sink sink;
  auto partial = libbsa::detail::transfer_payload(source, sink, 17);
  REQUIRE_FALSE(partial);
  REQUIRE(partial.error().code == libbsa::error_code::io_error);

  memory_source zero_source{payload_bytes()};
  memory_sink zero_sink;
  auto zero_chunk = libbsa::detail::transfer_payload(zero_source, zero_sink, 0);
  REQUIRE_FALSE(zero_chunk);
  REQUIRE(zero_chunk.error().code == libbsa::error_code::invalid_argument);
}
```

**Apply to Phase 12:** New `bulk_extraction_tests.cpp` should add static assertions for public types, compare `worker_count = 1` and `worker_count > 1`, include a sink that returns partial writes, and assert deterministic result ordering.

---

### Test patterns for writer correctness and publish safety

**Analogs:**
- `tests/unit/tes4_bsa_writer_tests.cpp`
- `tests/unit/ba2_gnrl_writer_tests.cpp`
- `tests/unit/ba2_dx10_writer_tests.cpp`

**Round-trip/reopen pattern** (`tes4_bsa_writer_tests.cpp` lines 153-159, 205-251):
```cpp
void require_extracted_bytes(const libbsa::archive_reader& reader,
                             std::string_view path,
                             const std::vector<std::byte>& expected) {
  auto extracted = reader.extract_bytes(path);
  REQUIRE(extracted.has_value());
  CHECK(extracted.value() == expected);
}
```

```cpp
auto written = writer.write_to(output.string());
REQUIRE(written.has_value());

auto opened = libbsa::archive_reader::open(output.string());
REQUIRE(opened.has_value());

auto metadata = opened.value().metadata();
REQUIRE(metadata.has_value());
CHECK(metadata.value().type == libbsa::archive_type::bsa);
...
auto extracted = opened.value().extract_bytes(path);
REQUIRE(extracted.has_value());
CHECK(extracted.value() == expected);

collecting_sink sink;
auto streamed = opened.value().extract(path, sink);
REQUIRE(streamed.has_value());
CHECK(sink.bytes() == expected);
```

**Missing disk source error pattern** (`ba2_gnrl_writer_tests.cpp` lines 351-363):
```cpp
TEST_CASE("BA2 GNRL writer reports missing disk sources as I/O errors", "[unit][ba2_gnrl_writer]") {
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
  const auto missing_source = output_path("missing-source-input.nif");
  std::filesystem::remove(missing_source);

  auto added = writer.add_file("Meshes/Missing.nif", missing_source.string());
  REQUIRE(added.has_value());

  auto written = writer.write_to(output_path("missing-source-output.ba2").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
}
```

**DX10 publish rollback helper test** (`ba2_dx10_writer_tests.cpp` lines 797-818):
```cpp
TEST_CASE("BA2 DX10 internal publish rollback helper restores the original archive after publish failure",
          "[unit][ba2_dx10_writer][publish][overwrite]") {
  const auto output = unique_output_path("dx10-overwrite-rollback");
  const std::vector<std::byte> original_bytes{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
  write_binary_file(output, original_bytes);
  auto backup = output;
  backup += ".libbsa-bak-test";
  std::filesystem::remove(backup);
  std::error_code fs_error;
  std::filesystem::rename(output, backup, fs_error);
  REQUIRE_FALSE(fs_error);

  auto restored = libbsa::formats::ba2::publish_detail::restore_backup_after_publish_failure(
      backup, output, [](const std::filesystem::path& from, const std::filesystem::path& to, std::error_code& error) {
        std::filesystem::rename(from, to, error);
      });

  REQUIRE_FALSE(restored.has_value());
  CHECK(restored.error().code == libbsa::error_code::io_error);
  CHECK(read_binary_file(output) == original_bytes);
  CHECK_FALSE(std::filesystem::exists(backup));
}
```

**Apply to Phase 12:** `parallel_writer_tests.cpp` should reuse the reopen/extract validation pattern, compare serial/parallel outputs, and cover missing-source/compression/publish failures without exact message matching.

---

### Policy test patterns

**Analog:** `tests/unit/validation_policy_tests.cpp`

**Read-text helper pattern** (lines 17-28):
```cpp
std::filesystem::path source_root() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR};
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}
```

**Machine-checked docs/catalog pattern** (lines 111-130):
```cpp
TEST_CASE("compatibility evidence catalog documents public warning codes", "[unit][compat][validation_policy]") {
  const auto catalog = read_text_file(source_root() / "docs/compatibility-evidence.md");
  const auto warning_codes = compatibility_warning_codes_from_public_header();
  REQUIRE_FALSE(warning_codes.empty());

  for (const auto code : warning_codes) {
    const auto heading = "### `" + code + "`";
    const auto entry_start = catalog.find(heading);
    INFO("Missing compatibility evidence entry: " << code);
    REQUIRE(entry_start != std::string::npos);

    const auto next_entry = catalog.find("\n### `", entry_start + heading.size());
    const auto entry = catalog.substr(entry_start, next_entry - entry_start);
    REQUIRE(entry.find("Rule:") != std::string::npos);
    REQUIRE(entry.find("Evidence:") != std::string::npos);
  }

  REQUIRE(catalog.find("generated") != std::string::npos);
  REQUIRE(catalog.find("writer-output") != std::string::npos);
}
```

**Build/CI policy pattern** (lines 161-197):
```cpp
TEST_CASE("CI and presets preserve static shared and TES5Edit build boundaries", "[unit][public-api]") {
  const auto root = source_root();
  const auto presets = read_text_file(root / "CMakePresets.json");
  const auto workflow = read_text_file(root / ".github/workflows/ci.yml");

  REQUIRE(presets.find("windows-msvc-debug-static") != std::string::npos);
  REQUIRE(presets.find("windows-msvc-debug-shared") != std::string::npos);
  REQUIRE(presets.find("\"BUILD_SHARED_LIBS\": \"OFF\"") != std::string::npos);
  REQUIRE(presets.find("\"BUILD_SHARED_LIBS\": \"ON\"") != std::string::npos);

  REQUIRE(workflow.find("windows-msvc-debug-static") != std::string::npos);
  REQUIRE(workflow.find("windows-msvc-debug-shared") != std::string::npos);
  REQUIRE(workflow.find("cmake --preset ${{ matrix.preset }}") != std::string::npos);
  REQUIRE(workflow.find("cmake --build --preset ${{ matrix.preset }}") != std::string::npos);
  REQUIRE(workflow.find("ctest --preset ${{ matrix.preset }} --output-on-failure") != std::string::npos);
  REQUIRE(workflow.find("git status --short TES5Edit") != std::string::npos);
  REQUIRE(workflow.find("TES5Edit submodule changed during CI") != std::string::npos);
}
```

**Apply to Phase 12:** Add docs/benchmark/bounded-memory policy tests by reading source/docs/CMake text. Keep checks semantic enough to avoid exact formatting brittleness, but explicit enough to reject final whole-archive vector publishing and missing docs sections.

---

### CMake/build patterns

**Analogs:** `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`

**Root target/source registration pattern** (`CMakeLists.txt` lines 12-15, 44-83):
```cmake
option(LIBBSA_BUILD_TESTS "Build libbsa tests" ON)

add_library(libbsa)
add_library(libbsa::libbsa ALIAS libbsa)
```

```cmake
target_sources(libbsa
  PUBLIC
    FILE_SET HEADERS
      BASE_DIRS include
      FILES
        include/libbsa/archive.hpp
        include/libbsa/libbsa.hpp
        include/libbsa/result.hpp
        include/libbsa/validation.hpp
        include/libbsa/version.hpp
        include/libbsa/writer.hpp
  PRIVATE
    src/archive.cpp
    src/validation.cpp
    ...
    src/libbsa.cpp
)
```

**Test executable and label discovery pattern** (`tests/CMakeLists.txt` lines 5-33, 229-234):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/ba2_gnrl_writer_tests.cpp
  ...
  unit/bethesda_hash_tests.cpp
)
```

```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Package-consumer smoke pattern** (`tests/CMakeLists.txt` lines 236-245):
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

**Package-consumer install/configure/build/test pattern** (`smoke.cmake` lines 7-47):
```cmake
execute_process(
  COMMAND ${CMAKE_COMMAND} --install "${LIBBSA_BUILD_DIR}" --config "${CONFIG}" --prefix "${LIBBSA_INSTALL_PREFIX}"
  RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "libbsa install failed: ${install_result}")
endif()
...
execute_process(
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CONSUMER_BUILD_DIR}" -C "${CONFIG}" --output-on-failure
  RESULT_VARIABLE test_result
)
if(NOT test_result EQUAL 0)
  message(FATAL_ERROR "package consumer test failed: ${test_result}")
endif()
```

**Apply to Phase 12:** Register new sources/tests directly in existing targets. Add Doxygen and benchmark targets as explicit/additive targets, not default CTest speed gates. If adding public headers, update `FILE_SET HEADERS`.

---

### Package-consumer and integration-example patterns

**Analog:** `tests/package-consumer/main.cpp`

**Minimal installed-package consumer style** (lines 1-10):
```cpp
#include <libbsa/libbsa.hpp>

int main() {
  auto result = libbsa::validate_archive("consumer-smoke.bsa");
  if (result) {
    return 1;
  }

  return result.error().code == libbsa::error_code::io_error ? 0 : 1;
}
```

**Apply to Phase 12:** Expand this into compile-checked examples for open/list/extract, bulk extraction, writer creation, `result` handling, and validation. Keep it installed-package-only: include `<libbsa/libbsa.hpp>`, link `libbsa::libbsa`, and avoid private `src/` includes.

---

### Documentation patterns

**Analogs:** `docs/compatibility-evidence.md`, `tests/fixtures/README.md`

**Catalog/guidance intro pattern** (`docs/compatibility-evidence.md` lines 1-9):
```markdown
# Compatibility Evidence Catalog

This catalog maps public `libbsa::compatibility_warning_code` values to the
rule they represent, the evidence that proves the rule, and the default gate
that keeps the evidence reproducible.

Generated legal fixtures and writer-output archives are the mandatory evidence
path for Phase 11. Optional local game or BSArchPro-derived checks may add
smoke/compare confidence, but they are never required for the default suite.
```

**Rule/evidence/default gate pattern** (`docs/compatibility-evidence.md` lines 16-32):
```markdown
### `compressed_sound_payload`

- Rule: BSA entries under `sound/`, or with `.wav`, `.xwm`, or `.fuz` names, are valid but should warn when their payload is compressed because sound compression is a known Bethesda compatibility risk.
- Evidence: `tests/unit/compatibility_warning_tests.cpp` test `compatibility_warning reports compressed sound payloads` creates a synthetic writer-output TES4-family BSA with `sound/fx/alert.wav` compressed through the public writer, validates it through `validate_archive`, and asserts the public warning code and advisory severity. `docs/PRD.md` also tracks the known quirk as a sounds-in-compressed warning.
- Default gate: generated/writer-output; covered by default CTest through the `compatibility_warning` test label and this `validation_policy` catalog check.
```

**Fixture/provenance policy pattern** (`tests/fixtures/README.md` lines 172-187):
```markdown
## Provenance requirements

Each committed generated fixture must document:

1. The generator or source recipe used to create it.
2. The legal provenance that makes it safe to commit.
3. The behavior it proves, such as header parsing, fixture extraction,
   round-trip metadata, compatibility warnings, or malformed-input handling.

## TES5Edit boundary

TES5Edit/ must not be used as a fixture workspace, mutable test data location,
or source of committed fixture files. It is a read-only behavior reference only.

Do not edit, format, compile, stage, or copy generated outputs into `TES5Edit/`
while preparing libbsa tests.
```

**Apply to Phase 12:** Use explicit, machine-checkable headings in `docs/thread-safety.md`, `docs/integration-examples.md`, and `docs/target-format-guide.md`. Include supported variants, compression routes, warning codes, and thread-safety ownership rules with names that policy tests can find.

---

### Benchmark patterns

**Analog:** `tests/CMakeLists.txt` fixture generator targets and `tests/fixtures/README.md`

**Generated fixture target pattern** (`tests/CMakeLists.txt` lines 118-127, 203-226):
```cmake
add_executable(generate_ba2_dx10_fixtures_tool
  fixtures/generated/generate_ba2_dx10_fixtures.cpp
)

target_link_libraries(generate_ba2_dx10_fixtures_tool
  PRIVATE
    libbsa::libbsa
)

target_compile_features(generate_ba2_dx10_fixtures_tool PRIVATE cxx_std_20)
```

```cmake
add_custom_target(generate_ba2_dx10_fixtures
  COMMAND generate_ba2_dx10_fixtures_tool --output ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_ba2_dx10_fixtures_tool
  BYPRODUCTS
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_fo4.ba2
    ...
  COMMENT "Generating BA2 DX10 texture success and malformed fixtures"
  VERBATIM
)
```

**Apply to Phase 12:** Add `benchmarks/libbsa_benchmarks.cpp` as an explicit executable/custom target that generates legal synthetic inputs, runs worker count 1 and >1, validates outputs, and writes JSON/Markdown reports. Policy tests should assert target/report wiring, not speed thresholds.

## Shared Patterns

### Public Headers Stay Dependency-Light
**Source:** `tests/unit/public_include_boundary_tests.cpp` lines 190-216  
**Apply to:** `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/libbsa.hpp`
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                   "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
```

### Partial Sink Writes Are Errors
**Source:** `src/detail/payload_stream.cpp` lines 27-34  
**Apply to:** bulk extraction, bounded extraction tests, writer streaming helpers
```cpp
const auto bytes = std::span<const std::byte>{scratch.data(), read.value()};
auto written = sink.write(bytes);
if (!written) {
  return written.error();
}
if (written.value() != bytes.size()) {
  return libbsa::error{libbsa::error_code::io_error, "payload sink accepted a partial chunk"};
}
```

### Stable Error Assertions
**Source:** `include/libbsa/result.hpp` lines 22-28  
**Apply to:** all new tests and public examples
```cpp
/// Structured error value returned by result-producing libbsa APIs.
///
/// `code` is stable and suitable for tests and programmatic handling. `message`
/// is diagnostic text for humans and should not be compared exactly by tests.
struct error {
  error_code code;
  std::string message;
};
```

### Safe Publish/Rollback
**Source:** `src/detail/atomic_file_ops.hpp` lines 19-57 and `src/formats/ba2/ba2_publish.hpp` lines 11-24  
**Apply to:** TES3, TES4, BA2 GNRL, BA2 DX10 writer refactors
```cpp
inline result<void> publish_file_without_replace(const std::filesystem::path& temp_path,
                                                  const std::filesystem::path& output_path);

inline result<void> replace_file_atomically(const std::filesystem::path& temp_path,
                                           const std::filesystem::path& output_path);
```

### Machine-Checked Documentation
**Source:** `tests/unit/validation_policy_tests.cpp` lines 111-130  
**Apply to:** `docs_policy_tests.cpp`, `thread_safety_docs_policy_tests.cpp`, `bounded_memory_policy_tests.cpp`
```cpp
const auto catalog = read_text_file(source_root() / "docs/compatibility-evidence.md");
const auto warning_codes = compatibility_warning_codes_from_public_header();
REQUIRE_FALSE(warning_codes.empty());

for (const auto code : warning_codes) {
  const auto heading = "### `" + code + "`";
  const auto entry_start = catalog.find(heading);
  INFO("Missing compatibility evidence entry: " << code);
  REQUIRE(entry_start != std::string::npos);
  ...
}
```

### Generated Legal Evidence, Not TES5Edit Fixtures
**Source:** `tests/fixtures/README.md` lines 172-187  
**Apply to:** benchmark data, bounded-memory fixtures, docs examples
```markdown
Each committed generated fixture must document:

1. The generator or source recipe used to create it.
2. The legal provenance that makes it safe to commit.
3. The behavior it proves, such as header parsing, fixture extraction,
   round-trip metadata, compatibility warnings, or malformed-input handling.
```

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `src/detail/parallel_work.hpp` | utility | batch/event-driven | No existing concurrency helper or worker pool exists. Use `payload_stream`/`byte_vector` result style and C++20 standard concurrency from research. |
| `src/detail/parallel_work.cpp` | utility | batch/event-driven | No existing concurrency implementation exists. Planner should require deterministic index-ordered aggregation and no global mutable state. |
| `docs/Doxyfile.in` | config | docs generation | No current Doxygen config. Use CMake `FindDoxygen` pattern from research and root `CMakeLists.txt` option/target style. |

## Metadata

**Analog search scope:** `include/libbsa`, `src`, `tests/unit`, `tests/package-consumer`, `tests/fixtures`, `docs`, root CMake/CI files; `TES5Edit/` excluded from source-edit analogs and left untouched.  
**Files scanned:** 160 non-TES5Edit paths via `rg --files -g '!TES5Edit/**'`.  
**Pattern extraction date:** 2026-05-10.  
**TES5Edit status check:** `git status --short -- TES5Edit` returned no output during mapping.
