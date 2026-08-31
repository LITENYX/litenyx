"""Litenyx SSS cold-restart rehydration integration test (CONTRACT-v0.2).

Proves the frozen SSS-REHYDRATION-CONTRACT-v0.2 RE2/RE3 contract against a BUILT
(forked) dogecoind: the global LitenyxSharedSpendSet is deterministically
reconstructed on daemon restart by REPLAY of canonical chain history (derivation),
skipping the daemon's in-memory-only SSS that is lost on process exit.

Scenarios:
  * R1 (primary): establish a canonical spend, clean-stop the daemon, cold-restart
    on the SAME datadir, and assert the spent outpoint is reported spent by
    `testlitenyxsharedstate query` — i.e. the set was rehydrated from chain, not
    from any serialized snapshot (there is none; SSS is pure in-memory).
  * R2 (read-ordering): the rehydration completes during Phase 7, before RPC
    warmup-finished, so the very first query after startup already reflects the
    canonical spends.

Requires (identical to test_litenyx_m3_integration.py):
  * dogecoind/dogecoin-cli built from pinned base
    (Dogecoin Core v1.14.9, commit e0a1c157791544e818c901bd9341896965afbf9d),
  * the litenyx patches applied, including deploy/patches/litenyx-rehydrate.patch
    (which wires LitenyxRehydrateSharedSpendSet into AppInitMain end-of-Phase-7),
  * LITENYX-sharedstate RPC ("query"/"record"/"revert") present.

STATUS (this environment): AUTHORED, READY-TO-RUN for CI (which builds the daemon).
Like its M3 sibling, this local MSYS2/UCRT64 install cannot produce a
production dogecoind build, so the harness is committed and left to CI to execute.
"""

from __future__ import annotations

import json
import os
import random
import shutil
import subprocess
import tempfile
import time


# Reuse the mandatory binary resolution + RPC scaffolding from the M3 sibling.
def _resolve(bin_env, name):
    if bin_env:
        if not os.path.exists(bin_env):
            raise RuntimeError(
                f"Litenyx binary not found at explicit path {bin_env!r} "
                f"(set LITENYXD_BIN / LITENYX_CLI_BIN to the built dogecoind/dogecoin-cli)"
            )
        return bin_env
    found = shutil.which(name)
    if not found:
        raise RuntimeError(
            f"Litenyx binary {name!r} not found on PATH and no explicit "
            f"LITENYXD_BIN / LITENYX_CLI_BIN provided."
        )
    return found


LITENYXD = _resolve(os.environ.get("LITENYXD_BIN"), "dogecoind")
LITENYX_CLI = _resolve(os.environ.get("LITENYX_CLI_BIN"), "dogecoin-cli")
RPC_USER = "Litenyx"
RPC_PASSWORD = "litenyxtest"
_WARMUP = ("couldn't connect", "EOF", "code -28", "Loading", "code -26", "Startup")


class _Dead(Exception):
    pass


