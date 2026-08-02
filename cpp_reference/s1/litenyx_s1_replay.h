// Litenyx S1 — Deterministic Consensus Replay Harness (CONSENSUS_REPLAY).
//
// S1 is NOT an AuxPoW simulator, NOT a topology-redesign, NOT a parameter tuner.
// It is a deterministic VERIFICATION SHELL around the frozen Phase-4 topology
// implementation. The oracle is the frozen production engine:
//
//     Frozen Consensus Engine  ->  { Production Validation, S1 Consensus Replay }
//
// This adapter links DIRECTLY against the authoritative frozen implementation:
//   * LitenyxTopologyTracker  (production validation entry points: Connect/
//                             Disconnect/Tick/Reset) -- the deferred-transition
//                             state machine.
//   * LitenyxTopologyAuthority (pure derivation + TopologyStateHash) -- used as a
//                             cross-check oracle, never as the source of truth for
//                             replay; both are frozen and must agree.
//
// No consensus constant is copied. Every value originates from the linked frozen
// headers (LITENYX_TOPOLOGY_OBS_WINDOW, _COOLDOWN, _HYST_HIGH/LOW,
// LITENYX_TOPO_MAX_CHAINS, LITENYX_MIN_CHAINS, LITENYX_TOPOLOGY_STATE_VERSION).
//
// DEFERRED-TRANSITION SEMANTICS (frozen, authoritative):
//   Decision at h_obs -> cache scheduled transition
//   -> h_t = LitenyxTopoTransitionHeight(h_obs)
//   -> apply N+/-1 (recorded in m_transitions[h_t]); nLastTransition = h_t.
//
// S1 reproduces EXACTLY that machinery; it never drops a signal and never applies
// an immediate (non-deferred nLastTransition) model.
//
// Authority hierarchy (S1 §1): frozen implementation wins over older docs/prose.

#ifndef LITENYX_S1_REPLAY_H
#define LITENYX_S1_REPLAY_H

#include <litenyx/LITENYX_topology_tracker.h>
#include <litenyx/LITENYX_topology_authority.h>

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <sstream>

// S1 simulator modes. Only CONSENSUS_REPLAY is implemented in S1.
// EXPERIMENTAL is declared but MUST NOT be able to masquerade as consensus replay.
enum class S1SimulatorMode {
    CONSENSUS_REPLAY = 0,
    EXPERIMENTAL = 1
};

// A canonical block as fed to S1 replay (only canonical data is used).
struct S1Block {
    uint32_t height = 0;
    uint8_t  chainId = 0;
    int      Mc = 0; // normalized demand pressure 0..100 (already derived by caller
                     // from canonical GetBlockWeight via the frozen LitenyxDemandV1/
                     // LitenyxMcV1/LitenyxMcToControllerInput pipeline if desired)
};

// Provenance record emitted on every S1 run (S1 §11).
struct S1Provenance {
    S1SimulatorMode mode = S1SimulatorMode::CONSENSUS_REPLAY;
    std::string     oracleCommit;       // frozen oracle SHA
    std::string     specVersion;        // spec version string
    std::string     parameterSetHash;   // fingerprint of frozen consensus config
    uint64_t        seed = 0;           // replay input seed (0 == deterministic)
    bool            runtimeOverrides = false; // MUST stay false in CONSENSUS_REPLAY

    std::string ToString() const {
        std::ostringstream os;
        os << "S1_PROVENANCE{"
           << "mode=" << (mode == S1SimulatorMode::CONSENSUS_REPLAY
                              ? "CONSENSUS_REPLAY" : "EXPERIMENTAL")
           << ",commit=" << oracleCommit
           << ",spec=" << specVersion
           << ",paramHash=" << parameterSetHash
           << ",seed=" << seed
           << ",runtime_overrides=" << (runtimeOverrides ? "YES" : "none")
           << "}";
        return os.str();
    }
};

