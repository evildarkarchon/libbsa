---
type: "query"
date: "2026-07-30T00:37:43.910817+00:00"
question: "How do BA2 DX10 prepared chunks compression finalization deduplication serialization work and where are Stored Payload integration points?"
contributor: "graphify"
outcome: "useful"
source_nodes: ["dedupe_key", "exactly_equals", "ba2_dx10_serialize.cpp", "stored_payload"]
---

# Q: How do BA2 DX10 prepared chunks compression finalization deduplication serialization work and where are Stored Payload integration points?

## Answer

Expanded from original query via graph vocabulary: [chunk, compression, dedupe, exact, finalization, fingerprint, payload, placement, prepared, serialization, stored, writer]. The graph identifies src/formats/ba2/ba2_dx10_layout.cpp dedupe_key and placement assignment, src/formats/ba2/ba2_dx10_serialize.cpp emission, and src/detail/stored_payload.hpp/.cpp exact equality, fingerprint, and emit as the integration points. Direct source and tests remain authoritative because traversal was truncated.

## Outcome

- Signal: useful

## Source Nodes

- dedupe_key
- exactly_equals
- ba2_dx10_serialize.cpp
- stored_payload