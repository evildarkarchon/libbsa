# Thread Safety {#thread_safety}

libbsa objects are isolated by ownership. The library does not use global mutable state for archive reading, writing, validation, or benchmark report generation.

## archive_reader

Independently opened `archive_reader` objects may be used concurrently by different threads. A single `archive_reader` object may run concurrent const metadata, lookup, listing, single-entry extraction, and bulk extraction calls when each extraction writes to a distinct caller-owned sink. This distinct sink requirement keeps libbsa responsible for archive parsing and scheduling while callers keep ownership of shared output state and any caller-owned synchronization.

## payload_sink

`payload_sink` instances are caller-owned. A sink may be written by libbsa on the thread performing extraction; when parallel bulk extraction is used, every requested entry must receive a distinct sink unless the caller has made a sink explicitly safe for the way it is shared. Partial writes remain extraction failures rather than partial successes.

## bulk_extract_sink_factory

`bulk_extract_sink_factory::create` may be called concurrently when `bulk_extract_options::worker_count` is greater than one. A successful factory call must return a distinct sink for that requested entry, and the factory must protect any caller-owned shared state it touches. libbsa does not call sink factory or sink methods while holding internal locks.

## bulk_extract_options

`bulk_extract_options` is a small value object. It may be copied freely between threads, but callers should not mutate an options object at the same time another thread passes it into `archive_reader::extract_entries`.

## bulk_extract_entry_result

`bulk_extract_entry_result` records are returned after extraction completes and are independent values. Different result records may be inspected concurrently after the result vector is no longer being mutated by the caller.

## tes3_bsa_writer

Separately constructed or moved-to `tes3_bsa_writer` objects may be used concurrently. Calls that mutate a writer, including `add_file` and `add_bytes`, are not concurrent with other mutation or `write_to` on the same writer object. `write_to` owns any worker scheduling for that finalization call.

## tes4_bsa_writer

Separately constructed or moved-to `tes4_bsa_writer` objects may be used concurrently. Calls that mutate a writer, including `add_file` and `add_bytes`, are not concurrent with other mutation or `write_to` on the same writer object. `write_to` owns any worker scheduling for that finalization call.

## ba2_gnrl_writer

Separately constructed or moved-to `ba2_gnrl_writer` objects may be used concurrently. Calls that mutate a writer, including `add_file` and `add_bytes`, are not concurrent with other mutation or `write_to` on the same writer object. `write_to` owns any worker scheduling for that finalization call.

## ba2_dx10_writer

Separately constructed or moved-to `ba2_dx10_writer` objects may be used concurrently. Calls that mutate a writer, including `add_file`, are not concurrent with other mutation or `write_to` on the same writer object. `write_to` owns any worker scheduling for that finalization call.

## write_execution_options

`write_execution_options` is copied into a write call. These write-call execution controls keep finalization scheduling separate from target compatibility options: `worker_count == 1` preserves serial behavior, `worker_count > 1` opts into writer-owned scheduling, and callers must not use `worker_count == 0`.

## validation_report

`validation_report` is returned as an independent value. It may be inspected concurrently after the caller stops mutating the report, its diagnostics, and its warning vectors.

## validate_archive

`validate_archive` has no global mutable state. Independent validation calls may run concurrently, including calls with entry extractability enabled, subject to normal host filesystem behavior for the archive paths being read.

## libbsa_benchmarks

`libbsa_benchmarks` is maintainer tooling for report generation. Benchmark reports are not a synchronization primitive and must not be used to coordinate application threads or prove application-level thread safety.
