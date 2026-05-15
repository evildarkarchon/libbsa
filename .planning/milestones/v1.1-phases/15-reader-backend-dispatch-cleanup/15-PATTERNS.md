# Phase 15: Reader Backend Dispatch Cleanup - Pattern Map

**Mapped:** 2026-05-14
**Files analyzed:** 4
**Analogs found:** 4 / 4

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|---|---|---|---|---|
| `src/archive.cpp` | service | request-response | `src/archive.cpp` | exact |
| `tests/unit/archive_reader_dispatch_tests.cpp` | test | request-response | `tests/unit/bulk_extraction_tests.cpp` | role-match |
| `tests/unit/archive_reader_dispatch_policy_tests.cpp` | test | file-I/O | `tests/unit/host_file_writer_name_tests.cpp` | role-match |
| `tests/CMakeLists.txt` | config | batch | `tests/CMakeLists.txt` | exact |

## Pattern Assignments

### `src/archive.cpp` (service, request-response)

**Primary analog:** `src/archive.cpp`

**Supporting analogs:**
- `src/formats/bsa/tes3_bsa_reader.cpp`
- `src/formats/bsa/tes4_bsa_reader.cpp`
- `src/formats/ba2/ba2_gnrl_reader.cpp`
- `src/formats/ba2/ba2_dx10_reader.cpp`

**Imports + file-local helper pattern** (`src/archive.cpp:1-26`, `41-111`):
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
...
namespace {
...
result<void> extract_entry_payload(const archive_metadata& metadata,
                                   bool is_ba2_dx10,
                                   const detail::host_file_path& host_path,
                                   const entry_metadata& entry,
                                   payload_sink& sink) {
  if (metadata.variant == archive_variant::tes3) {
    return formats::bsa::extract_tes3_bsa_payload(host_path, entry, sink);
  }
  if (metadata.type == archive_type::ba2) {
    return is_ba2_dx10 ? formats::ba2::extract_ba2_dx10_payload(host_path, entry, sink)
                       : formats::ba2::extract_ba2_gnrl_payload(host_path, entry, sink);
  }
  return formats::bsa::extract_tes4_bsa_payload_from_file(host_path, entry, sink);
}
```
**Copy from this:** keep the new backend seam file-local inside the anonymous namespace; do not move it into a new shared private API.

**State pattern** (`src/archive.cpp:30-36`):
```cpp
struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  /// Keeps caller UTF-8 text for diagnostics only; open-time and parser-time host-file I/O stay on the resolved path.
  detail::host_file_path host_path;
  bool is_ba2_dx10{false};
};
```
**Copy from this:** preserve the compact shared state shape and replace the boolean with one backend identity field instead of adding parallel backend payload state.

**Open-time centralization pattern** (`src/archive.cpp:113-205`):
```cpp
auto resolved_host_path = detail::resolve_host_file_path(host_path);
...
auto prefix = read_detection_prefix(resolved_host_path.value());
...
auto archive_size = archive_file_size(resolved_host_path.value());
...
archive_reader reader{ba2_archive.value().metadata};
reader.state_ = std::make_shared<state>(state{ba2_archive.value().metadata,
                                              std::move(ba2_archive.value().entries),
                                              std::move(resolved_host_path).value(),
                                              true});
return reader;
```
**Copy from this:** keep detection, parse, and final state assembly centralized in `open()`, with one helper that returns the chosen backend plus final state payload.

**Shared facade orchestration pattern** (`src/archive.cpp:314-382`):
```cpp
std::vector<bulk_extract_entry_result> results(requests.size());
std::vector<bulk_request_group> groups;
...
const auto [group, inserted] = group_by_path.emplace(request.path, groups.size());
if (inserted) {
  groups.push_back(bulk_request_group{request.path, std::vector<std::size_t>{index}});
  continue;
}
groups[group->second].result_indices.push_back(index);
...
auto found = find(group.path);
...
auto extracted = extract_entry_payload(state_->metadata,
                                       state_->is_ba2_dx10,
                                       state_->host_path,
                                       *record.entry,
                                       *sink.value());
