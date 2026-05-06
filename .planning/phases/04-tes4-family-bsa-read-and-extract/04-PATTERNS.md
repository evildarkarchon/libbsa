# Phase 04 Pattern Map: TES4-Family BSA Read and Extract

## Analog Files

| Planned File | Closest Existing Analog | Pattern to Reuse |
|--------------|-------------------------|------------------|
| `include/libbsa/bsa.hpp` | `include/libbsa/archive_view.hpp`, `include/libbsa/compression.hpp` | Public Doxygen comments, libbsa-owned types, `result<T>` returns, no dependency headers. |
| `src/bsa_reader.cpp` | `src/detect.cpp`, `src/archive_view.cpp`, `src/compression.cpp` | Bounded little-endian parsing, short stable error messages, copied metadata, dispatcher reuse. |
| `src/bsa_reader.hpp` | `src/hash.hpp`, `src/compression/*_codec.hpp` | Private implementation declarations under `libbsa::detail`. |
| `tests/bsa_reader_tests.cpp` | `tests/detection_tests.cpp`, `tests/archive_view_tests.cpp`, `tests/compression_policy_tests.cpp` | In-memory byte builders, Catch2 `REQUIRE/CHECK`, explicit labels via CMake. |
| `tests/public_header_smoke.cpp` | existing file | Consumer-style include/use coverage that links only `libbsa::libbsa`. |
| `README.md` | existing Phase 02/03 sections | Phase-specific behavior plus exact local validation commands. |

## Concrete Code Patterns

### Public result-returning declaration

```cpp
[[nodiscard]] result<archive_summary> detect_archive(const byte_source& source);
```

### Bounded read failure style

```cpp
auto read = source.read_at(0, std::span<std::byte>{bytes}.first(static_cast<std::size_t>(count)));
if (!read.has_value()) {
    return failure<std::array<std::byte, 36>>({error_code::malformed_archive, "truncated archive header"});
}
```

### Archive view ownership pattern

```cpp
archive_view::archive_view(archive_summary summary, std::vector<entry_metadata> entries) : summary_(std::move(summary))
```

### CMake test registration pattern

```cmake
add_executable(libbsa_bsa_reader_tests tests/bsa_reader_tests.cpp)
target_link_libraries(libbsa_bsa_reader_tests PRIVATE libbsa::libbsa Catch2::Catch2WithMain)
catch_discover_tests(libbsa_bsa_reader_tests TEST_PREFIX "libbsa_bsa_reader_tests." ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;fixture")
```

## Landmines

- Keep `TES5Edit/` read-only; reference comments in tests are allowed, runtime fixture reads from the submodule are not.
- Do not add CMake globs.
- Do not expose native codec or DirectXTex headers/tokens from public headers.
- Compression state must use archive flag XOR file-size flag behavior.
