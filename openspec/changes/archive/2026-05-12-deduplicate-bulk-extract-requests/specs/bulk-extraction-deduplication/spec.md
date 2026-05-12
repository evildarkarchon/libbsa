## ADDED Requirements

### Requirement: Bulk extraction coalesces duplicate request paths
`archive_reader::extract_entries` SHALL coalesce repeated `bulk_extract_request::path` values that are exactly equal within a single call. For each distinct request path string, libbsa MUST perform archive lookup, sink creation, and payload extraction at most once.

#### Scenario: Parallel duplicate requests share one extraction
- **WHEN** a caller submits multiple `bulk_extract_request` records with the same `path` and `bulk_extract_options::worker_count` is greater than one
- **THEN** libbsa creates at most one sink for that path
- **THEN** libbsa extracts that path at most once
- **THEN** every duplicate request still receives a result record in its original request position

#### Scenario: Serial duplicate requests share one extraction
- **WHEN** a caller submits duplicate request paths with the default serial bulk extraction options
- **THEN** libbsa applies the same duplicate coalescing behavior as parallel extraction
- **THEN** result ordering and success reporting remain identical to the caller's request order

### Requirement: Duplicate results mirror the first occurrence outcome
For an exact duplicate request path, `archive_reader::extract_entries` SHALL use the first occurrence as the owning extraction for that path. Later duplicate result records MUST preserve their original `path` values and mirror the owning occurrence's `entry` and `failure` outcome.

#### Scenario: Successful duplicate extraction produces per-request success records
- **WHEN** the first occurrence of a duplicated path is found, its sink is created successfully, and extraction succeeds
- **THEN** the first occurrence result contains the extracted entry metadata and no failure
- **THEN** each later duplicate result contains the same entry metadata and no failure
- **THEN** each result record's `path` equals the corresponding input request path

#### Scenario: Missing duplicate path produces per-request not-found failures
- **WHEN** a duplicated request path is not present in the archive
- **THEN** each result record for that duplicated path contains no entry metadata
- **THEN** each result record for that duplicated path contains a `libbsa::error_code::not_found` failure
- **THEN** unrelated distinct request paths can still complete independently

#### Scenario: Sink factory failure is mirrored for duplicate requests
- **WHEN** the first occurrence of a duplicated path is found but sink creation returns an error
- **THEN** the first occurrence result records that sink creation failure
- **THEN** each later duplicate result for that path records the same failure
- **THEN** libbsa does not call the sink factory again for that duplicate path within the same bulk extraction call

### Requirement: Distinct request paths remain independently processed
`archive_reader::extract_entries` SHALL preserve independent processing for request paths that are not exactly equal. Duplicate coalescing MUST NOT merge distinct request strings solely because archive-specific lookup rules might resolve them to the same canonical entry.

#### Scenario: Distinct request strings are not pre-merged
- **WHEN** a caller submits two request paths whose strings are not exactly equal
- **THEN** libbsa treats them as distinct bulk extraction work items before archive lookup
- **THEN** each distinct string receives its own lookup and result outcome according to the existing archive path rules
