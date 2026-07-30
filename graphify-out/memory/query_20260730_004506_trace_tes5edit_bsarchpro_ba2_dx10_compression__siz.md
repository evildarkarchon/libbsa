---
type: "query"
date: "2026-07-30T00:45:06.270244+00:00"
question: "Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33"
contributor: "graphify"
outcome: "dead_end"
source_nodes: ["ba2_dx10_serialize.cpp", "packed_size", "writer.hpp"]
---

# Q: Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33

## Answer

Expanded from original query via graph vocab: [archive, chunk, compression, deflate, dxgi, packed, payload, raw, records, serialization, texture, writer]. The graph exposed libbsa DX10 terminology but not the TES5Edit Pascal oracle, so direct read-only source tracing was required.

## Outcome

- Signal: dead_end

## Source Nodes

- ba2_dx10_serialize.cpp
- packed_size
- writer.hpp