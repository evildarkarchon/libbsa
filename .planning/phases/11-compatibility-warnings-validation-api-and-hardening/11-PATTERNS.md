# Phase 11: compatibility-warnings-validation-api-and-hardening - Pattern Map

**Mapped:** 2026-05-10
**Files analyzed:** 20
**Analogs found:** 20 / 20

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/validation.hpp` | model | request-response | `include/libbsa/archive.hpp`, `include/libbsa/result.hpp` | role-match |
| `include/libbsa/libbsa.hpp` | config | transform | `include/libbsa/libbsa.hpp` | exact |
| `src/validation.cpp` | service | file-I/O, request-response | `src/archive.cpp` | role-match |
| `CMakeLists.txt` | config | batch | `CMakeLists.txt` | exact |
| `tests/CMakeLists.txt` | config | batch | `tests/CMakeLists.txt` | exact |
| `tests/unit/validation_api_tests.cpp` | test | file-I/O, request-response | `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp` | role-match |
| `tests/unit/compatibility_warning_tests.cpp` | test | file-I/O, request-response | `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp` | role-match |
| `tests/unit/compatibility_evidence_tests.cpp` | test | batch, transform | `tests/unit/validation_policy_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp` | role-match |
| `tests/unit/public_include_boundary_tests.cpp` | test | transform | `tests/unit/public_include_boundary_tests.cpp` | exact |
| `tests/package-consumer/main.cpp` | test | request-response | `tests/package-consumer/main.cpp` | exact |
| `tests/unit/validation_policy_tests.cpp` | test | batch | `tests/unit/validation_policy_tests.cpp` | exact |
| `tests/unit/local_game_fixture_tests.cpp` | test | file-I/O, request-response | `tests/unit/local_game_fixture_tests.cpp` | exact |
| `tests/fixtures/generated/compatibility_matrix.json` | config | batch | `tests/fixtures/generated/archives/*_malformed_manifest.json` | role-match |
| `tests/fixtures/generated/validate_fixture_manifests.py` | utility | batch, transform | `tests/fixtures/generated/validate_fixture_manifests.py` | exact |
| `tests/fixtures/README.md` | config | transform | `tests/fixtures/README.md` | exact |
| `docs/compatibility-evidence.md` | config | transform | `tests/fixtures/README.md` | role-match |
| `CMakePresets.json` | config | batch | `CMakePresets.json` | exact |
| `.github/workflows/ci.yml` | config | batch | `.github/workflows/ci.yml` | exact; preserve/conditional |
| `tests/unit/tes3_bsa_reader_tests.cpp` | test | file-I/O, request-response | `tests/unit/ba2_dx10_malformed_tests.cpp` | role-match |
| `tests/unit/tes4_bsa_reader_tests.cpp` | test | file-I/O, request-response | `tests/unit/ba2_dx10_malformed_tests.cpp` | role-match |
| `tests/unit/ba2_gnrl_reader_tests.cpp` | test | file-I/O, request-response | `tests/unit/ba2_dx10_malformed_tests.cpp` | role-match |

## Pattern Assignments

### `include/libbsa/validation.hpp` (model, request-response)

**Analog:** `include/libbsa/result.hpp`; secondary analog `include/libbsa/archive.hpp`.

**Imports pattern** (`include/libbsa/archive.hpp` lines 3-12):
```cpp
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

**Stable public enum pattern** (`include/libbsa/result.hpp` lines 10-20):
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
```

**Public diagnostic contract pattern** (`include/libbsa/result.hpp` lines 22-29):
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

**Metadata reuse pattern** (`include/libbsa/archive.hpp` lines 120-134):
```cpp
/// Archive-level metadata exposed by an opened reader.
///
/// The structure is intentionally limited to stable Phase 3 fields: container
/// type, archive variant/version, raw archive flags, file count, the default
/// compression behavior advertised by the archive family, and optional
/// format-family metadata that remains dependency-light.
struct archive_metadata {
  archive_type type;
  archive_variant variant;
  std::uint32_t version;
  std::uint32_t archive_flags;
  std::uint32_t file_count;
  entry_compression default_compression;
  std::optional<ba2_archive_metadata> ba2;
};
```

**Validation API shape to copy:** define `compatibility_warning_code`, `compatibility_warning_severity`, `compatibility_warning`, `validation_options`, `validation_report`, and `result<validation_report> validate_archive(std::string_view host_path, validation_options options = {})`. Keep this header public-only: standard library plus `archive.hpp`/`result.hpp`; no private parser, codec, DirectXTex, DXGI, Windows, or TES5Edit types.

---

### `include/libbsa/libbsa.hpp` (config, transform)

**Analog:** `include/libbsa/libbsa.hpp`.

**Umbrella include pattern** (lines 1-6):
```cpp
#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>
#include <libbsa/version.hpp>
#include <libbsa/writer.hpp>
```

**Apply:** add `#include <libbsa/validation.hpp>` in this same short umbrella style.

---

### `src/validation.cpp` (service, file-I/O/request-response)

**Analog:** `src/archive.cpp`.

**Imports pattern** (`src/archive.cpp` lines 1-18):
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

#include <cstddef>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
```

**Host-path setup failure pattern** (`src/archive.cpp` lines 56-80):
```cpp
result<std::vector<std::byte>> read_detection_prefix(std::string_view host_path) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }

  std::vector<std::byte> bytes(36U);
  input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (input.bad()) {
    return error{error_code::io_error, "failed while reading archive host path"};
  }
  bytes.resize(static_cast<std::size_t>(input.gcount()));
  return bytes;
}

result<std::uint64_t> archive_file_size(std::string_view host_path) {
  std::ifstream input{std::string{host_path}, std::ios::binary | std::ios::ate};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
```

**Strict parser dispatch pattern** (`src/archive.cpp` lines 115-190):
```cpp
result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  auto prefix = read_detection_prefix(host_path);
  if (!prefix) {
    return prefix.error();
  }

  if (prefix.value().size() >= 4U && prefix.value()[0] == static_cast<std::byte>(static_cast<unsigned char>('B')) &&
      prefix.value()[1] == static_cast<std::byte>(static_cast<unsigned char>('T')) &&
      prefix.value()[2] == static_cast<std::byte>(static_cast<unsigned char>('D')) &&
      prefix.value()[3] == static_cast<std::byte>(static_cast<unsigned char>('X'))) {
    auto detected_ba2 = formats::ba2::detect_ba2_format(prefix.value());
    if (!detected_ba2) {
      return detected_ba2.error();
    }
```

```cpp
  auto detected = formats::bsa::detect_bsa_format(prefix.value());
  if (!detected) {
    return detected.error();
  }

  auto archive_size = archive_file_size(host_path);
  if (!archive_size) {
    return archive_size.error();
  }
```

**Extraction-validation pattern** (`src/archive.cpp` lines 240-289):
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
```

**Apply:** `validate_archive` should reuse strict `archive_reader::open` as the source of truth. Return `result` failures for empty host paths and unreadable host files. For inspectable archive failures (`unsupported`, `format_error` after bytes can be read), return a `validation_report` with `errors` populated and no usable reader. For valid archives, populate `metadata`, optionally iterate entries/extraction when requested, and append compatibility warnings from private policy helpers.

---

### `CMakeLists.txt` (config, batch)

**Analog:** `CMakeLists.txt`.

**Public header/source registration pattern** (lines 44-81):
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
        include/libbsa/writer.hpp
  PRIVATE
    src/archive.cpp
    src/detail/archive_path.cpp
    src/detail/bethesda_hash.cpp
```

**Apply:** add `include/libbsa/validation.hpp` to the public file set and `src/validation.cpp` to private sources. Do not add new public dependencies.

---

### `tests/CMakeLists.txt` (config, batch)

**Analog:** `tests/CMakeLists.txt`.

**Test source list pattern** (lines 4-29):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/ba2_gnrl_writer_tests.cpp
  unit/tes3_bsa_writer_tests.cpp
  unit/tes4_bsa_writer_tests.cpp
  unit/tes4_bsa_reader_tests.cpp
```

**Catch2 label discovery pattern** (lines 225-230):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Package consumer test pattern** (lines 232-241):
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

**Apply:** add new unit test `.cpp` files to `libbsa_tests`. If adding a Python matrix/catalog check, add it as an explicit `add_test` or invoke it from a C++ test; keep Catch2 tags so `malformed`, `compat`, `validation`, and `requires-game-fixture` labels remain selectable.

---

### `tests/unit/validation_api_tests.cpp` (test, file-I/O/request-response)

**Analog:** `tests/unit/tes4_bsa_writer_tests.cpp`; secondary analogs `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp`.

**Writer output -> strict open -> metadata pattern** (`tests/unit/tes4_bsa_writer_tests.cpp` lines 184-225):
```cpp
libbsa::tes4_bsa_writer_options options;
options.compression_policy = libbsa::archive_compression_policy::all_raw;
libbsa::tes4_bsa_writer writer{target, options};

const auto model_source = root / "sources" / "model.nif";
const auto diffuse_source = root / "sources" / "diffuse.dds";
write_binary_file(model_source, expected_entries[0].second);
write_binary_file(diffuse_source, expected_entries[1].second);

REQUIRE(writer.add_file(expected_entries[0].first, model_source.string()).has_value());
REQUIRE(writer.add_file(expected_entries[1].first, diffuse_source.string()).has_value());
REQUIRE(writer.add_bytes(expected_entries[2].first, expected_entries[2].second).has_value());

auto written = writer.write_to(output.string());
REQUIRE(written.has_value());

auto opened = libbsa::archive_reader::open(output.string());
REQUIRE(opened.has_value());

auto metadata = opened.value().metadata();
REQUIRE(metadata.has_value());
CHECK(metadata.value().type == libbsa::archive_type::bsa);
```

**TES3 writer-output metadata pattern** (`tests/unit/tes3_bsa_writer_tests.cpp` lines 401-425):
```cpp
libbsa::tes3_bsa_writer_options options;
options.overwrite_existing = true;
libbsa::tes3_bsa_writer writer{options};
REQUIRE(writer.add_file("Meshes/Disk/Probe.NIF", disk_source.string()).has_value());
REQUIRE(writer.add_bytes("textures\\Memory\\Probe.dds", memory_bytes).has_value());
REQUIRE(writer.add_bytes("Readme.txt", std::span<const std::byte>{}).has_value());

REQUIRE(writer.write_to(archive.string()).has_value());

auto opened = libbsa::archive_reader::open(archive.string());
REQUIRE(opened.has_value());
auto metadata = opened.value().metadata();
REQUIRE(metadata.has_value());
CHECK(metadata.value().type == libbsa::archive_type::bsa);
CHECK(metadata.value().variant == libbsa::archive_variant::tes3);
```

**BA2 DX10 writer validation pattern** (`tests/unit/ba2_dx10_writer_tests.cpp` lines 347-401):
```cpp
void require_writer_round_trip(libbsa::ba2_dx10_target target,
                               std::uint32_t starfield_compression_method,
                               std::string_view stem,
                               libbsa::entry_compression expected_compression) {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  libbsa::ba2_dx10_writer_options options;
  options.starfield_compression_method = starfield_compression_method;
  libbsa::ba2_dx10_writer writer{target, options};
  std::vector<const nlohmann::json*> added_cases;
  add_matrix_cases(writer, manifest, target, added_cases);
  REQUIRE_FALSE(added_cases.empty());

  const auto output_path = unique_output_path(stem);
  auto written = writer.write_to(output_path.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(output_path.string());
  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
```

**Apply:** mirror these tests with `auto validated = libbsa::validate_archive(output.string(), options); REQUIRE(validated.has_value()); CHECK(validated.value().is_valid());` and assert metadata/errors/warnings from the report. Cover generated success fixtures and writer-produced TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10 outputs.

---

### `tests/unit/compatibility_warning_tests.cpp` (test, request-response)

**Analog:** `tests/unit/public_include_boundary_tests.cpp`; secondary analog `tests/unit/tes4_bsa_writer_tests.cpp`.

**Programmatic-code-over-message pattern** (`include/libbsa/result.hpp` lines 22-25):
```cpp
/// `code` is stable and suitable for tests and programmatic handling. `message`
/// is diagnostic text for humans and should not be compared exactly by tests.
```

**Public contract assertion pattern** (`tests/unit/public_include_boundary_tests.cpp` lines 50-57):
```cpp
// BEGIN tes3_bsa_public_contract_assertions
static_assert(requires(libbsa::tes3_bsa_writer& writer, std::span<const std::byte> bytes) {
  { writer.options() } -> std::same_as<const libbsa::tes3_bsa_writer_options&>;
  { writer.add_bytes("Meshes/Memory.nif", bytes) } -> std::same_as<libbsa::result<void>>;
  { writer.add_file("Textures/Disk.dds", "source.dds") } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.bsa") } -> std::same_as<libbsa::result<void>>;
});
// END tes3_bsa_public_contract_assertions
```

**Warning scenario fixture pattern:** use writer-produced valid archives or generated valid fixtures, then assert warning `code`, `severity`, and optional `archive_path`. Do not assert exact `message`.

**Recommended first warning scenarios:** BSA embedded-name target risk, compressed sound payload risk, and BA2 target-family mismatch. Keep exact names discretionary, but every introduced `compatibility_warning_code` must appear in `docs/compatibility-evidence.md`.

---

### `tests/unit/compatibility_evidence_tests.cpp` (test, batch/transform)

**Analog:** `tests/unit/validation_policy_tests.cpp`; secondary analog `tests/unit/public_include_boundary_tests.cpp`.

**Read-text helper pattern** (`tests/unit/validation_policy_tests.cpp` lines 17-24):
```cpp
std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}
```

**Documentation coverage assertion pattern** (`tests/unit/validation_policy_tests.cpp` lines 36-52):
```cpp
TEST_CASE("CTest label taxonomy is documented and backed by selectable tests", "[unit][fixture][roundtrip][compat][malformed][slow]") {
  const auto readme = read_text_file(source_root() / "tests/fixtures/README.md");

  constexpr std::array<std::string_view, 7> required_labels{
    "unit",
    "fixture",
    "roundtrip",
    "compat",
    "malformed",
    "slow",
    "requires-game-fixture",
  };

  for (const auto label : required_labels) {
    INFO("Missing documented label: " << label);
    REQUIRE(readme.find(std::string{label}) != std::string::npos);
  }
}
```

**Header scan pattern** (`tests/unit/public_include_boundary_tests.cpp` lines 161-188):
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
```

**Apply:** explicitly list public warning codes in the test, read `docs/compatibility-evidence.md`, and require a catalog row/section for each code plus an evidence reference. Because C++20 has no enum reflection, keep the list explicit and fail loudly when new codes are added.

---

### `docs/compatibility-evidence.md` (config, transform)

**Analog:** `tests/fixtures/README.md`.

**Provenance/evidence pattern** (`tests/fixtures/README.md` lines 94-102):
```markdown
## Provenance requirements

Each committed generated fixture must document:

1. The generator or source recipe used to create it.
2. The legal provenance that makes it safe to commit.
3. The behavior it proves, such as header parsing, fixture extraction,
   round-trip metadata, compatibility warnings, or malformed-input handling.
```

**TES5Edit boundary pattern** (`tests/fixtures/README.md` lines 103-109):
```markdown
## TES5Edit boundary

TES5Edit/ must not be used as a fixture workspace, mutable test data location,
or source of committed fixture files. It is a read-only behavior reference only.

Do not edit, format, compile, stage, or copy generated outputs into `TES5Edit/`
while preparing libbsa tests.
```

**Apply:** create one row/section per warning code with stable code name, severity, rule description, affected archive family, evidence link, and whether evidence is generated fixture, writer-output test, TES5Edit/BSArchPro note, or optional local corpus check.

---

### `tests/fixtures/generated/compatibility_matrix.json` (config, batch)

**Analog:** malformed manifests under `tests/fixtures/generated/archives/`.

**Manifest shape pattern** (`tests/fixtures/generated/archives/ba2_dx10_malformed_manifest.json` lines 1-21):
```json
{
  "manifest_kind": "malformed_ba2_dx10_cases",
  "requirements": ["DDS-01", "DDS-02", "DDS-03", "DDS-04", "DDS-05", "DDS-06", "DDS-07"],
  "threat_references": ["T-06-01", "T-06-02", "T-06-03", "T-06-04"],
  "provenance": {
    "generator": "tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp",
    "source": "synthetic malformed texture bytes generated for libbsa tests; no game or TES5Edit bytes copied"
  },
  "cases": [
    {"id": "ba2_dx10_truncated_header", "archive": "ba2_dx10_truncated_header.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_truncated_records", "archive": "ba2_dx10_truncated_records.ba2", "expected_error": "format_error", "phase": "open"}
  ]
}
```

**Detailed case pattern** (`tests/fixtures/generated/archives/malformed_manifest.json` lines 37-47):
```json
{
  "id": "duplicate_canonical_path",
  "archive": "malformed_duplicate_canonical_path.bsa",
  "requirements": ["FMT-03", "FMT-04"],
  "threat_references": ["T-03-03"],
  "expected_error": "format_error",
  "phase": "open",
  "canonical_path": "meshes/dupe/same.txt",
  "original_paths": ["Meshes\\Dupe\\Same.txt", "Meshes\\Dupe\\same.TXT"],
  "description": "Two archive records differ by case but normalize to the same public key"
}
```

**Apply:** use a consolidated matrix keyed by row ID, archive family (`tes3`, `tes4_bsa`, `ba2_gnrl`, `ba2_dx10`, `compression`, `dds_layout`), phase (`open`, `extraction`, `validation`), expected public `error_code`, and evidence references to existing generated manifests or new writer-output tests.

---

### `tests/fixtures/generated/validate_fixture_manifests.py` (utility, batch/transform)

**Analog:** `tests/fixtures/generated/validate_fixture_manifests.py`.

**Schema constants pattern** (lines 12-48):
```python
SUCCESS_MANIFESTS = ("tes4_v103", "tes4_v104", "tes4_v105")
SUCCESS_REQUIRED_TOP_LEVEL = {
    "variant",
    "version",
    "flags",
    "file_count",
    "folder",
    "provenance",
    "entries",
}
MALFORMED_CASE_IDS = {
    "unsupported_version",
    "truncated_header",
    "truncated_table",
    "duplicate_canonical_path",
    "corrupt_compressed_payload",
    "size_mismatch",
    "non_bsa_bytes",
}
EXPECTED_ERROR_CATEGORIES = {"unsupported", "format_error"}
```

**Validation helper pattern** (lines 51-64):
```python
def load_json(path: Path) -> dict[str, Any]:
    """Load a manifest as a JSON object and fail if the root shape is not an object."""
    with path.open("r", encoding="utf-8") as manifest_file:
        value = json.load(manifest_file)
    if not isinstance(value, dict):
        raise AssertionError(f"{path.name}: manifest root must be an object")
    return value


def require_keys(mapping: dict[str, Any], required: set[str], label: str) -> None:
    """Assert that a JSON object contains the required field names."""
    missing = sorted(required.difference(mapping))
```

**Unknown expected-error rejection pattern** (lines 86-109):
```python
def validate_malformed_manifest(archives_dir: Path) -> None:
    """Validate malformed fixture case IDs, expected errors, and referenced archive files."""
    manifest = load_json(archives_dir / "malformed_manifest.json")
    require_keys(manifest, {"manifest_kind", "requirements", "threat_references", "provenance", "cases"}, "malformed")
    cases = manifest["cases"]
    if not isinstance(cases, list):
        raise AssertionError("malformed: cases must be a list")
```

**Apply:** extend with `validate_compatibility_matrix()` and/or `validate_compatibility_evidence()` using the same `AssertionError` style. Require all matrix rows to reference existing generated manifests/tests and reject unknown `expected_error` values.

---

### `tests/unit/tes3_bsa_reader_tests.cpp` (test, file-I/O/request-response)

**Analog:** `tests/unit/ba2_dx10_malformed_tests.cpp`.

**Current helper to fix** (`tests/unit/tes3_bsa_reader_tests.cpp` lines 55-63):
```cpp
libbsa::error_code error_code_from_manifest(std::string_view value) {
  if (value == "format_error") {
    return libbsa::error_code::format_error;
  }
  if (value == "unsupported") {
    return libbsa::error_code::unsupported;
  }
  return libbsa::error_code::invalid_argument;
}
```

**Copy strict helper pattern from BA2 DX10** (`tests/unit/ba2_dx10_malformed_tests.cpp` lines 31-40):
```cpp
libbsa::error_code error_code_from_manifest(std::string_view value) {
  if (value == "format_error") {
    return libbsa::error_code::format_error;
  }
  if (value == "unsupported") {
    return libbsa::error_code::unsupported;
  }
  FAIL("unknown BA2 DX10 malformed expected_error: " << value);
  return libbsa::error_code::format_error;
}
```

**Malformed loop pattern** (`tests/unit/tes3_bsa_reader_tests.cpp` lines 344-381):
```cpp
TEST_CASE("tes3_bsa_malformed rejects generated malformed TES3 cases with stable error codes",
          "[unit][fixture][malformed][tes3_bsa_malformed]") {
  const auto manifest = read_json_file(generated_archive_path("tes3_malformed_manifest.json"));
  const std::vector<std::string> required_case_ids{
      "tes3_truncated_header",
      "tes3_truncated_records",
      "tes3_invalid_name_span",
      "tes3_invalid_payload_span",
      "tes3_duplicate_canonical_path",
      "tes3_inconsistent_counts_offsets",
```

**Apply:** replace silent fallback with `FAIL(...)`, keep stable `error_code` assertions, and add matrix row coverage if the planner chooses C++ matrix tests.

---

### `tests/unit/tes4_bsa_reader_tests.cpp` (test, file-I/O/request-response)

**Analog:** `tests/unit/ba2_dx10_malformed_tests.cpp`.

**Current helper to fix** (`tests/unit/tes4_bsa_reader_tests.cpp` lines 51-59):
```cpp
libbsa::error_code error_code_from_manifest(std::string_view value) {
  if (value == "format_error") {
    return libbsa::error_code::format_error;
  }
  if (value == "unsupported") {
    return libbsa::error_code::unsupported;
  }
  return libbsa::error_code::invalid_argument;
}
```

**Malformed open/extraction patterns** (`tests/unit/tes4_bsa_reader_tests.cpp` lines 180-198 and 449-464):
```cpp
TEST_CASE("tes4_bsa_malformed_open rejects malformed open-phase fixtures with stable error codes",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
  std::ifstream manifest_stream{generated_archive_path("malformed_manifest.json")};
  const auto manifest = nlohmann::json::parse(manifest_stream);

  for (const auto& test_case : manifest.at("cases")) {
    if (test_case.at("phase").get<std::string>() != "open") {
      continue;
    }
```

```cpp
TEST_CASE("tes4_bsa_compression_routing rejects corrupt compressed payloads and size mismatches",
          "[unit][fixture][malformed][tes4_bsa_compression_routing]") {
  const auto manifest = read_json_file(generated_archive_path("malformed_manifest.json"));
  for (const auto& test_case : manifest.at("cases")) {
    if (test_case.at("phase").get<std::string>() != "extraction") {
      continue;
    }
```

**Apply:** copy BA2 DX10's `FAIL(...)` unknown mapping and keep phase-based open/extraction branching for matrix coverage.

---

### `tests/unit/ba2_gnrl_reader_tests.cpp` (test, file-I/O/request-response)

**Analog:** `tests/unit/ba2_dx10_malformed_tests.cpp`.

**Current helper to fix** (`tests/unit/ba2_gnrl_reader_tests.cpp` lines 42-50):
```cpp
libbsa::error_code error_code_from_manifest(std::string_view value) {
  if (value == "format_error") {
    return libbsa::error_code::format_error;
  }
  if (value == "unsupported") {
    return libbsa::error_code::unsupported;
  }
  return libbsa::error_code::invalid_argument;
}
```

**Malformed branch pattern** (`tests/unit/ba2_gnrl_reader_tests.cpp` lines 529-570):
```cpp
TEST_CASE("ba2_gnrl_malformed manifest cases fail with stable error codes", "[unit][fixture][ba2_gnrl_malformed]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_malformed_manifest.json"));
  constexpr auto required_cases = std::to_array<std::string_view>({"ba2_unsupported_v3_compression_method",
                                                                  "ba2_duplicate_canonical_path",
                                                                  "ba2_corrupt_compressed_payload",
                                                                  "ba2_exact_size_mismatch"});
  std::vector<std::string> observed_cases;

  for (const auto& test_case : manifest.at("cases")) {
```

**Apply:** copy BA2 DX10's `FAIL(...)` unknown mapping and keep the extraction-phase branch for decompressor matrix rows.

---

### `tests/unit/public_include_boundary_tests.cpp` (test, transform)

**Analog:** `tests/unit/public_include_boundary_tests.cpp`.

**Static public type pattern** (lines 16-31):
```cpp
static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");
static_assert(std::is_enum_v<libbsa::archive_type>);
static_assert(std::is_enum_v<libbsa::archive_variant>);
static_assert(std::is_enum_v<libbsa::entry_compression>);
static_assert(std::is_enum_v<libbsa::tes4_bsa_target>);
static_assert(std::is_enum_v<libbsa::archive_compression_policy>);
static_assert(std::is_enum_v<libbsa::entry_compression_policy>);
static_assert(std::is_class_v<libbsa::tes3_bsa_writer_options>);
static_assert(std::is_class_v<libbsa::tes3_bsa_writer>);
```

**Forbidden public dependency pattern** (lines 161-188):
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                   "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
```

**Apply:** add static assertions for `validation_options`, `validation_report`, `compatibility_warning_code`, `compatibility_warning_severity`, and `validate_archive` returning `libbsa::result<libbsa::validation_report>`. Keep validation public headers inside the existing forbidden-token scan.

---

### `tests/package-consumer/main.cpp` (test, request-response)

**Analog:** `tests/package-consumer/main.cpp`.

**Installed consumer smoke pattern** (lines 1-10):
```cpp
#include <libbsa/libbsa.hpp>

int main() {
  auto result = libbsa::archive_reader::open("consumer-smoke.bsa");
  if (result) {
    return 1;
  }

  return result.error().code == libbsa::error_code::io_error ? 0 : 1;
}
```

**Apply:** call `libbsa::validate_archive("consumer-smoke.bsa")` instead of or beside `archive_reader::open`. Missing file should remain a result-level `io_error` so installed consumers can branch on stable public `error_code`.

---

### `tests/unit/validation_policy_tests.cpp` (test, batch)

**Analog:** `tests/unit/validation_policy_tests.cpp`.

**Local fixture policy pattern** (lines 55-61):
```cpp
TEST_CASE("requires-game-fixture label is selectable without local archives", "[unit][requires-game-fixture]") {
  const auto readme = read_text_file(source_root() / "tests/fixtures/README.md");

  REQUIRE(readme.find("Tests discovered by default") != std::string::npos);
  REQUIRE(readme.find("skipped unless local") != std::string::npos);
  REQUIRE(readme.find("LIBBSA_GAME_FIXTURES") != std::string::npos);
}
```

**CI/preset preservation pattern** (lines 89-105):
```cpp
TEST_CASE("CI and presets preserve static shared and TES5Edit build boundaries", "[unit][public-api]") {
  const auto root = source_root();
  const auto presets = read_text_file(root / "CMakePresets.json");
  const auto workflow = read_text_file(root / ".github/workflows/ci.yml");

  REQUIRE(presets.find("windows-msvc-debug-static") != std::string::npos);
  REQUIRE(presets.find("windows-msvc-debug-shared") != std::string::npos);
  REQUIRE(presets.find("\"BUILD_SHARED_LIBS\": \"OFF\"") != std::string::npos);
  REQUIRE(presets.find("\"BUILD_SHARED_LIBS\": \"ON\"") != std::string::npos);
```

**Apply:** extend this test to assert sanitizer path documentation/preset exists while preserving static/shared Windows and `TES5Edit` checks.

---

### `tests/unit/local_game_fixture_tests.cpp` (test, request-response)

**Analog:** `tests/unit/local_game_fixture_tests.cpp`.

**Skipped-by-default optional local data pattern** (lines 6-13):
```cpp
TEST_CASE("local game fixtures are opt-in", "[requires-game-fixture][unit]") {
  const char* fixture_root = std::getenv("LIBBSA_GAME_FIXTURES");
  if (fixture_root == nullptr || std::string_view{fixture_root}.empty()) {
    SKIP("Set LIBBSA_GAME_FIXTURES or place local game archives under tests/fixtures/local; these files are not committed.");
  }

  REQUIRE_FALSE(std::string_view{fixture_root}.empty());
}
```

**Apply:** optional corpus smoke/compare tests should use this exact `std::getenv` plus `SKIP` shape, include `[requires-game-fixture]`, and never become default acceptance gates.

---

### `tests/fixtures/README.md` (config, transform)

**Analog:** `tests/fixtures/README.md`.

**Fixture directory and legal split pattern** (lines 1-18):
```markdown
# libbsa Fixture Policy

This directory separates committed legal fixtures from local game-derived data.
The goal is to make compatibility tests reproducible without committing
copyrighted Bethesda archives or mutating the `TES5Edit/` reference submodule.

## Committed generated fixtures

Committed fixtures must be tiny, legal, and generated specifically for tests.

- Source inputs belong under `tests/fixtures/generated/source`.
- Generated archive outputs belong under `tests/fixtures/generated/archives`.
```

**Label taxonomy pattern** (lines 111-122):
```markdown
## Test labels

CTest labels mirror Catch2 tags and use the following taxonomy:

- `unit` - Small deterministic tests for public and internal units.
- `fixture` - Tests using committed legal generated fixtures.
- `roundtrip` - Pack/open/extract comparisons for writer phases.
- `compat` - Compatibility checks against BSArchPro-derived or official-tool data.
- `malformed` - Invalid, truncated, oversized, or inconsistent archive inputs.
- `slow` - Longer-running tests that are not part of the quick path.
- `requires-game-fixture` - Tests discovered by default but skipped unless local
  game-derived data is available.
```

**Apply:** update with compatibility evidence catalog/matrix regeneration and sanitizer command path. Keep legal fixture, local data, and TES5Edit policy language intact.

---

### `CMakePresets.json` (config, batch)

**Analog:** `CMakePresets.json`.

**Additive preset pattern** (lines 8-35, 36-66):
```json
"configurePresets": [
  {
    "name": "windows-msvc-debug-static",
    "displayName": "Windows MSVC Debug Static",
    "binaryDir": "${sourceDir}/build/${presetName}",
    "cacheVariables": {
      "CMAKE_BUILD_TYPE": "Debug",
      "CMAKE_CXX_STANDARD": "20",
      "CMAKE_CXX_STANDARD_REQUIRED": "ON",
      "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
      "LIBBSA_BUILD_TESTS": "ON",
      "BUILD_SHARED_LIBS": "OFF"
    }
  }
]
```

```json
"testPresets": [
  {
    "name": "windows-msvc-debug-static",
    "configurePreset": "windows-msvc-debug-static",
    "configuration": "Debug",
    "output": {
      "outputOnFailure": true
    }
  }
]
```

**Apply:** add a non-Windows Clang/GCC sanitizer-oriented configure/build/test path only if the planner chooses preset over documentation-only. Do not modify existing Windows static/shared presets except to preserve them.

---

### `.github/workflows/ci.yml` (config, batch; preserve/conditional)

**Analog:** `.github/workflows/ci.yml`.

**Default Windows matrix pattern** (lines 8-34):
```yaml
windows-msvc:
  name: Windows MSVC (${{ matrix.preset }})
  runs-on: windows-latest
  strategy:
    fail-fast: false
    matrix:
      preset:
        - windows-msvc-debug-static
        - windows-msvc-debug-shared

  env:
    VCPKG_ROOT: C:\vcpkg
```

**TES5Edit status check pattern** (lines 36-43):
```yaml
- name: Verify TES5Edit stayed read-only
  shell: pwsh
  run: |
    $status = git status --short TES5Edit
    if ($status) {
      $status
      throw "TES5Edit submodule changed during CI"
    }
```

**Apply:** do not change default Windows matrix behavior. If adding sanitizer CI, make it a separate additive, toolchain-gated lane. Preserve the `TES5Edit` status check.

## Shared Patterns

### Public Header Boundary

**Source:** `tests/unit/public_include_boundary_tests.cpp` lines 161-188 and `include/libbsa/archive.hpp` lines 33-42.
**Apply to:** `include/libbsa/validation.hpp`, `include/libbsa/libbsa.hpp`, `tests/unit/public_include_boundary_tests.cpp`.

```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                   "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
```

### Stable Codes, Human Messages

**Source:** `include/libbsa/result.hpp` lines 22-25.
**Apply to:** validation errors, compatibility warnings, and all validation/warning tests.

```cpp
/// `code` is stable and suitable for tests and programmatic handling. `message`
/// is diagnostic text for humans and should not be compared exactly by tests.
```

### Strict Open Remains Source Of Truth

**Source:** `include/libbsa/archive.hpp` lines 178-184 and `src/archive.cpp` lines 115-190.
**Apply to:** `src/validation.cpp`, `tests/unit/validation_api_tests.cpp`, malformed tests.

```cpp
/// Empty host paths return `error_code::invalid_argument`, missing or
/// unreadable files return `error_code::io_error`, unsupported archive bytes
/// return `error_code::unsupported`, and malformed supported archives return
/// `error_code::format_error`.
static result<archive_reader> open(std::string_view host_path);
```

### Manifest-Driven Malformed Tests

**Source:** `tests/unit/ba2_dx10_malformed_tests.cpp` lines 56-85.
**Apply to:** all malformed reader tests and validation API malformed-report tests.

```cpp
for (const auto& test_case : manifest.at("cases")) {
  const auto id = test_case.at("id").get<std::string>();
  observed_cases.push_back(id);
  const auto archive = generated_archive_path(test_case.at("archive").get<std::string>()).string();
  const auto expected_error = error_code_from_manifest(test_case.at("expected_error").get<std::string>());
  const auto phase = test_case.at("phase").get<std::string>();

  INFO("BA2 DX10 malformed case: " << id);
  if (phase == "open") {
    auto opened = libbsa::archive_reader::open(archive);

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == expected_error);
    continue;
  }
```

### Unknown Manifest Values Must Fail

**Source:** `tests/unit/ba2_dx10_malformed_tests.cpp` lines 31-40.
**Apply to:** `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, matrix validators.

```cpp
FAIL("unknown BA2 DX10 malformed expected_error: " << value);
return libbsa::error_code::format_error;
```

### Generated Fixture Provenance

**Source:** `tests/fixtures/README.md` lines 94-109.
**Apply to:** `docs/compatibility-evidence.md`, `tests/fixtures/generated/compatibility_matrix.json`, optional corpus docs.

```markdown
Each committed generated fixture must document:

1. The generator or source recipe used to create it.
2. The legal provenance that makes it safe to commit.
3. The behavior it proves, such as header parsing, fixture extraction,
   round-trip metadata, compatibility warnings, or malformed-input handling.
```

### Optional Local Corpus Tests

**Source:** `tests/unit/local_game_fixture_tests.cpp` lines 6-13.
**Apply to:** optional compatibility smoke/compare tests.

```cpp
const char* fixture_root = std::getenv("LIBBSA_GAME_FIXTURES");
if (fixture_root == nullptr || std::string_view{fixture_root}.empty()) {
  SKIP("Set LIBBSA_GAME_FIXTURES or place local game archives under tests/fixtures/local; these files are not committed.");
}
```

### CTest Labels

**Source:** `tests/CMakeLists.txt` lines 225-230 and `tests/fixtures/README.md` lines 111-122.
**Apply to:** new validation, compatibility warning, evidence, matrix, malformed, and optional local tests.

```cmake
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

### TES5Edit Boundary

**Source:** `tests/fixtures/README.md` lines 103-109, `.github/workflows/ci.yml` lines 36-43, `AGENTS.md`.
**Apply to:** all files.

```yaml
$status = git status --short TES5Edit
if ($status) {
  $status
  throw "TES5Edit submodule changed during CI"
}
```

## No Analog Found

All planned files have role or exact analogs in the current codebase. Planner should still treat the first private warning-policy implementation as a new internal helper; its public shape should be copied from `result.hpp`/`archive.hpp`, and its tests should be copied from writer-output and malformed test patterns above.

## Metadata

**Analog search scope:** `include/`, `src/`, `tests/`, `docs/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/ci.yml`.
**Excluded:** `TES5Edit/` was excluded from code search and not modified.
**Project skills:** no repo-local `.codex/skills/` or `.agents/skills/` directories were found.
**Files scanned:** 80+ source/test/config/doc paths via `rg --files` and targeted line-numbered reads.
**Pattern extraction date:** 2026-05-10
**TES5Edit status check:** `git status --short TES5Edit` produced no output during mapping.
