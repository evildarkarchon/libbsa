## 1. Tests and Policy Coverage

- [x] 1.1 Add focused coverage that rejects the old counter-only BA2 DX10 snapshot directory reservation path in `src/formats/ba2/ba2_dx10_prepare.cpp`.
- [x] 1.2 Add or update BA2 DX10 writer coverage to confirm snapshot cleanup ownership remains best-effort writer-state teardown behavior.
- [x] 1.3 Keep existing BA2 DX10 writer fixture and round-trip expectations unchanged so archive bytes, compression metadata, and validation behavior remain stable.

## 2. Snapshot Directory Hardening

- [x] 2.1 Replace the static monotonic counter in `make_unique_snapshot_directory` with a helper that obtains at least 128 bits of fresh Windows cryptographic randomness for each candidate suffix.
- [x] 2.2 Encode the random suffix into the `libbsa-dx10-snapshot-<suffix>` directory name without adding public API surface or new third-party dependencies.
- [x] 2.3 Preserve atomic reservation by only accepting candidates after successful directory creation, retrying collisions with fresh random bytes, and returning `io_error` on RNG or repeated reservation failure.
- [x] 2.4 Keep `ba2_dx10_writer::state` as the owner of snapshot directory cleanup and preserve move-owned staged snapshot behavior.

## 3. Build Integration

- [x] 3.1 If the selected Windows RNG API requires an import library, add the private system link dependency to `libbsa` in `CMakeLists.txt`.
- [x] 3.2 If shared-build internal test support compiles libbsa sources directly, mirror any required private system link dependency in `tests/CMakeLists.txt`.

## 4. Verification

- [x] 4.1 Run `cmake --build --preset windows-msvc-debug-static` and resolve any compile or link failures.
- [x] 4.2 Run `ctest --preset windows-msvc-debug-static` and confirm the hardened snapshot and existing BA2 DX10 writer tests pass.
- [x] 4.3 Run `openspec status --change "harden-dx10-snapshot-temp-dirs"` and confirm the change remains apply-ready.
