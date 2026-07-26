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

## License

Litenyx is derived from [Dogecoin Core](https://github.com/dogecoin/dogecoin),
which is derived from Bitcoin Core. See `COPYING` for the upstream license.

Additional Litenyx contributions are distributed under the same terms.
