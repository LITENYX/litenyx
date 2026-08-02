// Litenyx H1 — MON-ONTOLOGY-1 Candidate Policy Shadow / Counterfactual Engine.
//
// H1 is COUNTERFACTUAL. It evaluates candidate economic policies A (geometric lane
// mining subsidy) and B (negative-supply hysteresis) WITHOUT any authority over
// consensus. It reads an IMMUTABLE canonical snapshot and produces candidate
// results explicitly labeled observed/derived/counterfactual/simulated.
//
// AUTHORITY HIERARCHY (frozen > candidate > counterfactual):
//   Frozen Consensus  >  Candidate Economic Policy  >  Counterfactual Simulation
//
// STRUCTURAL ISOLATION (INV-1 / INV-2 / INV-3):
//   * W_t  -/-> N_h   (wallet population never commands topology)
//   * G_c  -/-> N_h   (gross mining revenue never commands topology)
//   * Delta S_t -/-> N_h
//   * Lane availability never alters transaction validity.
// The H1 API receives the canonical topology state (nN, nLastTransition) as a
// READ-ONLY value and has NO mutable handle to the tracker or to any transition
// function. It cannot call LitenyxTopoDecide/Apply/TransitionHeight with policy
// inputs, and it receives no pointer/reference to LitenyxTopologyTracker.

#ifndef LITENYX_H1_CANDIDATE_POLICY_H
#define LITENYX_H1_CANDIDATE_POLICY_H

#include <litenyx/LITENYX_topology_authority.h>
#include <litenyx/LITENYX_types.h>

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

// Immutable canonical snapshot handed to H1. Contains NO mutation handle.
struct H1CanonicalSnapshot {
    uint32_t nHeight = 0;
    uint8_t  nN = LITENYX_MIN_CHAINS;       // active lane count (authoritative)
    uint32_t nLastTransition = 0;
    // Optional canonical monetary facts (read-only; never authoritative for N_h).
    int64_t  P_t = 0;   // positive supply (observed)
    int64_t  N_t = 0;   // negative supply (observed, pressure only)
    int64_t  W_t = 0;   // wallet-space observation (observed)
    int64_t  W_t_star = 0; // wallet-space target (observed/input)

    // The snapshot is immutable from H1's perspective: provide a copy only.
    static H1CanonicalSnapshot FromTopologyState(const LitenyxTopologyState& s,
                                                 int64_t P=0,int64_t N=0,int64_t W=0,int64_t Ws=0) {
        H1CanonicalSnapshot snap;
        snap.nHeight = s.nHeight;
        snap.nN = s.nN;
        snap.nLastTransition = s.nLastTransition;
        snap.P_t = P; snap.N_t = N; snap.W_t = W; snap.W_t_star = Ws;
        return snap;
    }
};

// Candidate-policy result kinds (never authoritative consensus state).
enum class H1ResultKind {
    OBSERVED = 0,
    DERIVED = 1,
    COUNTERFACTUAL = 2,
    SIMULATED = 3
};

// Deterministic bounded-integer helper (checked floor division).
inline int64_t H1FloorDiv(int64_t num, int64_t den) {
    if (den == 0) return 0;
    // floor division for non-negative denom (denom is always a positive power of 2)
    return num / den; // denom > 0 in our usage -> standard C++ / is floor for >=0 num
}

// ---- MON-ONTOLOGY-1 Policy A: Geometric Lane Mining (candidate) ------------
// R(c,h) = floor( R_base(h) / 2^c ), for c in [0, N_h).
// G_c(h) = R(c,h) + F_c(h). All integer, bounded, deterministic.
struct H1GeometricReward {
    H1ResultKind kind = H1ResultKind::COUNTERFACTUAL;
    int64_t R_base = 0;     // designated base subsidy input (candidate)
    uint8_t N_h = 0;        // active lanes at h (authoritative, read-only)
    std::vector<int64_t> R_c; // per-lane candidate subsidy R(c,h)
    std::vector<int64_t> F_c; // per-lane observed fee (input)
    std::vector<int64_t> G_c; // per-lane candidate gross revenue

