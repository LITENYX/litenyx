# Litenyx

Litenyx is the canonical **implementation repository** for the Litenyx research and protocol program. The project currently uses a Dogecoin-derived scientific-control track (DBET) to establish evidence before promoting experimental mechanisms into normative protocol work.

## Repository role

This repository contains implementation source, harnesses, tests, adapters, build integration, CI, and implementation evidence.

It does **not** independently define protocol authority. Normative boundaries live in `litenyx-spec`; execution planning lives in `litenyx-plan`; actual execution history lives in `litenyx-walkthrough`; provenance and discrepancy reconciliation are indexed by `Litenyx-oracle`.

```text
Spec: SHOULD / MUST
        ↓
Plan: WILL
        ↓
Implementation: CODE / HARNESS / TESTS
        ↓
Walkthrough: DID
        ↓
Oracle: EVIDENCE / MEANING / PROVENANCE
```

## Current DBET checkpoint

Current frontier:

```text
DBET-D0-HARNESS-COMPLETION-1
```

Frozen behavioral-vector surface:

```text
V_D0(1) = 1,212 vectors
```

Current gate state:

- D0 harness completion: **ACTIVE FRONTIER**
- BV: **BLOCKED**
- D1+: **BLOCKED**
- Gate-1: **BLOCKED**

No downstream DBET result may be claimed until its prerequisite gate is actually evidenced.

## D0 scientific control

`deploy/external/dogecoin` is the pristine pinned Dogecoin D0 upstream evidence source.

D0 exists to provide a reproducible scientific control. Experimental Litenyx mechanisms must not silently contaminate D0.

The promotion path is:

```text
D0 control
  → Dn experiment
  → verified evidence
  → explicit graduation decision
  → normative specification
  → implementation
```

Experimental evidence does not promote itself into consensus.

## Authority and anti-oscillation discipline

Interpret project material proposition-by-proposition. Preserve evidence, but do not resurrect superseded conclusions because an older document says `LOCKED`, `FROZEN`, or `MANDATORY`.

```text
Authority status != historical evidence value != implementation existence
```

Implementation and tests prove only the path they actually exercise. A harness passing does not by itself establish production consensus enforcement.

## Canonical workspace

Canonical implementation root on the primary Windows workspace:

```text
C:\Users\sunilkr\New\Litenyx\Litenyx
```

The lowercase duplicate checkout is non-authoritative.

## Engineering rule

Before claiming a consensus feature is enforced, distinguish:

```text
Specified
→ Implemented
→ Compiled
→ Tested
→ Production-path exercised
→ Verified
→ Frozen
```

If implementation requires unresolved consensus semantics:

> **STOP — SPECIFICATION REQUIRED**

## Related repositories

- `litenyx-spec` — normative boundaries, invariants, contracts, gate definitions
- `litenyx-plan` — per-execution planning, sequencing, counters, next operation
- `litenyx-walkthrough` — commands/actions actually executed and their results
- `Litenyx-oracle` — provenance, reconciliation, evidence mapping, discrepancy analysis

Historical white papers and superseded experiments remain useful provenance, not automatic current authority.