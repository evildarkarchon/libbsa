# M001-k9wo8b Discussion Log

## Exchange — 2026-05-19T23:21:57.468Z

### API Shape

How should M001 treat the public API architecture?

- **Explicit plus helpers (Recommended)** — Keep archive_reader and family-specific writers as the stable core, then add only audit-proven ergonomic helpers.
- **New façade layer** — Design a higher-level API that hides archive families and routes internally, accepting more up-front design risk.
- **Document current API** — Do not change the API architecture in M001; make current capabilities clear and tested first.

**Selected:** Explicit plus helpers (Recommended)

### Audit Form

What should the audit produce as the durable source of truth?

- **Matrix plus tests (Recommended)** — Create a coverage matrix and convert key gaps into tests or policy checks as they are fixed.
- **Tests only** — Let the test suite be the contract and avoid maintaining a separate written gap matrix.
- **Docs first** — Produce the inventory and roadmap of gaps before implementing many tests or fixes.

**Selected:** Matrix plus tests (Recommended)

### Proof Bias

Which proof source should dominate stabilization decisions when evidence conflicts?

- **Layered proof (Recommended)** — Use generated fixtures for CI, opt-in game/BSArchPro comparisons for compatibility, and package-consumer tests for API usability.
- **CI fixtures first** — Treat committed/generated fixtures as the main authority so all proof is reproducible without local data.
- **Reference compatibility** — Prioritize BSArchPro/game-archive evidence, even if some checks remain opt-in locally.

**Selected:** Layered proof (Recommended)

---
## Exchange — 2026-05-19T23:25:02.976Z

### Errors

We can go deep on error handling and failure modes, or I can apply sensible defaults based on the architecture decisions above. Which do you prefer?

- **Use defaults (Recommended)** — Use structured defaults from the existing result/error model, then audit for gaps and inconsistent diagnostics.
- **Go deep** — Spend discussion time specifying detailed failure behavior for corrupt archives, I/O races, partial extraction, and compatibility warnings.

**Selected:** Use defaults (Recommended)

---
## Exchange — 2026-05-19T23:28:16.927Z

### Done Bar

What should be the M001 quality bar?

- **Full layered bar (Recommended)** — Require audit matrix, targeted fixes, default build/test pass, package-consumer proof, and documented optional compatibility path.
- **CI-only bar** — Require only default CI-verifiable artifacts and avoid optional compatibility requirements in M001 acceptance.
- **Compatibility-heavy** — Require local game/BSArchPro evidence where available before considering support claims stabilized.

**Selected:** Full layered bar (Recommended)

### Fix Budget

How much remediation should M001 commit to after the audit?

- **Risk-bounded fixes (Recommended)** — Fix the highest-risk gaps that fit naturally after the audit, but do not promise every gap is closed in M001.
- **Close all gaps** — Make M001 pass only if all audit-discovered gaps across current formats are closed.
- **Audit mostly** — Make M001 primarily inventory and planning, with minimal fixes unless trivial.

**Selected:** Risk-bounded fixes (Recommended)

---
## Exchange — 2026-05-19T23:40:12.772Z

### Depth Check

Did I capture the depth right?

- **Yes, you got it (Recommended)** — Proceed to focused research, requirements, and roadmap using this understanding.
- **Not quite — let me clarify** — Pause here so you can correct scope, priorities, or wording before artifacts are written.

**Selected:** Yes, you got it (Recommended)

---
## Exchange — 2026-05-19T23:45:03.341Z

### Roadmap

Ready to write, or want to adjust?

- **Ready to write (Recommended)** — Write PROJECT, CONTEXT, ROADMAP, decisions, and finalize M001-k9wo8b planning artifacts.
- **Adjust first** — Pause so you can change slices, dependencies, wording, or definition of done before artifacts are written.

**Selected:** Ready to write (Recommended)

---