```
**Copy from this:** keep exact request-string coalescing, per-request result mirroring, and outer worker orchestration in the facade; only move family-sensitive list/find/extract primitives behind the backend table.

**Contains wrapper pattern** (`src/formats/bsa/tes3_bsa_reader.cpp:66-72`; same shape in `tes4_bsa_reader.cpp:106-112`, `ba2_gnrl_reader.cpp:95-101`, `ba2_dx10_reader.cpp:84-90`):
```cpp
result<bool> contains_tes3_bsa_entry(std::span<const entry_metadata> entries, std::string_view path) {
  auto found = find_tes3_bsa_entry(entries, path);
  if (!found) {
    return found.error();
  }
  return found.value().has_value();
}
```
**Copy from this:** implement `archive_reader::contains()` as `find()` + `has_value()` at the facade level instead of adding a dedicated backend callback.

**Lookup normalization pattern** (`src/formats/ba2/ba2_gnrl_reader.cpp:78-93`; same shape in all four readers):
```cpp
auto normalized = detail::normalize_archive_path(path);
if (!normalized) {
  return normalized.error();
}

const auto found = std::lower_bound(entries.begin(), entries.end(), normalized.value().value,
                                    [](const entry_metadata& entry, const std::string& key) {
                                      return entry.path < key;
                                    });
```
**Copy from this:** backend `find` callbacks should keep returning `result<std::optional<entry_metadata>>` so the facade can preserve existing invalid-path vs not-found semantics.

---

### `tests/unit/archive_reader_dispatch_tests.cpp` (test, request-response)

**Primary analog:** `tests/unit/bulk_extraction_tests.cpp`

**Supporting analogs:**
- `tests/unit/tes3_bsa_reader_tests.cpp`
- `tests/unit/tes4_bsa_reader_tests.cpp`
- `tests/unit/ba2_gnrl_reader_tests.cpp`
- `tests/unit/ba2_dx10_extraction_tests.cpp`
- `tests/unit/archive_reader_tests.cpp`

**Includes + local helper layout** (`tests/unit/bulk_extraction_tests.cpp:1-25`, `27-40`):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>
...
namespace {

std::filesystem::path bulk_extraction_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_bulk_extraction_tests";
  std::filesystem::create_directories(path);
  return path;
}
```
**Copy from this:** use anonymous-namespace helpers, local sink/factory test doubles, and plain Catch2 `TEST_CASE`s.

**Committed fixture path helper pattern** (`tests/unit/tes3_bsa_reader_tests.cpp:29-39`; same shape in TES4/BA2 suites):
```cpp
std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

std::filesystem::path generated_archive_path(std::string_view filename) {
  return generated_archive_dir() / std::string{filename};
}
```
**Copy from this:** the new dispatch runtime suite should reuse committed fixtures from `tests/fixtures/generated/archives`, not synthesize a new archive matrix.

**Cross-fixture loop pattern** (`tests/unit/tes4_bsa_reader_tests.cpp:40-53`, `194-207`; `tests/unit/ba2_gnrl_reader_tests.cpp:146-157`, `676-717`; `tests/unit/ba2_dx10_extraction_tests.cpp:77-80`, `143-162`):
```cpp
std::vector<success_fixture> success_fixtures() {
  return {
      {.archive_filename = "tes4_v103.bsa", .version = 103U, .flags = 3U, .file_count = 2U},
      {.archive_filename = "tes4_v104.bsa", .version = 104U, .flags = 259U, .file_count = 2U},
      {.archive_filename = "tes4_v105.bsa", .version = 105U, .flags = 259U, .file_count = 2U},
  };
}
...
for (const auto& fixture : success_fixtures()) {
  auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
  REQUIRE(opened.has_value());
```
**Copy from this:** build one compact representative matrix and iterate it inside one dedicated suite instead of scattering Phase 15 assertions across family-specific files.

**Public API contract assertions** (`tests/unit/archive_reader_tests.cpp:30-44`):
```cpp
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().entries()),
                             libbsa::result<std::vector<libbsa::entry_metadata>>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().find("meshes/example.nif")),
                             libbsa::result<std::optional<libbsa::entry_metadata>>>);
```
**Copy from this:** keep any compile-surface checks lightweight and separate from runtime fixture loops.

