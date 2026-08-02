# AGENT.md — litenyx (Implementation)

## Repository role

Implementation of validated Litenyx protocol specifications. This repository contains C++ consensus code, test harnesses, build system, patches, and implementation evidence.

## Authority boundaries

- **Must implement** what is specified in `litenyx-spec` — never invent unresolved consensus behavior
- **May extend** test coverage, harness infrastructure, and build tooling without specification changes
- **Must not** modify frozen vectors or consensus predicates without explicit gate authorization
- **Evidence** produced here is VERIFIED when reproducible, OBSERVED when witnessed

## Current verified milestone

```
DBET-D0-HARNESS-COMPLETION-1  —  COMPLETE
916/916 tests passed, 1,212/1,212 frozen vectors accounted for, 0 stubs
```

## Next engineering frontier

To be determined by the project roadmap. Candidate areas include:
- Transition to D1 specification work
- MSF framework production hardening
- Next ratified gate implementation

## Agent rules

1. All changes must conform to the repository's authority model (docs/CURRENT_AUTHORITY.md)
2. Consensus behavior changes require prior specification in litenyx-spec
3. Test evidence must be reproducible and referenced in the oracle index
4. Build artifacts and logs are excluded from version control
5. Implementation worktree may remain dirty during active development
