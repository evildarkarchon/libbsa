---
type: "query"
date: "2026-07-29T11:52:32.486111+00:00"
question: "MSVC C2039/C3861: ba2_gnrl_plan_placements is not a member of libbsa::formats::ba2 in writer_stage_tests.cpp"
contributor: "graphify"
outcome: "useful"
source_nodes: ["writer_stage_tests.cpp", "ba2_gnrl_plan_placements()", "ba2_gnrl_layout.hpp"]
---

# Q: MSVC C2039/C3861: ba2_gnrl_plan_placements is not a member of libbsa::formats::ba2 in writer_stage_tests.cpp

## Answer

Expanded from original query via graph vocabulary: [gnrl, placement, plan, serialize, writer, stage, layout, prepare]. Root cause: branch HEAD tests call ba2_gnrl_plan_placements while checked-in ba2_gnrl_layout still exposes only ba2_gnrl_assign_payload_offsets. The current uncommitted worktree contains the coherent placement-plan migration across layout, preparation, serialization, writer orchestration, and tests. Verified with an isolated Windows MSVC ASan build; focused BA2 GNRL tests passed; full suite passed 502 cases with 2 local-fixture skips and 27753 assertions.

## Outcome

- Signal: useful

## Source Nodes

- writer_stage_tests.cpp
- ba2_gnrl_plan_placements()
- ba2_gnrl_layout.hpp