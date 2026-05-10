# libbsa Benchmarks

The benchmark harness is explicit maintainer tooling for PERF-05. It generates
legal synthetic data in a temporary directory, runs serial and opt-in parallel
archive workflows, verifies correctness by reopening and extracting through
public libbsa APIs, and writes report files for review.

Build the normal Windows debug preset, then generate a report with:

```powershell
cmake --build --preset windows-msvc-debug-static --target libbsa_benchmark_report
```

The target writes:

- `build/windows-msvc-debug-static/benchmarks/libbsa-benchmark.json`
- `build/windows-msvc-debug-static/benchmarks/libbsa-benchmark.md`

Each report row contains:

- `scenario` - the benchmark scenario name.
- `worker_count` - `1` for serial or `4` for opt-in parallel execution.
- `elapsed_ms` - wall-clock elapsed milliseconds for the measured workflow.
- `bytes_processed` - synthetic payload bytes packed and extracted by the scenario.
- `correctness_passed` - whether the workflow reopened and extracted as expected.

The current scenarios are `tes4_bsa_pack_extract`, `ba2_gnrl_pack_extract`,
`ba2_dx10_pack_extract`, and `bulk_extract`. They use legal synthetic payloads
created by the benchmark runner. The benchmark must not use game archives,
BSArchPro exports, or `TES5Edit/` as a fixture workspace.

D-20 policy: default CTest does not gate on a fixed speedup threshold. Timing
values are host-dependent and report-only; correctness and report generation are
the automated contract.
