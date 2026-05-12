## 1. Regression Tests

- [x] 1.1 Add bulk extraction test support that can count sink factory `create` calls per request path while still capturing extracted bytes for the owning request.
- [x] 1.2 Add a duplicate-success test for serial extraction that asserts request-order success records, mirrored entry metadata, and one sink creation for the duplicate path.
- [x] 1.3 Add a duplicate-success test for parallel extraction that asserts the same duplicate coalescing behavior with `worker_count > 1`.
- [x] 1.4 Add failure tests proving duplicate missing paths and duplicate sink-factory failures are mirrored to every duplicate result without aborting unrelated distinct paths.

## 2. Bulk Extraction Implementation

- [x] 2.1 Refactor `archive_reader::extract_entries` to build exact-string request groups before dispatching worker tasks.
- [x] 2.2 Run `detail::run_indexed_work` over unique request groups instead of every original request index.
- [x] 2.3 For each unique group, perform lookup, sink creation, and extraction only for the first occurrence, then copy the outcome to every grouped result index while preserving each original result `path`.
- [x] 2.4 Preserve existing outer validation for unopened readers and invalid `worker_count`, and preserve per-entry failure reporting for lookup, sink creation, null sink, and extraction failures.

## 3. Documentation And Verification

- [x] 3.1 Update public comments or thread-safety documentation that describe bulk sink creation so duplicate request handling is explicit.
- [x] 3.2 Run the focused bulk extraction unit tests and fix any regressions.
- [x] 3.3 Run the broader available unit test target or CTest preset for the configured build.
- [x] 3.4 Validate the OpenSpec change artifacts before implementation is considered complete.
