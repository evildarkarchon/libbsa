## 1. Public Writer API Ownership

- [x] 1.1 Update `include/libbsa/writer.hpp` so `tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, and `ba2_dx10_writer` explicitly delete copy construction and copy assignment.
- [x] 1.2 Add explicit public destructor, move constructor, and move assignment declarations for each writer class with Doxygen comments documenting move-only ownership transfer.
- [x] 1.3 Change each writer class private `state_` owner from shared ownership to exclusive ownership so the header no longer models mutable writer state as shareable.

## 2. Writer Implementation Updates

- [x] 2.1 Update `src/formats/bsa/tes3_bsa_writer.cpp` and `src/formats/bsa/tes4_bsa_writer.cpp` to construct exclusive state ownership and define the destructor, move constructor, and move assignment out-of-line.
- [x] 2.2 Update `src/formats/ba2/ba2_gnrl_writer.cpp` and `src/formats/ba2/ba2_dx10_writer.cpp` to construct exclusive state ownership and define the destructor, move constructor, and move assignment out-of-line.
- [x] 2.3 Verify BA2 DX10 snapshot directory ownership still transfers on move and cleanup remains best-effort exactly once through the moved-to writer state.
- [x] 2.4 Keep existing add-entry methods, option accessors, and `write_to` methods behaviorally unchanged apart from following the exclusive state owner.

## 3. Tests And Documentation

- [x] 3.1 Add focused writer ownership unit coverage, either in a new `tests/unit/writer_ownership_tests.cpp` added to `tests/CMakeLists.txt` or in an existing writer test file.
- [x] 3.2 Add compile-time assertions that all four writer classes are not copy constructible, not copy assignable, move constructible, and move assignable.
- [x] 3.3 Add runtime move-construction and move-assignment coverage proving a moved-to writer can finalize staged entries without changing existing archive-output expectations.
- [x] 3.4 Update `docs/thread-safety.md` if needed so the independent-writer wording is consistent with move-only public writer ownership.

## 4. Verification

- [x] 4.1 Configure and build the Windows MSVC debug static preset: `cmake --preset windows-msvc-debug-static` and `cmake --build --preset windows-msvc-debug-static`.
- [x] 4.2 Run the test preset: `ctest --preset windows-msvc-debug-static`.
- [x] 4.3 If shared-library export behavior is affected by the new special members, also build and test `windows-msvc-debug-shared`.
