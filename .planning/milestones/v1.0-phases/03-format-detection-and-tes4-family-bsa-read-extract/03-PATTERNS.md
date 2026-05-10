# Phase 03: Format Detection and TES4-Family BSA Read/Extract - Pattern Map

**Mapped:** 2026-05-07
**Files analyzed:** 16
**Analogs found:** 14 / 16

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/archive.hpp` | model / public API | request-response | `include/libbsa/result.hpp` | role-match |
| `include/libbsa/result.hpp` | model / public API | request-response | `include/libbsa/result.hpp` | exact-modify |
| `include/libbsa/libbsa.hpp` | config / public API | transform | `include/libbsa/libbsa.hpp` | exact-modify |
| `src/archive.cpp` | route / facade | request-response | `src/archive.cpp` | exact-modify |
| `src/formats/bsa/bsa_format_detector.hpp` | utility | transform | `src/detail/compression_router.hpp` | role-match |
| `src/formats/bsa/bsa_format_detector.cpp` | utility | transform | `src/detail/compression_router.cpp` | role-match |
| `src/formats/bsa/tes4_bsa_parser.hpp` | service / parser | file-I/O | `src/detail/binary_io.hpp` | role-match |
| `src/formats/bsa/tes4_bsa_parser.cpp` | service / parser | file-I/O | `src/detail/binary_io.cpp` | role-match |
| `src/formats/bsa/tes4_bsa_reader.hpp` | service / model | request-response / file-I/O | `src/detail/payload_stream.hpp` | partial |
| `src/formats/bsa/tes4_bsa_reader.cpp` | service | request-response / file-I/O | `src/detail/payload_stream.cpp` | partial |
| `CMakeLists.txt` | config | transform | `CMakeLists.txt` | exact-modify |
| `vcpkg.json` | config | transform | `vcpkg.json` | exact-modify |
| `tests/CMakeLists.txt` | config / test | transform | `tests/CMakeLists.txt` | exact-modify |
| `tests/unit/archive_reader_tests.cpp` | test | request-response | `tests/unit/archive_reader_tests.cpp` | exact-modify |
| `tests/unit/tes4_bsa_reader_tests.cpp` | test | file-I/O / request-response | `tests/unit/compression_router_tests.cpp` | role-match |
| `tests/fixtures/generated/generate_tes4_bsa_fixtures.*` | utility / fixture generator | file-I/O / transform | `src/detail/binary_io.hpp` + codec tests | partial |
| `tests/fixtures/generated/archives/*.json` | test fixture | file-I/O | none | no-analog |
| `tests/fixtures/README.md` | docs / fixture policy | transform | `tests/fixtures/README.md` | exact-modify |

## Pattern Assignments

### `include/libbsa/archive.hpp` (model / public API, request-response)

**Analog:** `include/libbsa/result.hpp` and current `include/libbsa/archive.hpp`

**Imports pattern** (`include/libbsa/archive.hpp` lines 3-5):
```cpp
#include <string_view>

#include <libbsa/result.hpp>
```

**Public Doxygen pattern** (`include/libbsa/result.hpp` lines 21-34):
```cpp
/// Structured error value returned by result-producing libbsa APIs.
///
/// `code` is stable and suitable for tests and programmatic handling. `message`
/// is diagnostic text for humans and should not be compared exactly by tests.
struct error {
  error_code code;
  std::string message;
};

/// C++20-compatible result type used by libbsa public APIs.
///
/// This is a deliberately small expected-like type. I/O and format failures are
/// represented as `error` values; calling `value()` on an error result is a
/// programmer mistake and throws `std::logic_error`.
```

**Facade shape to extend** (`include/libbsa/archive.hpp` lines 15-23):
```cpp
class archive_reader {
 public:
  /// Attempts to open an archive from a host path string.
  ///
  /// Phase 1 deliberately returns `error_code::unsupported` for non-empty paths
  /// because real format detection begins in later phases. Empty paths are
  /// rejected as `error_code::invalid_argument`.
  static result<archive_reader> open(std::string_view host_path);
};
```

**Apply:** Add library-owned public value types here (`archive_metadata`, `entry_metadata`, public sink interface or callback-like equivalent) with tight Doxygen comments. Keep includes dependency-light: standard C++ headers plus `libbsa/result.hpp`; do not expose `detail::`, libdeflate, lz4, DirectXTex, TES5Edit, or C++23 `std::expected`.

---

### `include/libbsa/result.hpp` (model / public API, request-response)

**Analog:** `include/libbsa/result.hpp`

**Error enum extension pattern** (lines 10-19):
```cpp
/// Stable error categories returned by the Phase 1 public API.
///
/// The enum intentionally starts small; later parser and compression phases can
/// add more specific categories once real archive behavior exists.
enum class error_code {
  unsupported,
  invalid_argument,
  io_error,
  format_error,
};
```

**Result error propagation pattern** (lines 76-79, 108-109):
```cpp
/// Returns the contained error.
[[nodiscard]] const libbsa::error& error() const noexcept {
  return std::get<libbsa::error>(storage_);
}

/// Returns the contained error.
[[nodiscard]] const libbsa::error& error() const noexcept { return error_; }
```

**Apply:** Add `not_found` (or exact chosen equivalent) to `error_code` for valid missing extraction paths. Preserve stable-code testing guidance: tests assert `error().code`, not exact messages.

---

### `include/libbsa/libbsa.hpp` (config / public umbrella, transform)

**Analog:** `include/libbsa/libbsa.hpp`

**Umbrella include pattern** (lines 1-5):
```cpp
#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>
#include <libbsa/version.hpp>
```

**Apply:** Only add new public headers if Phase 3 splits public metadata/sink types out of `archive.hpp`. Otherwise leave this file unchanged.

---

### `src/archive.cpp` (route / facade, request-response)

**Analog:** `src/archive.cpp`

**Imports pattern** (line 1):
```cpp
#include <libbsa/archive.hpp>
```

**Validation and structured error pattern** (lines 5-12):
```cpp
result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  return error{error_code::unsupported,
               "archive detection and parsing are not implemented in Phase 1"};
}
```

**Apply:** Keep `archive_reader::open` as the single public route. Preserve empty-path `invalid_argument`, then delegate to private detector/TES4-family open code. Do not use host filename extension as format truth.

---

### `src/formats/bsa/bsa_format_detector.hpp` (utility, transform)

**Analog:** `src/detail/compression_router.hpp`

**Private enum/helper declaration pattern** (lines 9-19):
```cpp
namespace libbsa::detail {

/// Explicit compression method selected from archive metadata by format parsers.
enum class compression_method { none, deflate, lz4_frame, lz4_block };

/// Compresses `input` using the explicitly selected compression method.
result<std::vector<std::byte>> compress_payload(compression_method method, std::span<const std::byte> input);

/// Decompresses `input` with the selected method and exact expected size.
result<std::vector<std::byte>> decompress_payload_exact(compression_method method, std::span<const std::byte> input,
                                                        std::size_t expected_size);
```

**Apply:** Put detector types under a private namespace such as `libbsa::formats::bsa` or `libbsa::detail`. Declare an enum/struct for detected BSA variant/version and a fallible `detect_*` function returning `result<T>`.

---

### `src/formats/bsa/bsa_format_detector.cpp` (utility, transform)

**Analog:** `src/detail/compression_router.cpp`

**Private helper + switch pattern** (lines 7-16, 32-48):
```cpp
namespace libbsa::detail {
namespace {

std::vector<std::byte> copy_bytes(std::span<const std::byte> input) { return {input.begin(), input.end()}; }

libbsa::error unsupported_method_error() {
  return {libbsa::error_code::invalid_argument, "unsupported compression method"};
}

} // namespace

result<std::vector<std::byte>> decompress_payload_exact(compression_method method, std::span<const std::byte> input,
                                                        std::size_t expected_size) {
  switch (method) {
  case compression_method::none:
    if (input.size() != expected_size) {
      return libbsa::error{libbsa::error_code::format_error, "uncompressed payload size did not match metadata"};
    }
    return copy_bytes(input);
  case compression_method::deflate:
    return decompress_deflate_exact(input, expected_size);
  case compression_method::lz4_frame:
    return decompress_lz4_frame_exact(input, expected_size);
  case compression_method::lz4_block:
    return decompress_lz4_block_exact(input, expected_size);
  }
  return unsupported_method_error();
}
```

**Apply:** Use anonymous-namespace constants for `BSA\0`, versions `0x67`, `0x68`, `0x69`; switch on parsed version; return `format_error` for malformed/truncated bytes and `unsupported` for unsupported but structurally recognizable versions if planner chooses that distinction.

---

### `src/formats/bsa/tes4_bsa_parser.hpp` (service / parser, file-I/O)

**Analog:** `src/detail/binary_io.hpp`

**Parser class declaration pattern** (lines 12-20, 21-43):
```cpp
/// Reads fixed-width little-endian archive fields from a bounded byte span.
///
/// The reader never advances after failed reads, which lets format parsers report
/// truncation without losing the offset that caused the failure.
class binary_reader {
 public:
  /// Creates a reader over immutable archive bytes owned by the caller.
  explicit binary_reader(std::span<const std::byte> bytes) noexcept;

  /// Returns the current byte offset from the start of the input span.
  [[nodiscard]] std::size_t position() const noexcept;

  /// Returns the number of bytes available before a read would be truncated.
  [[nodiscard]] std::size_t remaining() const noexcept;

  /// Reads an unsigned 8-bit value and advances one byte on success.
  result<std::uint8_t> read_u8();
```

**Apply:** Make parser own no public dependency types. Accept `std::span<const std::byte>` or host bytes; return an internal parsed model with archive header, folder/file record metadata, original paths, canonical keys, and payload offsets. Add Doxygen comments for non-trivial parser APIs.

---

### `src/formats/bsa/tes4_bsa_parser.cpp` (service / parser, file-I/O)

**Analog:** `src/detail/binary_io.cpp`, `src/detail/archive_path.cpp`, `src/detail/bethesda_hash.cpp`

**Checked-read error pattern** (`src/detail/binary_io.cpp` lines 8-20, 65-71):
```cpp
libbsa::error truncated_error() {
  return {libbsa::error_code::format_error, "binary input ended before requested field"};
}

binary_reader::binary_reader(std::span<const std::byte> bytes) noexcept : bytes_(bytes) {}

std::size_t binary_reader::remaining() const noexcept { return bytes_.size() - position_; }

bool binary_reader::can_read(std::size_t count) const noexcept { return count <= remaining(); }

result<std::span<const std::byte>> binary_reader::read_bytes(std::size_t count) {
  if (!can_read(count)) {
    return truncated_error();
  }
  const auto start = position_;
  position_ += count;
  return bytes_.subspan(start, count);
}
```

**Path normalization + invalid-path pattern** (`src/detail/archive_path.cpp` lines 24-57):
```cpp
result<archive_path_key> normalize_archive_path(std::string_view input) {
  if (input.empty() || input.front() == '/' || input.front() == '\\' || is_drive_rooted(input)) {
    return invalid_path_error();
  }

  archive_path_key key;
  key.value.reserve(input.size());
  std::string segment;

  for (const char raw : input) {
    const char normalized = raw == '\\' ? '/' : lower_ascii(raw);
    if (normalized == '/') {
      if (segment.empty() || segment == "." || segment == "..") {
        return invalid_path_error();
      }
      if (!key.value.empty()) {
        key.value.push_back('/');
      }
      key.value.append(segment);
      segment.clear();
      continue;
    }
    segment.push_back(normalized);
  }
```

**Hash compatibility comments/pattern** (`src/detail/bethesda_hash.cpp` lines 87-130):
```cpp
std::uint64_t hash_tes4(std::string_view name_without_extension, std::string_view extension_with_dot) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashTES4 encodes name edge bytes and
  // extension special bits for .kf/.nif/.dds/.wav, then adds sdbm accumulators.
  const auto length = name_without_extension.size();
  if (length == 0) {
    return 0;
  }
```

**Apply:** Parse in archive record order, validate all count/offset spans before slicing, call existing `normalize_archive_path` for public keys, reject duplicate canonical paths with `format_error`, and call `hash_tes4` instead of reimplementing hash behavior.

---

### `src/formats/bsa/tes4_bsa_reader.hpp` (service / model, request-response / file-I/O)

**Analog:** `src/detail/payload_stream.hpp`

**Sink interface pattern** (lines 22-35):
```cpp
/// Bounded synchronous sink for archive payload bytes.
class payload_sink {
 public:
  virtual ~payload_sink() = default;

  /// Writes `bytes` and returns the number accepted by the sink.
  virtual result<std::size_t> write(std::span<const std::byte> bytes) = 0;
};

/// Copies all remaining payload bytes using bounded chunks.
///
/// Partial sink acceptance is reported as `io_error` so callers never observe an
/// ambiguous partial-success extraction or packing operation.
result<void> transfer_payload(payload_source& source, payload_sink& sink, std::size_t chunk_size);
```

**Apply:** Public sink can mirror this shape, but must live in `include/libbsa/archive.hpp` or another public header without `detail::` names. Internal TES4 reader should bridge public sink to `detail::payload_sink` semantics.

---

### `src/formats/bsa/tes4_bsa_reader.cpp` (service, request-response / file-I/O)

**Analog:** `src/detail/payload_stream.cpp`, `src/detail/compression_router.cpp`

**Partial sink failure pattern** (`src/detail/payload_stream.cpp` lines 8-37):
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

**Exact-size decompression routing pattern** (`src/detail/compression_router.cpp` lines 32-48):
```cpp
result<std::vector<std::byte>> decompress_payload_exact(compression_method method, std::span<const std::byte> input,
                                                        std::size_t expected_size) {
  switch (method) {
  case compression_method::none:
    if (input.size() != expected_size) {
      return libbsa::error{libbsa::error_code::format_error, "uncompressed payload size did not match metadata"};
    }
    return copy_bytes(input);
  case compression_method::deflate:
    return decompress_deflate_exact(input, expected_size);
  case compression_method::lz4_frame:
    return decompress_lz4_frame_exact(input, expected_size);
  case compression_method::lz4_block:
    return decompress_lz4_block_exact(input, expected_size);
  }
  return unsupported_method_error();
}
```

**Apply:** Extraction should normalize lookup path, return `not_found` for valid missing paths, skip embedded-name prefix before payload read/decompression, route v103/v104 compressed entries to `compression_method::deflate`, route v105 compressed entries to `compression_method::lz4_frame`, and enforce exact sink acceptance.

---

### `CMakeLists.txt` (config, transform)

**Analog:** `CMakeLists.txt`

**Private dependency boundary pattern** (lines 8-10, 36-40):
```cmake
find_package(libdeflate CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

target_link_libraries(libbsa
  PRIVATE
    $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,libdeflate::libdeflate_static,libdeflate::libdeflate_shared>
    lz4::lz4
)
```

**Header file set + private source registration pattern** (lines 42-62):
```cmake
target_sources(libbsa
  PUBLIC
    FILE_SET HEADERS
      BASE_DIRS include
      FILES
        include/libbsa/archive.hpp
        include/libbsa/libbsa.hpp
        include/libbsa/result.hpp
        include/libbsa/version.hpp
  PRIVATE
    src/archive.cpp
    src/detail/archive_path.cpp
    src/detail/bethesda_hash.cpp
    src/detail/binary_io.cpp
    src/detail/compression_router.cpp
    src/detail/deflate_codec.cpp
    src/detail/lz4_block_codec.cpp
    src/detail/lz4_frame_codec.cpp
    src/detail/payload_stream.cpp
    src/libbsa.cpp
)
```

**Apply:** Add private `src/formats/bsa/*.cpp` sources under `PRIVATE`. Only add public headers to `FILE_SET HEADERS` if they are intended installed API. Do not put private format headers in the public header file set.

---

### `vcpkg.json` (config, transform)

**Analog:** `vcpkg.json`

**Dependency declaration pattern** (lines 4-16):
```json
"dependencies": [
  {
    "name": "libdeflate",
    "features": [
      "compression",
      "decompression"
    ]
  },
  "lz4",
  "directxtex",
  "catch2"
],
"builtin-baseline": "12dcccadfe573d0eaa6c67a968413ded7805d256"
```

**Apply:** If adopting the research recommendation, add `nlohmann-json` as a test/tool-only dependency and document it in the phase plan as the D-29 exception. Do not link it to `libbsa`.

---

### `tests/CMakeLists.txt` (config / test, transform)

**Analog:** `tests/CMakeLists.txt`

**Test source registration pattern** (lines 1-22):
```cmake
find_package(Catch2 CONFIG REQUIRED)

add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/public_include_boundary_tests.cpp
  unit/local_game_fixture_tests.cpp
  unit/validation_policy_tests.cpp
  unit/binary_io_tests.cpp
  unit/archive_path_tests.cpp
  unit/payload_stream_tests.cpp
  unit/deflate_codec_tests.cpp
  unit/lz4_codec_tests.cpp
  unit/compression_router_tests.cpp
  unit/bethesda_hash_tests.cpp
)

target_link_libraries(libbsa_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain
)
```

**Source-dir macro + Catch labels pattern** (lines 31-41):
```cmake
target_compile_definitions(libbsa_tests
  PRIVATE
    LIBBSA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Apply:** Add `unit/tes4_bsa_reader_tests.cpp` to `libbsa_tests`. If using nlohmann-json, `find_package(nlohmann_json CONFIG REQUIRED)` here and link only `libbsa_tests PRIVATE nlohmann_json::nlohmann_json`.

---

### `tests/unit/archive_reader_tests.cpp` (test, request-response)

**Analog:** `tests/unit/archive_reader_tests.cpp`

**Public API test pattern** (lines 1-17):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

TEST_CASE("archive_reader open reports unsupported archives in phase one", "[unit][public-api]") {
  auto result = libbsa::archive_reader::open("example.bsa");

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().code == libbsa::error_code::unsupported);
}

TEST_CASE("archive_reader open rejects empty host paths", "[unit][public-api]") {
  auto result = libbsa::archive_reader::open("");

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().code == libbsa::error_code::invalid_argument);
}
```

**Apply:** Replace unsupported-stub expectation for non-empty paths with byte-driven fixture expectations. Preserve the empty-path invalid-argument test.

---

### `tests/unit/tes4_bsa_reader_tests.cpp` (test, file-I/O / request-response)

**Analog:** `tests/unit/compression_router_tests.cpp`, `tests/unit/archive_path_tests.cpp`, `tests/unit/payload_stream_tests.cpp`, `tests/unit/local_game_fixture_tests.cpp`

**Catch2 test structure and tags** (`tests/unit/compression_router_tests.cpp` lines 24-33):
```cpp
TEST_CASE("compression_router dispatches explicit codec methods", "[unit][compression][compression-router]") {
  const auto original = router_vector();
  for (auto method : {libbsa::detail::compression_method::deflate, libbsa::detail::compression_method::lz4_frame,
                      libbsa::detail::compression_method::lz4_block}) {
    auto compressed = libbsa::detail::compress_payload(method, original);
    REQUIRE(compressed);
    auto decoded = libbsa::detail::decompress_payload_exact(method, compressed.value(), original.size());
    REQUIRE(decoded);
    REQUIRE(decoded.value() == original);
  }
}
```

**Malformed error-code assertion pattern** (`tests/unit/archive_path_tests.cpp` lines 15-29):
```cpp
TEST_CASE("archive_path rejects obvious invalid virtual paths", "[unit][archive-path][malformed]") {
  constexpr auto invalid_paths = std::to_array<std::string_view>({
      "",
      "/absolute/path",
      "C:/drive/rooted",
      "textures//bad.dds",
      "textures/./bad.dds",
      "textures/../bad.dds",
  });

  for (const auto input : invalid_paths) {
    auto key = libbsa::detail::normalize_archive_path(input);
    REQUIRE_FALSE(key);
    REQUIRE(key.error().code == libbsa::error_code::invalid_argument);
  }
}
```

**Test sink analog** (`tests/unit/payload_stream_tests.cpp` lines 42-53):
```cpp
class memory_sink final : public libbsa::detail::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

 private:
  std::vector<std::byte> bytes_;
};
```

**Opt-in fixture skip pattern** (`tests/unit/local_game_fixture_tests.cpp` lines 6-12):
```cpp
TEST_CASE("local game fixtures are opt-in", "[requires-game-fixture][unit]") {
  const char* fixture_root = std::getenv("LIBBSA_GAME_FIXTURES");
  if (fixture_root == nullptr || std::string_view{fixture_root}.empty()) {
    SKIP("Set LIBBSA_GAME_FIXTURES or place local game archives under tests/fixtures/local; these files are not committed.");
  }

  REQUIRE_FALSE(std::string_view{fixture_root}.empty());
}
```

**Apply:** Tag default generated-fixture tests `[unit][fixture]` or `[unit][malformed][fixture]`; tag optional local comparisons `[requires-game-fixture][compat]`. Assert stable error codes and byte vectors, not exact diagnostic strings.

---

### `tests/fixtures/generated/generate_tes4_bsa_fixtures.*` (utility / fixture generator, file-I/O / transform)

**Analog:** `src/detail/binary_io.hpp`, codec tests

**Binary writer pattern** (`src/detail/binary_io.hpp` lines 52-75):
```cpp
/// Writes little-endian archive fields into an owned byte buffer.
class binary_writer {
 public:
  /// Appends an unsigned 8-bit value.
  result<void> write_u8(std::uint8_t value);

  /// Appends an unsigned 16-bit value in little-endian byte order.
  result<void> write_u16_le(std::uint16_t value);

  /// Appends an unsigned 32-bit value in little-endian byte order.
  result<void> write_u32_le(std::uint32_t value);

  /// Appends an unsigned 64-bit value in little-endian byte order.
  result<void> write_u64_le(std::uint64_t value);

  /// Appends raw bytes preserving their exact order.
  result<void> write_bytes(std::span<const std::byte> bytes);

  /// Returns all bytes written so far.
  [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
```

**Compression fixture data pattern** (`tests/unit/deflate_codec_tests.cpp` lines 24-32; `tests/unit/lz4_codec_tests.cpp` lines 25-38):
```cpp
auto compressed = libbsa::detail::compress_deflate(original);
REQUIRE(compressed);
auto decompressed = libbsa::detail::decompress_deflate_exact(compressed.value(), original.size());

REQUIRE(decompressed);
REQUIRE(decompressed.value() == original);
```

```cpp
auto frame = libbsa::detail::compress_lz4_frame(original);
REQUIRE(frame);
auto frame_decoded = libbsa::detail::decompress_lz4_frame_exact(frame.value(), original.size());
REQUIRE(frame_decoded);
REQUIRE(frame_decoded.value() == original);
```

**Apply:** Prefer a C++ fixture generator if reusing internal binary writer/compression/hash helpers. If planner chooses Python, copy the generated fixture provenance/docs pattern but treat byte-layout/compression/hash duplication as a risk to mitigate in tests.

---

### `tests/fixtures/README.md` (docs / fixture policy, transform)

**Analog:** `tests/fixtures/README.md`

**Generated fixture placement pattern** (lines 7-15):
```markdown
## Committed generated fixtures

Committed fixtures must be tiny, legal, and generated specifically for tests.

- Source inputs belong under `tests/fixtures/generated/source`.
- Generated archive outputs belong under `tests/fixtures/generated/archives`.
- Generated binary fixtures are not required in Phase 1 and may be added later
  only when their provenance and recipe are documented.
```

**Provenance + TES5Edit boundary pattern** (lines 26-41):
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

**Apply:** Update Phase 1 wording if needed and document generated TES4-family BSA fixture recipe, manifest fields, and legal provenance. Keep `TES5Edit/` read-only.

## Shared Patterns

### Public API boundary and Doxygen comments
**Source:** `include/libbsa/result.hpp`, `include/libbsa/archive.hpp`
**Apply to:** All public headers and new public APIs
```cpp
/// C++20-compatible result type used by libbsa public APIs.
///
/// This is a deliberately small expected-like type. I/O and format failures are
/// represented as `error` values; calling `value()` on an error result is a
/// programmer mistake and throws `std::logic_error`.
template <typename T>
class result {
```

### Structured errors instead of exceptions for I/O/format failures
**Source:** `src/archive.cpp`, `src/detail/binary_io.cpp`, tests
**Apply to:** Parser, detector, lookup, extraction, fixtures tests
```cpp
if (host_path.empty()) {
  return error{error_code::invalid_argument, "archive path must not be empty"};
}
```
```cpp
if (!can_read(count)) {
  return truncated_error();
}
```

### Checked binary parsing
**Source:** `src/detail/binary_io.hpp/.cpp`
**Apply to:** `tes4_bsa_parser.*`, fixture generator if C++
```cpp
/// The reader never advances after failed reads, which lets format parsers report
/// truncation without losing the offset that caused the failure.
class binary_reader {
```

### Archive virtual path normalization
**Source:** `src/detail/archive_path.cpp`
**Apply to:** Parser duplicate detection, public lookup, `contains`, `extract`
```cpp
const char normalized = raw == '\\' ? '/' : lower_ascii(raw);
if (normalized == '/') {
  if (segment.empty() || segment == "." || segment == "..") {
    return invalid_path_error();
  }
```

### TES4 hash compatibility with reference comments
**Source:** `src/detail/bethesda_hash.cpp`
**Apply to:** Entry metadata, internal record/hash compatibility tests
```cpp
// TES5Edit/Core/wbBSArchive.pas CreateHashTES4 encodes name edge bytes and
// extension special bits for .kf/.nif/.dds/.wav, then adds sdbm accumulators.
```

### Compression routing
**Source:** `src/detail/compression_router.cpp`
**Apply to:** TES4-family extraction and fixture generation
```cpp
case compression_method::deflate:
  return decompress_deflate_exact(input, expected_size);
case compression_method::lz4_frame:
  return decompress_lz4_frame_exact(input, expected_size);
```

### Sink partial-write failure
**Source:** `src/detail/payload_stream.cpp`
**Apply to:** Public extraction and tests
```cpp
if (written.value() != bytes.size()) {
  return libbsa::error{libbsa::error_code::io_error, "payload sink accepted a partial chunk"};
}
```

### Catch2/CTest labels
**Source:** `tests/CMakeLists.txt`
**Apply to:** All Phase 3 tests
```cmake
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

## No Analog Found

Files with no close match in the codebase (planner should use RESEARCH.md patterns instead):

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `tests/fixtures/generated/archives/*.json` | test fixture | file-I/O | No JSON manifests currently exist; use RESEARCH.md nlohmann-json pattern and fixture README provenance requirements. |
| `tests/fixtures/generated/source/*` | test fixture inputs | file-I/O | No generated fixture source tree currently exists; follow README placement/provenance policy. |

## Metadata

**Analog search scope:** `include/libbsa/`, `src/`, `tests/`, root `CMakeLists.txt`, `vcpkg.json`, `tests/fixtures/README.md`
**Files scanned:** 23
**Pattern extraction date:** 2026-05-07
**TES5Edit boundary:** Reference files named in research/context were not modified; implementation patterns above all come from libbsa-owned files outside `TES5Edit/`.
