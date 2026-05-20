# M001: Audit and Stabilization Baseline

**Vision:** Make libbsa support claims truthful and evidence backed by auditing the implemented archive families, public API story, validation and error behavior, generated fixture proof, package consumer proof, and public documentation, then close a bounded set of the highest claim risk gaps without requiring copyrighted local archives or BSArchPro output.

## Success Criteria

- A public docs coverage and gap matrix covers every current archive family and agreed capability axis with final Proven, Fixed, Partial, Missing, or Deferred status, evidence references, and rationale.
- A ranked claim risk gap list shows what M001 fixed, what remains deferred, why, and likely future ownership.
- The public API story for archive_reader, validate_archive, bulk extraction, family specific writers, result errors, validation diagnostics, dependency light headers, and package consumer usage is audited and backed by evidence.
- A bounded tranche of highest risk gaps is closed with durable repository proof such as Catch2 tests, generated fixtures, manifests, validation tests, policy checks, docs, or package consumer examples.
- Default completion uses only generated legal fixtures and repository checks; optional local game archive or BSArchPro comparison paths remain opt in and non blocking.
- The installed and exported public API remains usable through <libbsa/libbsa.hpp> and the libbsa::libbsa target.
- The TES5Edit submodule remains read only and is not edited, formatted, staged, compiled into libbsa, or used as mutable fixture data.

## Slices

- [ ] **S01: Evidence Matrix and Gap Ranking** `risk:Highest risk: current docs, tests, fixtures, and support claims may not align, so downstream remediation could target the wrong gaps unless the truth source is established first.` `depends:[]`
  > After this: A public docs coverage matrix and ranked gap list exist for TES3 BSA, TES4 family BSA, BA2 GNRL, and BA2 DX10, with evidence links and fixed partial missing deferred status seeded from repository proof.

- [ ] **S02: Public API and Error Contract Audit** `[sketch]` `risk:High risk: public headers and package consumer examples may compile while still leaving consumers unclear about the stable core API, validation semantics, dependency leakage, or branchable error behavior.` `depends:[S01]`
  > After this: The coverage matrix and docs identify the stable public API story, package consumer evidence, public header dependency boundaries, validation semantics, and any API or error contract gaps that need fixes.

- [ ] **S03: Risk Bounded Gap Remediation** `[sketch]` `risk:High risk: the milestone is not complete if it only documents gaps, but the exact safe remediation set must be selected from S01 and S02 evidence rather than guessed up front.` `depends:[S01,S02]`
  > After this: A coherent tranche of the highest claim risk gaps is fixed with durable tests, generated fixture evidence, validation or package consumer proof, and the matrix marks each fixed or intentionally deferred gap.

- [ ] **S04: Integrated Verification and Truth Closeout** `[sketch]` `risk:Medium risk: individual fixes can pass locally while the default package consumable whole or public truth matrix drifts from final behavior.` `depends:[S03]`
  > After this: The default Windows CMake and CTest workflow plus package consumer proof passes, the public matrix records final statuses and evidence, and remaining gaps have explicit deferred rationale and owners.

## Boundary Map

## Boundary Map

| Boundary | Planning stance | Owning slice |
| --- | --- | --- |
| Requirements | Active R001 through R009 must be mapped, fixed, deferred, or explicitly out of scope. | S01 and S04 |
| Decisions | Preserve the audit first, risk bounded remediation, public docs matrix, layered proof, and no broad API redesign decisions. | All slices |
| Shutdown or lifecycle | No service lifecycle is involved; no shutdown behavior is in scope. | Out of scope |
| Revenue or billing | No revenue, billing, or monetization boundary is involved. | Out of scope |
| Auth or secrets | No auth providers, credentials, SaaS APIs, or secret material are required. | Out of scope |
| Shared resources | Local filesystem, generated fixtures, CMake build trees, vcpkg packages, and the read only TES5Edit submodule are the shared resources. | S01, S03, S04 |
| Reconnection or offline behavior | No network reconnection behavior is involved; default verification should work without external services after dependencies are available. | Out of scope |

## Skills Discovered

Installed prompt skills directly relevant to this milestone include api-design, design-an-interface, tdd, verify-before-complete, write-docs, cmake, cpp-testing, review, and systematic-debugging. No additional external skills are required for the core technologies because the milestone uses repository local C++20, CMake, vcpkg, Catch2, generated fixtures, and documentation proof rather than a new SaaS or framework integration.