    // Pure function. Reads N_h as a value; never writes it.
    static H1GeometricReward Evaluate(int64_t R_base, uint8_t N_h,
                                      const std::vector<int64_t>& fees) {
        H1GeometricReward r;
        r.kind = H1ResultKind::COUNTERFACTUAL;
        r.R_base = R_base;
        r.N_h = N_h;
        for (uint8_t c = 0; c < N_h && c < LITENYX_TOPO_MAX_CHAINS; ++c) {
            // R(c,h) = floor(R_base / 2^c). 2^c computed in int64 to avoid overflow.
            int64_t denom = (int64_t)1 << c; // c < 8 -> safe
            int64_t Rc = H1FloorDiv(R_base, denom);
            int64_t Fc = (c < fees.size()) ? fees[c] : 0;
            r.R_c.push_back(Rc);
            r.F_c.push_back(Fc);
            r.G_c.push_back(Rc + Fc); // checked: both non-negative -> no overflow risk here
        }
        return r;
    }
};

// ---- MON-ONTOLOGY-1 Policy B: Negative-Supply Hysteresis (candidate) --------
// Delta S_t = P_t - N_t ; e_t = W_t - W_t^* ; smoothed error e_bar.
// This is an INDEPENDENT monetary candidate loop. It MUST NOT expose a topology
// mutation command. It returns only candidate monetary observations/derivations.
struct H1NegativeSupply {
    H1ResultKind kind = H1ResultKind::COUNTERFACTUAL;
    int64_t P_t = 0;
    int64_t N_t = 0;
    int64_t DeltaS_t = 0;     // P_t - N_t
    int64_t W_t = 0;
    int64_t W_t_star = 0;
    int64_t e_t = 0;          // W_t - W_t^*
    int64_t e_bar_t = 0;      // smoothed error (input: previous e_bar + alpha)
    int64_t alpha = 0;        // smoothing numerator (0..1000 scale, input)

    // Pure function over observed/input values. No topology handle. No N_h output.
    static H1NegativeSupply Evaluate(int64_t P, int64_t N, int64_t W, int64_t Ws,
                                     int64_t prevEbar, int64_t alphaNum) {
        H1NegativeSupply r;
        r.kind = H1ResultKind::COUNTERFACTUAL;
        r.P_t = P; r.N_t = N; r.W_t = W; r.W_t_star = Ws; r.alpha = alphaNum;
        r.DeltaS_t = P - N;             // can be negative (pressure metric)
        r.e_t = W - Ws;
        // e_bar_t = alpha*e_t + (1-alpha)*prevEbar ; alpha in 0..1000 (permille)
        int64_t a = alphaNum, oma = 1000 - alphaNum;
        r.e_bar_t = (a * r.e_t + oma * prevEbar) / 1000;
        return r;
    }
};

// Top-level H1 evaluation entry point. Takes an immutable snapshot + simulation
// inputs and returns candidate policy results. Returns NO production consensus
// mutation handle. The canonical snapshot is passed by VALUE.
struct H1CandidatePolicyResult {
    H1GeometricReward  reward;
    H1NegativeSupply   negSupply;
    uint8_t            authoritativeN_h = 0; // echoed read-only for provenance
};

inline H1CandidatePolicyResult EvaluateCandidatePolicy(
    const H1CanonicalSnapshot& snap,           // immutable
    int64_t R_base,                            // Policy A input
    const std::vector<int64_t>& fees,          // Policy A fee observation
    int64_t alphaNum)                          // Policy B smoothing input
{
    H1CandidatePolicyResult out;
    out.authoritativeN_h = snap.nN; // echoed, never written back
    out.reward = H1GeometricReward::Evaluate(R_base, snap.nN, fees);
    out.negSupply = H1NegativeSupply::Evaluate(snap.P_t, snap.N_t,
                                               snap.W_t, snap.W_t_star, 0, alphaNum);
    return out;
}

#endif // LITENYX_H1_CANDIDATE_POLICY_H
