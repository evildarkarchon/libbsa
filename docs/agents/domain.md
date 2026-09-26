# Domain Docs

This repository uses a single-context domain-document layout.

## Before exploring, read these

- `CONTEXT.md` at the repository root.
- Relevant architectural decision records under `docs/adr/`.

If either location does not exist, proceed silently. Do not flag its absence or suggest creating it upfront. The domain-modeling workflows create domain documents lazily when terminology or decisions are resolved.

## File structure

```text
/
├── CONTEXT.md
├── docs/adr/
└── src/
```

## Use the glossary's vocabulary

When output names a domain concept—in an issue title, refactor proposal, hypothesis, or test name—use the term defined in `CONTEXT.md`. Do not drift to synonyms that the glossary explicitly avoids.

If a needed concept is absent, reconsider whether the term belongs to the project. If it represents a real domain gap, record it for the domain-modeling workflow.

## Flag ADR conflicts

If proposed work contradicts an existing ADR, surface the conflict explicitly rather than silently overriding the decision.