// S1 consensus-replay harness. Wraps the FROZEN LitenyxTopologyTracker.
// All replay goes through the production Connect/Disconnect/Tick entry points so
// the deferred-transition state machine is exercised exactly as in validation.
class LitenyxS1Replay {
public:
    // oracleCommit / specVersion are captured for provenance only; they do not
    // change consensus behavior.
    explicit LitenyxS1Replay(const std::string& oracleCommit = "6855d2f79f7993208f3a2fe16069367178f1eb94",
                             const std::string& specVersion = "topology-authority-v0.1",
                             uint64_t seed = 0)
        : m_oracleCommit(oracleCommit), m_specVersion(specVersion), m_seed(seed)
    {
        Reset();
    }

    void Reset() {
        // Use the frozen tracker's own reset (no consensus parameter overrides).
        LitenyxTopologyTracker::Instance().Reset();
        m_blocks.clear();
    }

    // Replay a single canonical block via the production Connect path.
    void ConnectBlock(const S1Block& b) {
        LitenyxTopologyTracker::Instance().Connect(b.chainId, b.height, b.Mc);
        if (!m_blocks.empty() && m_blocks.back().height >= b.height) {
            // out-of-order insert: keep sorted by height for deterministic replay
            m_blocks.push_back(b);
            std::sort(m_blocks.begin(), m_blocks.end(),
                      [](const S1Block& x, const S1Block& y){ return x.height < y.height; });
        } else {
            m_blocks.push_back(b);
        }
    }

    // Force an observation boundary finalize at `height` (production Tick path).
    void Tick(uint32_t height) {
        LitenyxTopologyTracker::Instance().Tick(height);
    }

    // Disconnect a single block via the production Disconnect path.
    void DisconnectBlock(const S1Block& b) {
        LitenyxTopologyTracker::Instance().Disconnect(b.chainId, b.height, b.Mc);
    }

    // Read current replay state from the linked frozen tracker.
    uint8_t ActiveChains() const { return LitenyxTopologyTracker::Instance().Chains(); }
    uint32_t LastTransition() const { return LitenyxTopologyTracker::Instance().LastTransition(); }

    // Build an authoritative TopologyState at the current height for hashing/cross-
    // check. The tracker's Chains()/LastTransition() are the live replay values.
    LitenyxTopologyState CurrentState(uint32_t height) const {
        LitenyxTopologyState s;
        s.nVersion = LITENYX_TOPOLOGY_STATE_VERSION;
        s.nHeight = height;
        s.nN = ActiveChains();
        s.nLastTransition = LastTransition();
        return s;
    }

    // Provenance fingerprint of the FROZEN consensus configuration actually in
    // force (NOT an empty-object hash). Derived from the authoritative constants.
    static std::string ParameterSetHash() {
        std::ostringstream os;
        os << "OBS_WINDOW=" << LITENYX_TOPOLOGY_OBS_WINDOW
           << ";COOLDOWN=" << LITENYX_TOPOLOGY_COOLDOWN
           << ";HYST_HIGH=" << LITENYX_TOPOLOGY_HYST_HIGH
           << ";HYST_LOW=" << LITENYX_TOPOLOGY_HYST_LOW
           << ";MAX_CHAINS=" << LITENYX_TOPO_MAX_CHAINS
           << ";MIN_CHAINS=" << (int)LITENYX_MIN_CHAINS
           << ";STATE_VERSION=" << LITENYX_TOPOLOGY_STATE_VERSION;
        return os.str();
    }

    S1Provenance Provenance() const {
        S1Provenance p;
        p.mode = S1SimulatorMode::CONSENSUS_REPLAY;
        p.oracleCommit = m_oracleCommit;
        p.specVersion = m_specVersion;
        p.parameterSetHash = ParameterSetHash();
        p.seed = m_seed;
        p.runtimeOverrides = false; // frozen constants only
        return p;
    }

    const std::vector<S1Block>& Blocks() const { return m_blocks; }

private:
    std::string m_oracleCommit;
    std::string m_specVersion;
    uint64_t    m_seed = 0;
    std::vector<S1Block> m_blocks;
};

#endif // LITENYX_S1_REPLAY_H
