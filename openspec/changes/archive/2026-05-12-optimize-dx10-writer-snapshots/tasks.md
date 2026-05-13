## 1. Focused Tests

- [x] 1.1 Add or update BA2 DX10 writer-stage coverage for a planned chunk spanning multiple snapshot-backed mips, asserting successful preparation and unchanged raw-size validation.
- [x] 1.2 Add BA2 DX10 writer-stage coverage for a corrupted, truncated, or mismatched snapshot file, asserting `write_to`/preparation fails without publishing output and preserves result-based error semantics.
- [x] 1.3 Add or preserve BA2 DX10 writer coverage proving that mutating or deleting the original DDS host file after `add_file` does not change the written archive payload.

## 2. Snapshot Chunk Assembly

- [x] 2.1 Introduce a private BA2 DX10 helper in `src/formats/ba2/ba2_dx10_prepare.cpp` that collects the snapshots required by one `planned_texture_chunk` in BA2-required mip order and reports their aggregate byte count.
- [x] 2.2 Route raw chunk byte assembly through the helper, pre-sizing or pre-reserving the destination vector from `planned.raw_size` with `detail::byte_vector` allocation helpers before copying snapshot bytes.
- [x] 2.3 Validate the aggregate snapshot byte count and final raw chunk byte count against `planned.raw_size` before compression, returning a failed `result` on mismatch.
- [x] 2.4 Keep snapshot reads on the existing bounded disk-source helper path with `ba2_dx10_snapshot_source_context`, preserving BA2 DX10 I/O diagnostics and snapshot cleanup ownership.

## 3. Benchmark Visibility

- [x] 3.1 Update `benchmarks/libbsa_benchmarks.cpp` so the BA2 DX10 scenario records add-time DDS snapshot staging separately from final `write_to` preparation/serialization.
- [x] 3.2 Ensure benchmark reports keep enough scenario and worker-count context to compare add-time staging and finalization costs across runs.
- [x] 3.3 Keep BA2 DX10 benchmark correctness checks for reopened metadata and extracted DDS payload shape after splitting the timing measurements.

## 4. Validation

- [x] 4.1 Run the focused BA2 DX10 writer and writer-stage tests with `ctest --preset windows-msvc-debug-static` after building the debug-static preset.
- [x] 4.2 Run the benchmark target or executable to confirm the new BA2 DX10 timing rows are emitted and correctness checks pass.
- [x] 4.3 Run `openspec status --change optimize-dx10-writer-snapshots` and confirm the change remains apply-ready.
