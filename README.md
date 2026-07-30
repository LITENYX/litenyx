# Litenyx

> **Current-authority notice (2026-07-24):** this repository contains historical constitutional/experimental material. Before treating an older `LOCKED`, `FROZEN`, `MANDATORY`, or `CONSTITUTIONAL` statement as current architecture, read `docs/CURRENT_AUTHORITY.md`. Historical labels do not independently establish current authority; relaxability does not imply removal.

**Experimental protocol proving ground — built on Dogecoin Core 1.14.9.**

Litenyx extends Dogecoin with topology, delta state, and geographic reward research. It is an
engineering/experimental foundation, not a production network.

## Status

**Experimental / Research.** No mainnet. No real value. No guarantee of stability,
backward compatibility, or chain persistence.

## Building

See `doc/build-unix.md` or `doc/build-windows.md` for platform-specific instructions.

Quick start (Linux):

```bash
cd deploy
make m3-integration   # build debug daemon + run KATs + regtest gate
```

## Tests

```
cd deploy
make m3-integration   # fail-closed M3 integration gate
```

This runs:

- Pin verification against the upstream Dogecoin anchor
- Prerequisite patch application
- Debug daemon build (fault-injection capable)
- M3 class delta KATs (6/6 required)
- G1–G3 regtest harness (zero SKIP/FAIL required)

## Current Verified Milestone

```
Milestone:   DBET-D0-HARNESS-COMPLETION-1
Status:      COMPLETE — authorized with execution evidence
Date:        2026-07-30

D0 specification freeze: 1,212 vectors (V_D0(1))
Harness:                 882 behavioral + 29 negative control + 5 integrity tests
Test execution:          916 / 916 passed (2.20 s)
Stubs eliminated:        979 → 0
R0 reconciliation:       385 vectors (55 new PW tests + 330 COVERED_BY_EXISTING)
Invariant preserved:     Construct(D0) ⇏ Modify(V_D0(1))
Gate:                    DBET-D0-HARNESS-COMPLETION-1 → AUTHORIZED

Evidence artifacts:
  litenyx-plan/research/DBET-D0-SUBSTRATE-CONSTRUCTION-1/d0-test-harness/
    ├── EXECUTION-REPORT.json     (phase: EXECUTION_COMPLETE, status: ALL_916_PASSED)
    ├── TRACEABILITY-INDEX.csv    (N_accounted=1212, N_unaccounted=0)
    └── tests/                    (916 tests, all passing)
```

## Workspace governance

| Repository | Responsibility |
|---|---|
| litenyx | Implementation |
| litenyx-spec | Normative specification |
| litenyx-plan | Planning and execution sequencing |
| litenyx-walkthrough | Reproducible execution history |
| Litenyx-oracle | Provenance and evidence reconciliation |

## License

Litenyx is derived from [Dogecoin Core](https://github.com/dogecoin/dogecoin),
which is derived from Bitcoin Core. See `COPYING` for the upstream license.

Additional Litenyx contributions are distributed under the same terms.
