## Context

The public writer classes (`tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, and `ba2_dx10_writer`) store mutable writer state through `std::shared_ptr<state>` and do not declare copy or move special member functions. The compiler therefore permits copying, and a copied writer aliases the same mutable entry collection, options, and in the DX10 case snapshot directory ownership.

The documented ownership model says independent writer objects may be used concurrently, while mutation is not safe only on the same writer object. Aliasing copies make two distinct C++ objects behave as one shared mutable writer and can leak entries from a copy into the original `write_to` output.

## Goals / Non-Goals

**Goals:**
- Make all public archive writer classes impossible to copy.
- Preserve move construction and move assignment for ownership transfer without mutable-state aliasing.
- Encode exclusive state ownership in the private implementation where practical.
- Add compile-time regression coverage for copy and move traits.
- Preserve existing add-entry validation, `write_to` behavior, emitted archive bytes, and writer option semantics.

**Non-Goals:**
- Do not implement deep-copy writer semantics in this change.
- Do not add locks or make mutation concurrently safe on one writer object.
- Do not change archive layout, compression, publish, or snapshot cleanup behavior.
- Do not touch the `TES5Edit/` reference submodule.

## Decisions

1. Public writer types become move-only.

   Delete copy construction and copy assignment for each writer class. Declare move construction and move assignment explicitly so callers can still return writers from factories, pass ownership between scopes, and store writers in move-aware containers. This is a source-breaking API change for callers that copied writers, but it is the smallest contract that prevents accidental shared mutable state.

   Alternative considered: implement deep-copy semantics. This would preserve source compatibility but creates ambiguous and potentially expensive behavior for disk-backed entries and BA2 DX10 snapshot directories. It also risks presenting copies as cheap when they may duplicate staged payload metadata or files.

2. Prefer exclusive PIMPL ownership for writer state.

   Replace the private `std::shared_ptr<state>` with an exclusive owner such as `std::unique_ptr<state>` and define the required destructor and move operations out-of-line in each writer translation unit. This makes the implementation match the public ownership contract and prevents future internal code from accidentally reintroducing aliasing.

   Alternative considered: keep `std::shared_ptr<state>` and only delete copy operations. That blocks public copies, but the private type would still suggest shared ownership and leaves the aliasing footgun available to future internal helpers.

3. Validate through type traits and existing writer behavior tests.

   Add focused static assertions in writer unit coverage proving all four writer classes are non-copy-constructible, non-copy-assignable, move-constructible, and move-assignable. Existing writer tests continue to validate that archive output and `write_to` semantics did not change.

   Alternative considered: add runtime tests that try copy mutation. Deleted copy operations are a compile-time contract, so type traits provide a clearer regression guard without intentionally adding code that should not compile.

## Risks / Trade-offs

- Source compatibility break for consumers that copied writers -> Mitigation: document the change in proposal/tasks and keep moves supported for ownership transfer.
- Moved-from writer misuse could dereference an empty private state -> Mitigation: rely on normal C++ moved-from object rules and keep tests focused on moving into valid destination writers rather than calling operational methods on moved-from writers.
- Unique PIMPL ownership requires explicit destructor and move definitions because `state` is incomplete in the public header -> Mitigation: define those special members in the existing writer translation units next to the `state` definitions.
- BA2 DX10 snapshot cleanup ownership changes from shared cleanup to exclusive cleanup -> Mitigation: moving transfers the single state object, so cleanup still happens exactly once when the owning moved-to writer is destroyed.
