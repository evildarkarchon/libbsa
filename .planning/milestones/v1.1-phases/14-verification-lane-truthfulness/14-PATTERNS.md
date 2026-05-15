# Phase 14: Verification Lane Truthfulness - Pattern Map

**Mapped:** 2026-05-13
**Files analyzed:** 12
**Analogs found:** 12 / 12

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|---|---|---|---|---|
| `CMakePresets.json` | config | transform | `CMakePresets.json` | exact |
| `.github/workflows/ci.yml` | config | batch | `.github/workflows/ci.yml` | exact |
| `CMakeLists.txt` | config | transform | `CMakeLists.txt` | exact |
| `tests/CMakeLists.txt` | config | batch | `tests/CMakeLists.txt` | exact |
| `tests/package-consumer/smoke.cmake` | test | file-I/O | `tests/package-consumer/smoke.cmake` | exact |
| `tests/package-consumer/CMakeLists.txt` | config | file-I/O | `tests/package-consumer/CMakeLists.txt` | exact |
| `tests/unit/validation_policy_tests.cpp` | test | file-I/O | `tests/unit/validation_policy_tests.cpp` | exact |
| `README.md` | config | transform | `README.md` | exact |
| `tests/fixtures/README.md` | config | transform | `tests/fixtures/README.md` | exact |
| `.planning/PROJECT.md` | config | transform | `.planning/PROJECT.md` | exact |
| `.planning/ROADMAP.md` | config | transform | `.planning/ROADMAP.md` | exact |
| `.planning/STATE.md` | config | transform | `.planning/STATE.md` | exact |

## Pattern Assignments

### `CMakePresets.json` (config, transform)

**Analog:** `CMakePresets.json`

**Preset family pattern** (`CMakePresets.json:8-35`, `36-65`):
```json
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
```

**Copy:** add new lanes as full triads in `configurePresets`, `buildPresets`, and `testPresets`; keep lane names identical across all three sections.

---

### `.github/workflows/ci.yml` (config, batch)

**Analog:** `.github/workflows/ci.yml`

**Main matrix pattern** (`.github/workflows/ci.yml:8-17`):
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
```

**Step sequence pattern** (`.github/workflows/ci.yml:52-68`):
```yaml
- name: Configure
  run: cmake --preset ${{ matrix.preset }}

- name: Build
  run: cmake --build --preset ${{ matrix.preset }}

- name: Test
  run: ctest --preset ${{ matrix.preset }} --output-on-failure

- name: Verify TES5Edit stayed read-only
```

**Copy:** keep one shared step sequence for each supported lane; preserve `fail-fast: false`; split ASan into a separate named job rather than another matrix row.

---

### `CMakeLists.txt` (config, transform)

**Analog:** `CMakeLists.txt`

**Option declaration pattern** (`CMakeLists.txt:13-15`):
```cmake
option(LIBBSA_BUILD_TESTS "Build libbsa tests" ${BUILD_TESTING})
option(LIBBSA_BUILD_BENCHMARKS "Build libbsa benchmark tools" ON)
```

**Compiler-flag injection pattern** (`CMakeLists.txt:74-89`):
```cmake
target_compile_options(libbsa
  PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)
