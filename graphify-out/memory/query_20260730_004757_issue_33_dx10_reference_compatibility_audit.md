---
type: "query"
date: "2026-07-30T00:47:57.796721+00:00"
question: "Issue 33 DX10 reference compatibility audit"
contributor: "graphify"
outcome: "useful"
source_nodes: ["stored_payload.cpp", "dedupe_key", "ba2_dx10_serialize.cpp"]
---

# Q: Issue 33 DX10 reference compatibility audit

## Answer

Expanded from original query via graph vocab: [stored, payload, chunk, compression, fingerprint, dedupe, representative, packed, size, serialization, placement, prepared]. Direct source audit: preserve FO4 deflate and SF3 deflate/raw-LZ4 routing; raw and packed size semantics; canonical first representative with exact Stored Payload equality; no fingerprint when dedupe is disabled; and current libbsa metadata-name-payload serialization order. TES5Edit hash-only and physical-writer-first behavior is an oracle limitation, not a contract to copy.

## Outcome

- Signal: useful

## Source Nodes

- stored_payload.cpp
- dedupe_key
- ba2_dx10_serialize.cpp