class _Daemon:
    """Owns one dogecoind process + its CLI wrapper on a fixed datadir/port."""

    def __init__(self, datadir, rpc_port, env, log_suffix):
        self.datadir = datadir
        self.rpc_port = rpc_port
        self.env = env
        self.log_suffix = log_suffix
        self.proc = None
        self.out = None
        self.err = None

    def start(self):
        if self.proc is not None and self.proc.poll() is None:
            raise RuntimeError("daemon already running")
        self.out = open(
            os.path.join(self.datadir, f"dogecoind{self.log_suffix}.stdout.log"), "w"
        )
        self.err = open(
            os.path.join(self.datadir, f"dogecoind{self.log_suffix}.stderr.log"), "w"
        )
        self.proc = subprocess.Popen(
            [
                LITENYXD,
                f"-datadir={self.datadir}",
                "-regtest",
                "-txindex=1",
                "-wallet=w",
                "-debug",
                f"-rpcport={self.rpc_port}",
                f"-rpcuser={RPC_USER}",
                f"-rpcpassword={RPC_PASSWORD}",
                "-nolisten",
            ],
            env=self.env,
            stdout=self.out,
            stderr=self.err,
        )
        self.cli = [
            LITENYX_CLI,
            "-regtest",
            f"-datadir={self.datadir}",
            f"-rpcport={self.rpc_port}",
            f"-rpcuser={RPC_USER}",
            f"-rpcpassword={RPC_PASSWORD}",
        ]
        self._wait_ready()

    def _wait_ready(self):
        deadline = time.time() + 120
        while time.time() < deadline:
            if self.proc.poll() is not None:
                raise RuntimeError(
                    f"dogecoind died during init (exit {self.proc.poll()}) "
                    f": {open(os.path.join(self.datadir, 'debug.log')).read()[:2000]}"
                )
            try:
                self.rpc("getblockchaininfo")
                return
            except _Dead as e:
                raise RuntimeError(str(e))
            except RuntimeError:
                time.sleep(0.5)
        raise RuntimeError("dogecoind did not become ready within 120s")

    def rpc(self, method, *args):
        if self.proc.poll() is not None:
            raise _Dead("%s: daemon exited %s" % (method, self.proc.poll()))
        r = subprocess.run(
            self.cli + [method, *[str(a) for a in args]],
            capture_output=True,
            text=True,
            env=self.env,
        )
        if r.returncode != 0:
            if self.proc.poll() is not None:
                raise _Dead("%s: daemon exited %s" % (method, self.proc.poll()))
            if any(m in r.stderr for m in _WARMUP):
                raise RuntimeError("warmup: " + r.stderr)
            raise RuntimeError("RPC %s failed: %s" % (method, r.stderr))
        try:
            return json.loads(r.stdout.strip())
        except json.JSONDecodeError:
            return r.stdout.strip()

    def stop(self):
        if self.proc is None:
            return
        if self.proc.poll() is None:
            subprocess.run(
                self.cli + ["stop"], capture_output=True, text=True, env=self.env
            )
            try:
                self.proc.wait(timeout=30)
            except subprocess.TimeoutExpired:
                self.proc.terminate()
        if self.out:
            self.out.close()
        if self.err:
            self.err.close()
        self.proc = None


def _outpoints(specs):
    return json.dumps([{"txid": t, "n": n} for (t, n) in specs])


def _sss_spent(d, spec):
    q = d.rpc("testlitenyxsharedstate", "query", 0, _outpoints([spec]))
    return q["results"][0]["spent"]


def _test():
    datadir = tempfile.mkdtemp(prefix="Litenyx-ColdRestart-")
    rpc_port = random.randint(20000, 40000)
    env = dict(os.environ)
    try:
        # ---- Phase A: first boot. Mine past activation, spend a known UTXO. ----
        d1 = _Daemon(datadir, rpc_port, env, log_suffix=".boot1")
        d1.start()
        addr = d1.rpc("getnewaddress")
        d1.rpc("generatetoaddress", 101, addr)  # past segwit/aux activation

        # Pick a specific UTXO to spend, so we can query its exact outpoint later.
        utxo = d1.rpc("listunspent")[0]
        spent_op = (utxo["txid"], utxo["vout"])
        assert _sss_spent(d1, spent_op) is False, "precondition: UTXO starts unspent"

        raw = d1.rpc(
            "createrawtransaction",
            json.dumps([{"txid": utxo["txid"], "vout": utxo["vout"]}]),
            json.dumps({addr: utxo["amount"] - 0.0001}),
        )
        signed = d1.rpc("signrawtransaction", raw)
        hex_tx = signed["hex"] if isinstance(signed, dict) else signed
        d1.rpc("sendrawtransaction", hex_tx)
        d1.rpc("generatetoaddress", 1, addr)

        # The spend is now canonical and visible in the live (in-memory) SSS.
        assert _sss_spent(d1, spent_op) is True, (
            "precondition: canonical spend visible pre-restart"
        )

        height_before = d1.rpc("getblockcount")
        h1 = d1.rpc("getblockhash", height_before)
        d1.stop()

        # ---- Phase B: cold restart on the SAME datadir (rehydration path). ----
        d2 = _Daemon(datadir, rpc_port, env, log_suffix=".boot2")
        d2.start()

        # R2 read-ordering: the very first query already reflects the canonical
        # spend (rehydration ran in Phase 7, before RPC warmup-finished).
        assert _sss_spent(d2, spent_op) is True, (
            "R1/R2 VIOLATED: SSS not rehydrated from chain after cold restart "
            "(expected spent=True, got False)"
        )
        # Chain unchanged across restart (same canonical tip; replay was exact).
        assert d2.rpc("getblockcount") == height_before
        assert d2.rpc("getblockhash", height_before) == h1
        d2.stop()
    finally:
        shutil.rmtree(datadir, ignore_errors=True)


def test_litenyx_cold_restart_rehydrates_sss():
    """RE2/RE3: canonical spends survive a daemon restart via chain replay."""
    _test()