**Bulk extraction assertion pattern** (`tests/unit/bulk_extraction_tests.cpp:237-247`, `455-483`, `485-578`):
```cpp
extraction_run extract_with_workers(const libbsa::archive_reader& reader,
                                    std::span<const libbsa::bulk_extract_request> requests,
                                    std::uint32_t worker_count) {
  recording_sink_factory sink_factory;
  libbsa::bulk_extract_options options;
  options.worker_count = worker_count;

  auto extracted = reader.extract_entries(requests, sink_factory, options);
  REQUIRE(extracted.has_value());
  return extraction_run{std::move(extracted).value(), sink_factory.report()};
}
```
**Copy from this:** test `extract_entries()` through helper sink factories and assert request-order preservation, duplicate mirroring, and per-entry success/failure records.

**Per-family operation assertions** (`tests/unit/tes3_bsa_reader_tests.cpp:346-381`, `397-419`; `tests/unit/tes4_bsa_reader_tests.cpp:476-619`; `tests/unit/ba2_gnrl_reader_tests.cpp:676-755`; `tests/unit/ba2_dx10_extraction_tests.cpp:143-162`):
```cpp
auto found = opened.value().find(variant.get<std::string>());
REQUIRE(found.has_value());
REQUIRE(found.value().has_value());
...
auto contains = opened.value().contains(variant.get<std::string>());
REQUIRE(contains.has_value());
REQUIRE(contains.value());
...
auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);
REQUIRE(extracted.has_value());
...
auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
REQUIRE(bytes.has_value());
```
**Copy from this:** for each representative backend, cover `entries`, `find`, `contains`, `extract`, and `extract_bytes` once with manifest-backed expectations.

---

### `tests/unit/archive_reader_dispatch_policy_tests.cpp` (test, file-I/O)

**Primary analog:** `tests/unit/host_file_writer_name_tests.cpp`

**Supporting analog:** `tests/unit/validation_policy_tests.cpp`

**Repo-reading helper pattern** (`tests/unit/host_file_writer_name_tests.cpp:12-20`; same shape in `validation_policy_tests.cpp:20-31`):
```cpp
std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.is_open());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}
```
**Copy from this:** keep the policy suite dependency-light and read repo files directly.

**Method-scoped source guard pattern** (`tests/unit/host_file_writer_name_tests.cpp:99-114`, `176-182`):
```cpp
const auto archive_text = read_text_file(source_root() / "src/archive.cpp");

REQUIRE(archive_text.find("detail::host_file_path host_path;") != std::string::npos);
REQUIRE(archive_text.find("resolve_host_file_path(host_path)") != std::string::npos);
...
REQUIRE(archive_text.find("state_->host_path, *found.value(), sink") != std::string::npos);
REQUIRE(archive_text.find("state_->host_path.original_utf8, *found.value(), sink") == std::string::npos);
```
**Copy from this:** inspect `src/archive.cpp` directly and assert presence/absence of critical tokens.

**Token-count / occurrence helper pattern** (`tests/unit/validation_policy_tests.cpp:173-180`):
```cpp
std::size_t count_occurrences(std::string_view text, std::string_view needle) {
  std::size_t count = 0;
  std::size_t offset = 0;
  while ((offset = text.find(needle, offset)) != std::string_view::npos) {
    ++count;
    offset += needle.size();
  }
  return count;
}
```
**Copy from this:** use small string helpers when the guard needs to prove repeated branching disappeared without locking one exact helper name.

**Negative-only policy style** (`tests/unit/host_file_writer_name_tests.cpp:41-47`, `109-113`; `tests/unit/validation_policy_tests.cpp:219-223`):
```cpp
REQUIRE(text.find("open_host_file(") != std::string::npos);
REQUIRE(text.find("std::ifstream input{std::string{host_path}, std::ios::binary}") == std::string::npos);
```
**Copy from this:** fail on forbidden branch tokens inside the affected methods (`metadata.variant`, `metadata.type`, `is_ba2_dx10`, or backend-specific helper names in method bodies), but allow one centralized selector elsewhere in the file.

---

### `tests/CMakeLists.txt` (config, batch)

**Primary analog:** `tests/CMakeLists.txt`

