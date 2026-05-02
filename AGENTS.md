# AGENTS.md

## Project Purpose

This repository is for building a reusable C++ library for reading and writing Bethesda Archive formats.

The behavioral reference is the BSArchPro code from the `TES5Edit` git submodule. Treat that code as prior art and compatibility guidance while designing a clean C++ library surface for this project.

Useful reference areas currently include:

- `TES5Edit/BSArchPro.dpr`
- `TES5Edit/BSArch/`
- `TES5Edit/Core/wbBSArchive.pas`
- `TES5Edit/Core/wbBSA.pas`

## Hard Boundary: TES5Edit Is Read-Only

`TES5Edit/` is a read-only reference submodule. Do not modify it for any reason.

This includes:

- Do not edit files under `TES5Edit/`.
- Do not format files under `TES5Edit/`.
- Do not apply generated changes under `TES5Edit/`.
- Do not update the submodule pointer.
- Do not stage or commit changes inside `TES5Edit/`.
- Do not treat the submodule as vendored source to be compiled into this project.

All implementation work belongs outside `TES5Edit/`.

## Language and Implementation Direction

- The implementation language is C++.
- Keep the library reusable and independent of application-specific UI or tooling.
- Prefer clear, portable C++ interfaces over direct transliteration of Delphi/Pascal structure.
- Preserve archive-format behavior discovered from BSArchPro unless there is a documented reason to diverge.
- When porting behavior, trace the reference code first and record non-obvious compatibility constraints near the new implementation.

## Dependencies

Deflate and LZ4 compression/decompression support are required.

- Do not introduce external dependencies speculatively.
- Prefer the C++ standard library until a real format, compression, filesystem, testing, or packaging requirement justifies more.
- Use `libdeflate` for deflate compression and decompression.
- Use the official `lz4` library for LZ4 compression and decompression.
- Use `vcpkg` for dependency management.
- If a dependency becomes useful, document the need, the alternatives considered, and the expected project impact before adding it.

## Comments and Documentation

- Never delete an accurate comment as cleanup. Remove or rewrite a comment only when the code it describes is deleted or has changed enough to make the comment wrong.
- If a comment is removed or rewritten, mention it in the final reply.
- Add comments for non-obvious why: format compatibility constraints, ownership/lifetime decisions, error-handling edge cases, threading behavior, cancellation behavior, and deliberate deviations from the reference implementation.
- Add Doxygen-compliant C++ doc comments (/// or /** ... */) for public APIs and for methods that are added or substantially rewritten.
- Trivial private helpers may omit doc comments when their purpose is obvious.

## Validation Expectations

- Add focused tests for archive parsing, writing, round-tripping, and compatibility behavior as those surfaces are implemented.
- Prefer fixture-based tests that prove byte-level or metadata-level compatibility with known archive behavior.
- Do not use the `TES5Edit/` submodule as a mutable test fixture.

## MCP Server Usage

### Exa (`mcp__exa`)

- `web_search_exa` - Use for general web lookups, articles, blog posts, and non-documentation content.
- `get_code_context_exa` - Prefer for code-related web search, tutorials, examples, and SDK/API context.
- `deep_researcher_start` - Use for complex multi-source research, then poll with `deep_researcher_check`.

### Ref (`mcp__ref`)

- `ref_search_documentation` - Search documentation across the web, GitHub, and private resources. Include the programming language and library/framework name in the query.
- `ref_read_url` - Read a URL returned by `ref_search_documentation`. Pass the exact URL, including any `#hash`.

### Context7 (`mcp__context7`)

- `resolve-library-id` - Call before `query-docs` unless an explicit `/org/project` ID is provided.
- `query-docs` - Retrieve current documentation and examples for the resolved library ID.
- Do not call Context7 tools more than three times per question.

### Tool Choice

- Official Microsoft/Azure docs: use Microsoft Learn MCP tools first.
- Library or framework docs: use Context7 or Ref.
- Code-related web search: use Exa `get_code_context_exa`.
- General web search: use Exa `web_search_exa`.
- Deep research: use Exa `deep_researcher_start`.