```

**Tests-gated subdirectory pattern** (`CMakeLists.txt:205-209`):
```cmake
if(BUILD_TESTING AND LIBBSA_BUILD_TESTS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  endif()
endif()
```

**Copy:** add ASan enablement as a checked-in CMake option/flag path, not as preset-only metadata; keep the existing option + guarded compiler-flag style.

---

### `tests/CMakeLists.txt` (config, batch)

**Analog:** `tests/CMakeLists.txt`

**Package smoke registration pattern** (`tests/CMakeLists.txt:284-303`):
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

set_tests_properties(package_consumer_smoke package_consumer_runtime_dll_copy PROPERTIES
  LABELS "package_consumer;target_format_policy"
)
```

**Shared-only proof pattern** (`tests/CMakeLists.txt:305-317`):
```cmake
if(WIN32 AND BUILD_SHARED_LIBS)
  add_test(
    NAME shared_export_surface
    COMMAND ${CMAKE_COMMAND}
      -DLIBBSA_DLL=$<TARGET_FILE:libbsa>
      -DLIBBSA_DUMPBIN_HINT=${CMAKE_LINKER}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/export-surface/check-dll-exports.cmake
  )
endif()
```

**Copy:** keep Release package proof inside `ctest` by reusing these tests; prefer labels/properties or preset ownership tweaks over inventing a new CI-only smoke path.

---

### `tests/package-consumer/smoke.cmake` (test, file-I/O)

**Analog:** `tests/package-consumer/smoke.cmake`

**Required-variable + fail-fast pattern** (`tests/package-consumer/smoke.cmake:1-13`):
```cmake
foreach(required_var IN ITEMS LIBBSA_BUILD_DIR LIBBSA_INSTALL_PREFIX CONSUMER_SOURCE_DIR CONSUMER_BUILD_DIR CONFIG)
  if(NOT DEFINED ${required_var})
    message(FATAL_ERROR "Missing required variable: ${required_var}")
  endif()
endforeach()

execute_process(
  COMMAND ${CMAKE_COMMAND} --install "${LIBBSA_BUILD_DIR}" --config "${CONFIG}" --prefix "${LIBBSA_INSTALL_PREFIX}"
  RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "libbsa install failed: ${install_result}")
endif()
```

**Install → configure → build → test pipeline** (`tests/package-consumer/smoke.cmake:25-47`):
```cmake
execute_process(COMMAND ${CMAKE_COMMAND} -S "${CONSUMER_SOURCE_DIR}" -B "${CONSUMER_BUILD_DIR}" "-DCMAKE_PREFIX_PATH=${consumer_prefix_path}")
execute_process(COMMAND ${CMAKE_COMMAND} --build "${CONSUMER_BUILD_DIR}" --config "${CONFIG}")
execute_process(COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CONSUMER_BUILD_DIR}" -C "${CONFIG}" --output-on-failure)
```

**Copy:** Release-lane proof should keep this exact staged flow and fatal-on-failure behavior.

---

### `tests/package-consumer/CMakeLists.txt` (config, file-I/O)

**Analog:** `tests/package-consumer/CMakeLists.txt`

**Minimal consumer pattern** (`tests/package-consumer/CMakeLists.txt:3-10`):
```cmake
project(libbsa_package_consumer LANGUAGES CXX)

find_package(libbsa CONFIG REQUIRED)

add_executable(libbsa_package_consumer main.cpp)
target_compile_features(libbsa_package_consumer PRIVATE cxx_std_20)
target_link_libraries(libbsa_package_consumer PRIVATE libbsa::libbsa)
```

**Shared-runtime handling pattern** (`tests/package-consumer/CMakeLists.txt:11-20`):
```cmake
# Keep the consumer test runnable for shared installs without relying on source-tree paths.
add_custom_command(TARGET libbsa_package_consumer POST_BUILD
  COMMAND ${CMAKE_COMMAND}
    "-DRUNTIME_DLLS=$<TARGET_RUNTIME_DLLS:libbsa_package_consumer>"
    "-DTARGET_DIR=$<TARGET_FILE_DIR:libbsa_package_consumer>"
    -P "${CMAKE_CURRENT_LIST_DIR}/copy-runtime-dlls.cmake"
  VERBATIM
)
```

**Copy:** preserve the minimal downstream `find_package` proof and the shared-runtime DLL-copy hook.

---

### `tests/unit/validation_policy_tests.cpp` (test, file-I/O)

**Analog:** `tests/unit/validation_policy_tests.cpp`

**Helper pattern** (`tests/unit/validation_policy_tests.cpp:17-50`):
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

**Independent surface assertions pattern** (`tests/unit/validation_policy_tests.cpp:165-213`):
```cpp
const auto presets = read_text_file(root / "CMakePresets.json");
const auto workflow = read_text_file(root / ".github/workflows/ci.yml");
const auto fixture_policy = read_text_file(root / "tests/fixtures/README.md");
const auto readme = read_text_file(root / "README.md");

REQUIRE(presets.find("windows-msvc-debug-static") != std::string::npos);
REQUIRE(workflow.find("windows-msvc-debug-static") != std::string::npos);
REQUIRE(fixture_policy.find("Windows-only") != std::string::npos);
REQUIRE(readme.find("Windows-only") != std::string::npos);
```

**Secondary analog for token helpers:** `tests/unit/docs_policy_tests.cpp:27-39`
```cpp
void require_all_tokens(std::string_view text, std::initializer_list<std::string_view> tokens) {
  for (const auto token : tokens) {
    INFO("Missing token: " << token);
    REQUIRE(text.find(token) != std::string_view::npos);
  }
}
```

**Copy:** build the Phase 14 truth gate around one shared expected-contract table/helper set, then validate each surface independently.

---

### `README.md` (config, transform)

**Analog:** `README.md`

**Quick-path command pattern** (`README.md:9-18`):
```markdown
## Build

Set `VCPKG_ROOT` to your vcpkg checkout, then use one of the supported Windows presets:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static --output-on-failure
```
```

**Supported-lane summary pattern** (`README.md:20-24`):
```markdown
The supported preset set is `windows-msvc-debug-static` and `windows-msvc-debug-shared`.

## Reference Boundary
```

**Copy:** keep the concrete quick path first, then expand into a concise role-grouped matrix instead of replacing the command-led style.

---

### `tests/fixtures/README.md` (config, transform)

**Analog:** `tests/fixtures/README.md`

**Opt-in local-corpus policy pattern** (`tests/fixtures/README.md:84-99`):
```markdown
- Local copies may be placed under ignored `tests/fixtures/local`.
- Larger local datasets may be stored outside the repository and referenced with
  the `LIBBSA_GAME_FIXTURES` environment variable.
- Tests that require local game data must be tagged `requires-game-fixture` and
  skipped by default when no local fixture path is configured.
```

**Platform-policy pattern** (`tests/fixtures/README.md:164-169`):
```markdown
## Platform policy

libbsa is Windows-only. Fixture, malformed-input, compression, validation, and
compatibility checks are maintained through the Windows MSVC static/shared
presets in `CMakePresets.json`.
```

**Copy:** preserve the opt-in corpus language and Windows-only tone; update only the supported-lane contract facts.

---

### `.planning/PROJECT.md` (config, transform)

**Analog:** `.planning/PROJECT.md`

**Milestone summary pattern** (`.planning/PROJECT.md:13-22`):
```markdown
## Current Milestone: v1.1 Hardening

**Goal:** Strengthen libbsa's reliability and maintainability...

**Target features:**
- Fix non-ASCII Windows host-path handling...
- Reconcile hardening-policy drift and add stronger verification coverage such as sanitizer and/or Release-mode lanes.
```

**Current-state chronology pattern** (`.planning/PROJECT.md:60-67`):
```markdown
v1.1 now shifts focus from feature completeness to hardening work...
Phase 13 is complete and verified...
The next focus is Phase 14's verification-lane truthfulness work.
```

**Copy:** keep this file summary-level and chronological; fix the v1.0 hardening-history claim here rather than copying full lane commands into planning.

---

### `.planning/ROADMAP.md` (config, transform)

**Analog:** `.planning/ROADMAP.md`

**Phase bullet pattern** (`.planning/ROADMAP.md:33-39`):
```markdown
### 🚧 v1.1 Hardening (In Progress)

- [x] **Phase 13: Host Path Correctness Boundary** ...
- [ ] **Phase 14: Verification Lane Truthfulness** - Make Release and ASan verification lanes real, runnable, and policy-aligned.
```

**Phase detail pattern** (`.planning/ROADMAP.md:69-77`):
```markdown
### Phase 14: Verification Lane Truthfulness
**Goal**: Maintainers can rely on the documented hardening lanes...
**Depends on**: Phase 13
**Requirements**: VER-01, VER-02, VER-03
**Success Criteria** ...
```

**Copy:** update roadmap summaries in the same short goal/requirements/success-criteria style; do not move detailed contract facts out of the phase context.

---

### `.planning/STATE.md` (config, transform)

**Analog:** `.planning/STATE.md`

**Frontmatter + position pattern** (`.planning/STATE.md:1-15`, `26-36`):
```yaml
status: planning
stopped_at: Phase 14 context gathered
last_updated: "2026-05-14T01:00:09.350Z"
```

```markdown
## Current Position

Phase: 14
Plan: Not started
Status: Ready to plan
```

**Concise current-note pattern** (`.planning/STATE.md:35-36`):
```markdown
Current note: repository state still contains uncommitted Phase 13 implementation changes...
```

**Copy:** keep `STATE.md` terse and session-oriented; add only a short truthful matrix note instead of lane-by-lane detail.

## Shared Patterns

### Shared Pattern: Repo-reading policy tests
**Sources:**
- `tests/unit/validation_policy_tests.cpp:17-50`
- `tests/unit/docs_policy_tests.cpp:27-39`

```cpp
std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

void require_all_tokens(std::string_view text, std::initializer_list<std::string_view> tokens) {
  for (const auto token : tokens) {
    INFO("Missing token: " << token);
    REQUIRE(text.find(token) != std::string_view::npos);
  }
}
```

**Apply to:** `tests/unit/validation_policy_tests.cpp`

**Use for:** one shared expected-contract helper/table plus per-surface token assertions.

### Shared Pattern: CTest-owned package proof
**Sources:**
- `tests/CMakeLists.txt:284-317`
- `tests/package-consumer/smoke.cmake:1-47`
- `tests/package-consumer/CMakeLists.txt:3-20`

```cmake
add_test(NAME package_consumer_smoke ...)
set_tests_properties(package_consumer_smoke package_consumer_runtime_dll_copy PROPERTIES
  LABELS "package_consumer;target_format_policy"
)

execute_process(COMMAND ${CMAKE_COMMAND} --install "${LIBBSA_BUILD_DIR}" ...)
execute_process(COMMAND ${CMAKE_COMMAND} -S "${CONSUMER_SOURCE_DIR}" -B "${CONSUMER_BUILD_DIR}" ...)
execute_process(COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CONSUMER_BUILD_DIR}" -C "${CONFIG}" --output-on-failure)
```

**Apply to:** `tests/CMakeLists.txt`, `README.md`, `.github/workflows/ci.yml`, `tests/unit/validation_policy_tests.cpp`

**Use for:** Release-lane ownership language and enforcement. Do not create a second smoke path.

### Shared Pattern: Full lane-family naming
**Sources:**
- `CMakePresets.json:8-65`
- `.github/workflows/ci.yml:8-17,52-59`

```text
same lane name in configure preset -> build preset -> test preset -> CI job invocation
```

**Apply to:** `CMakePresets.json`, `.github/workflows/ci.yml`, `README.md`, `tests/unit/validation_policy_tests.cpp`

**Use for:** `windows-msvc-release-static`, `windows-msvc-release-shared`, and `windows-msvc-asan-static`.

### Shared Pattern: Summary vs detailed planning surfaces
**Sources:**
- `.planning/PROJECT.md:13-22,60-67`
- `.planning/ROADMAP.md:69-77`
- `.planning/STATE.md:26-36`
- `14-CONTEXT.md:56-60`

```text
PROJECT/ROADMAP carry role-aware summary;
STATE stays session-oriented;
phase context holds the most detailed contract.
```

**Apply to:** `.planning/PROJECT.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`

## No Analog Found

None. Every planned surface already has a strong in-repo analog; this phase is mostly extension and truth-alignment work, not greenfield structure.

## Metadata

**Analog search scope:** repo root config, workflow, tests, package-consumer, README, and `.planning/` summary surfaces.

**Files scanned:** 16

**Pattern extraction date:** 2026-05-13