**Unit test registration pattern** (`tests/CMakeLists.txt:65-111`):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/bulk_extraction_tests.cpp
  ...
  unit/host_file_writer_name_tests.cpp
  unit/host_path_correctness_boundary_tests.cpp
  unit/bethesda_hash_tests.cpp
)
```
**Copy from this:** add new Phase 15 test files directly to the single `libbsa_tests` target alongside other unit suites.

**Shared test target wiring** (`tests/CMakeLists.txt:113-132`):
```cmake
target_link_libraries(libbsa_tests
  PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

libbsa_link_internal_test_support(libbsa_tests)

target_compile_features(libbsa_tests PRIVATE cxx_std_20)
```
**Copy from this:** no extra test target is needed; rely on the existing `libbsa_tests` wiring.

**Catch2 discovery pattern** (`tests/CMakeLists.txt:295-300`):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```
**Copy from this:** new suites become discoverable automatically once added to `libbsa_tests`.

**CTest naming/comment style** (`tests/CMakeLists.txt:302-325`):
```cmake
# The supported windows-msvc-release-static and windows-msvc-release-shared lanes own this
# Windows-only package proof inside the normal CTest graph. Keep opt-in requires-game-fixture
# corpus checks separate so Release proof stays runnable from committed repository assets.
```
**Copy from this:** keep short explanatory comments when a test/config choice exists for lane-policy reasons.

## Shared Patterns

### Open-time selection stays centralized
**Source:** `src/archive.cpp:113-205`
**Apply to:** `src/archive.cpp`
```cpp
auto resolved_host_path = detail::resolve_host_file_path(host_path);
auto prefix = read_detection_prefix(resolved_host_path.value());
auto archive_size = archive_file_size(resolved_host_path.value());
```
Use one open-time selector that resolves host path, reads detection bytes, parses, and stores final backend identity once.

### Shared facade wrappers keep public semantics stable
**Source:** `src/archive.cpp:275-300`, `303-382`
**Apply to:** `src/archive.cpp`, `tests/unit/archive_reader_dispatch_tests.cpp`
```cpp
auto found = find(path);
if (!found.value()) {
  return error{error_code::not_found, "archive path was not found"};
}
```
Keep `extract_bytes()` and `extract_entries()` routing through shared lookup/error translation instead of re-implementing family-specific failure behavior.

### `contains == has_value(find(...))`
**Source:** `src/formats/bsa/tes3_bsa_reader.cpp:66-72` and peers
**Apply to:** `src/archive.cpp`
```cpp
auto found = find_tes3_bsa_entry(entries, path);
if (!found) {
  return found.error();
}
return found.value().has_value();
```
Do not add a dedicated backend `contains` callback.

### Repo-reading policy tests are simple file-text assertions
**Source:** `tests/unit/host_file_writer_name_tests.cpp:12-20`, `99-114`; `tests/unit/validation_policy_tests.cpp:173-180`
**Apply to:** `tests/unit/archive_reader_dispatch_policy_tests.cpp`
```cpp
const auto archive_text = read_text_file(source_root() / "src/archive.cpp");
REQUIRE(archive_text.find("...") != std::string::npos);
REQUIRE(archive_text.find("forbidden token") == std::string::npos);
```
Keep the guard negative-only and method-scoped.

### Fixture-backed runtime suites iterate representative archives
**Source:** `tests/unit/tes4_bsa_reader_tests.cpp:47-53`, `194-207`; `tests/unit/ba2_gnrl_reader_tests.cpp:151-157`, `676-717`; `tests/unit/ba2_dx10_extraction_tests.cpp:143-162`
**Apply to:** `tests/unit/archive_reader_dispatch_tests.cpp`
```cpp
for (const auto& fixture : success_fixtures()) {
  auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
  REQUIRE(opened.has_value());
}
```
Use existing committed fixtures for TES3, TES4, FO4 BA2 GNRL, Starfield BA2 GNRL v3, and BA2 DX10.

## No Analog Found

None.

## Metadata

**Analog search scope:** `src/archive.cpp`, `src/formats/bsa/*_reader.*`, `src/formats/ba2/*_reader.*`, `tests/unit/*.cpp`, `tests/CMakeLists.txt`, `tests/fixtures/generated/archives/`
**Files scanned:** 16 code/test files plus planning artifacts
**Pattern extraction date:** 2026-05-14
