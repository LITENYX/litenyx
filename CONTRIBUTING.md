# Contributing to Litenyx

## Repository structure

This workspace contains five repositories with distinct responsibilities:

| Repository | Role | Authority |
|---|---|---|
| litenyx | Implementation | Implementation authority |
| litenyx-spec | Normative specification | Specification authority |
| litenyx-plan | Planning and execution sequencing | Research only |
| litenyx-walkthrough | Reproducible execution history | Non-authoritative |
| Litenyx-oracle | Provenance and evidence reconciliation | Non-authoritative index |

## Development workflow

1. **Spec-first**: Consensus behavior must be specified in `litenyx-spec` before implementation
2. **Plan then build**: Complex changes should first be gated through `litenyx-plan` (OPEN → CANDIDATE → RATIFIED → SPEC)
3. **Implement**: Build against the frozen specification in `litenyx`
4. **Evidence**: Record test results and execution evidence
5. **Walkthrough**: Document the change in `litenyx-walkthrough` for reproducibility
6. **Index**: The Oracle captures the provenance snapshot

## Branching strategy

- `main` — stable, reviewed changes only
- `task/<TASK-ID>` — active development branches
- `review/<TASK-ID>` — review branches
- `ratification/<TASK-ID>` — ratification branches (litenyx-plan only)

## Commit conventions

- Prefix with the domain: `d0:`, `spec:`, `plan:`, `walk:`, `oracle:`, `gate:`
- Reference gates and task IDs where applicable
- Evidence commits should reference the reproducing command

## Evidence requirements

- All behavioral claims must be backed by reproducible tests
- Test execution commands must be documented
- Evidence artifacts (EXECUTION-REPORT.json, TRACEABILITY-INDEX.csv) must be committed
- Confidence classes (OBSERVED, DECLARED, DERIVED, INFERRED, UNRESOLVED) must be respected

## DBET discipline

- Construct(D0) ⇏ Modify(V_D0(1)) — implementation must not mutate frozen vectors
- Frozen specification changes require explicit gate authorization
- Stub elimination must preserve predicate disposition
- R0 reconciliation must not alter V_D0(1)
- Test coverage must account for all frozen vectors

## Review expectations

- All PRs require review before merge
- Review must verify: evidence completeness, methodology soundness, authority compliance
- Consensus behavior changes require specification agent review
- Evidence-only changes require verification of reproducibility

## Cross-repository coordination

- Changes spanning multiple repos should use a tracking issue in `litenyx-plan`
- Gate authorization is recorded in `litenyx-plan` with evidence in the implementing repo
- The Oracle indexes all cross-repo references

## Repository-specific guidelines

### litenyx (Implementation)

- Build via `deploy/Makefile` (`make m3-integration` for full gate)
- Test via `uv run --python 3.12 --with pytest python -m pytest d0-test-harness`
- All C++ code must compile with Dogecoin Core 1.14.9 pinned dependency
- Test evidence must reference the reproducing command and environment

### litenyx-spec (Specification)

- Specifications must be compatible with deterministic reconstruction
- Consensus-changing behavior requires explicit Connect/Disconnect/replay semantics
- Stable identifiers: `SPEC-<DOMAIN>-<ID>-v<N>`, `DEC-<DOMAIN>-<ID>`
- Superseded mechanisms must remain visible as historical records

### litenyx-plan (Planning)

- Maturity pipeline: `OPEN → CANDIDATE → RATIFIED → SPEC`
- OPEN and CANDIDATE material has no production-consensus authority
- The planning repository may frame alternatives — it does not authorize choosing unspecified consensus behavior
- Stable identifiers: `CRIT-<N>`, `OPEN-<DOMAIN>-<ID>`, `CAND-<DOMAIN>-<ID>`, `DEC-<DOMAIN>-<ID>`

### litenyx-walkthrough (Walkthrough)

- Walkthrough ≠ SpecificationAuthority
- If a walkthrough conflicts with an authoritative source, the walkthrough is wrong
- Identify provenance: Derived from, Decision, Verified by, Status
- Do not bulk-copy historical white papers

### Litenyx-oracle (Oracle / Provenance Index)

- Read-only index — never modifies source documents
- References, not copies — store repository, path, commit, hash
- All claims must have a confidence level
- AI classification must be marked INFERRED, never silently promoted to OBSERVED
- When Oracle and source disagree, the source wins
