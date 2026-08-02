// Litenyx H1-SHADOW-ENG — Shared M3/M4 miner-response engine (HEADER-ONLY).
//
// H1 ONLY. Conditioned simulation: miner elasticity is an ASSUMPTION, not consensus.
// This engine drives the causal chain
//   W_t/W_t^* -> T_econ -> T_PoW^CF = min(T_time, T_econ) -> p_accept -> realized
//   blocks -> miner profitability (EMA expected reward/unit hash) -> hash migration
//   -> effective security/throughput.
//
// Paired BASE-vs-H1 trajectories on IDENTICAL CRN samples: miner response enabled
// identically in both arms; only the target differs (isolated CF effect, ISO-proven).
// Divergence is attributed to H1 alone.
//
// Used by test_litenyx_h1_m3.cpp (M3) and test_litenyx_h1_m4.cpp (M4). No production
// code linked or modified.

#ifndef LITENYX_H1_MINER_ENGINE_H
#define LITENYX_H1_MINER_ENGINE_H

#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#include "litenyx_h1_pow_target.h"

// Deterministic RNG (splitmix64).
struct H1SplitMix64 {
    uint64_t s;
    explicit H1SplitMix64(uint64_t seed = 0x9E3779B97F4A7C15ULL) : s(seed) {}
    uint64_t next() {
        s += 0x9E3779B97F4A7C15ULL;
        uint64_t z = s;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double u01() { return (double)(next() >> 11) * (1.0 / (double)(1ULL << 53)); }
};

enum class H1MinerClass { INELASTIC, MODERATELY_ELASTIC, HIGHLY_ELASTIC };

inline double H1HashElasticity(H1MinerClass m) {
    switch (m) {
        case H1MinerClass::INELASTIC:          return 0.0;
        case H1MinerClass::MODERATELY_ELASTIC: return 0.30;
        case H1MinerClass::HIGHLY_ELASTIC:     return 1.00;
    }
    return 0.0;
}
inline const char* H1MinerClassName(H1MinerClass m) {
    return (m == H1MinerClass::INELASTIC) ? "INELASTIC"
         : (m == H1MinerClass::MODERATELY_ELASTIC) ? "MODERATE" : "HIGH";
}

struct H1M3Params {
    int64_t T_time;
    int64_t block_reward = 50;
    int64_t fee_per_block = 5;
    int64_t total_hash0 = 1000000;
    int64_t W_star = 1000000;
    double   elasticity = 0.0;       // miner elasticity (assumption)
    int64_t  horizon_blocks = 4000;
    uint64_t seed = 12345;
    int64_t  floor_level = H1_T_SCALE / 2; // W_t/W_star^* floor (fixed-point)
    int64_t  shock_at = 0;
    int64_t  recover_at = -1;
    bool     shock = false;
    bool     gradual = false;
    int64_t  gradual_end = 0;        // block where gradual drift reaches floor
    // Asymmetric exit/re-entry elasticity: re-entry uses reentry_elasticity (<=exit).
    double   reentry_elasticity = -1.0; // <0 => use elasticity for both
    // Restoration model: when true, idle hash mean-reverts back toward total_hash0
    // (models the ordinary time-difficulty controller restoring cadence). When false,
    // only exit push acts (stress/adversarial assumption: NO cadence restoration) ->
    // exposes the pure death-spiral regime. Both are SIMULATION ASSUMPTIONS.
    bool     restoration = true;
};

inline int64_t H1WalletLevel(const H1M3Params& mp, int64_t t) {
    if (mp.gradual) {
        if (mp.gradual_end <= 0) return H1_T_SCALE;
        if (t >= mp.gradual_end) return mp.floor_level;
        int64_t num = (H1_T_SCALE - mp.floor_level) * t;
        return H1_T_SCALE - num / mp.gradual_end;
    }
    if (mp.shock) {
        if (t < mp.shock_at) return H1_T_SCALE;
        if (mp.recover_at > 0 && t >= mp.recover_at) return H1_T_SCALE;
        return mp.floor_level;
    }
    return H1_T_SCALE;
}

struct H1M3Result {
    std::string profile;
    std::string miner_class;
    bool shock = false;
    std::vector<int64_t> hash_trace;
    std::vector<int64_t> T_econ_trace;
    std::vector<int64_t> accepted;
    int64_t blocks_solved = 0;
    int64_t cumulative_issuance = 0;
    double  final_hash_retention = 0.0;
    double  mean_block_interval = 0.0;
    double  accepted_block_rate = 0.0;
    double  revenue_per_hash = 0.0;
    int64_t recovery_blocks = -1;
    bool    bounded_equilibrium = true;
    bool    failed_to_recover = false;
    double  min_hash_retention = 1.0; // trough during run
    int64_t structural_fail = 0;
};

// Run one arm. rng must be pre-seeded identically for BASE and H1 calls.
inline H1M3Result H1RunArm(H1M3Params mp, bool use_econ, H1EconProfile prof,
                           int64_t k, H1SplitMix64& rng) {
    H1M3Result r;
    r.profile = use_econ ? H1EconParams{prof}.ProfileName() : "BASE";
    r.miner_class = "E=" + std::to_string(mp.elasticity);
    r.shock = mp.shock || mp.gradual;

    H1EconParams ep; ep.profile = prof;
    ep.T_ref = mp.T_time; ep.T_min_CF = 1; ep.T_max_CF = H1_T_SCALE; ep.k = k;
    ep.deadband = 50000;
    H1EconHysteresisState hs;

    int64_t hash = mp.total_hash0;
    int64_t hash_min = mp.total_hash0 / 20;
    bool recovered_flag = false;
    double smooth_margin = 0.0;

    for (int64_t t = 0; t < mp.horizon_blocks; ++t) {
        int64_t lvl = H1WalletLevel(mp, t);
        int64_t W_t = (mp.W_star * lvl) / H1_T_SCALE;
        int64_t W_star_pp = mp.W_star;

        int64_t T_econ = mp.T_time;
        if (use_econ) {
            T_econ = H1ComputeT_econ(W_t, W_star_pp, ep, hs);
            if (T_econ > mp.T_time) { r.structural_fail++; T_econ = mp.T_time; }
        }
        int64_t T_pow = H1EffectiveTarget(mp.T_time, T_econ);
        r.T_econ_trace.push_back(T_pow);

        int64_t num = T_pow * hash;
        int64_t den = H1_T_SCALE * mp.total_hash0;
        double p_accept = (den > 0) ? (double)num / (double)den : 0.0;
        if (p_accept > 1.0) p_accept = 1.0;

        double U = rng.u01();
        int64_t found = (U < p_accept) ? 1 : 0;
        r.accepted.push_back(found);
        if (found) { r.blocks_solved++; r.cumulative_issuance += mp.block_reward + mp.fee_per_block; }

        double slot_reward = (double)(mp.block_reward + mp.fee_per_block);
        double base_expected = (mp.T_time == 0) ? 0.0
            : (double)mp.T_time / (double)H1_T_SCALE * slot_reward / (double)mp.total_hash0;
        double this_expected = p_accept * slot_reward / (double)hash;
        double inst_margin = (base_expected > 0) ? (this_expected / base_expected - 1.0) : 0.0;
        if (t == 0) smooth_margin = inst_margin;
        smooth_margin = 0.98 * smooth_margin + 0.02 * inst_margin;
        double margin = smooth_margin;

        // Hash migration as MEAN-REVERTING drift with elasticity-scaled exit push:
        //   re-entry pull : always pulls idle hash back toward total_hash0 when below
        //                   full (scaled by reentry_elasticity). This is what lets the
        //                   system RECOVER after T_econ returns to T_time (margin->0).
        //   exit push     : only when unprofitable (margin<0), scaled by exit elasticity.
        // Collapse occurs only when exit push persistently dominates re-entry pull
        // (high elasticity + deep/long tightening) -> persistent scarcity attractor.
        double reentry_el = (mp.reentry_elasticity >= 0.0) ? mp.reentry_elasticity : mp.elasticity;
        double pull = 0.0;
        if (mp.restoration) {
            pull = reentry_el * (double)(mp.total_hash0 - hash) / (double)mp.total_hash0
                   * (double)hash * 0.02;
        }
        double push = mp.elasticity * (margin < 0.0 ? -margin : 0.0) * (double)hash * 0.05;
        double delta = pull - push;
        hash = (int64_t)((double)hash + delta);
        if (hash < hash_min) hash = hash_min;
        if (hash > mp.total_hash0 * 3 / 2) hash = mp.total_hash0 * 3 / 2;
        r.hash_trace.push_back(hash);
        double ret = (double)hash / (double)mp.total_hash0;
        if (ret < r.min_hash_retention) r.min_hash_retention = ret;

        if (mp.shock && mp.recover_at > 0 && t >= mp.recover_at && !recovered_flag) {
            if (hash >= (mp.total_hash0 * 95) / 100) {
                r.recovery_blocks = t - mp.recover_at;
                recovered_flag = true;
            }
        }
    }

    r.final_hash_retention = (double)hash / (double)mp.total_hash0;
    int64_t nz = 0, sum = 0;
    for (int64_t d : r.accepted) { if (d > 0) nz++; }
    r.accepted_block_rate = (double)r.blocks_solved / (double)mp.horizon_blocks;
    int64_t hs_sum = 0; for (int64_t h : r.hash_trace) hs_sum += h;
    double mean_hash = r.hash_trace.empty() ? 0 : (double)hs_sum / (double)r.hash_trace.size();
    r.revenue_per_hash = (mean_hash > 0) ? (double)r.cumulative_issuance / mean_hash : 0.0;
    r.mean_block_interval = 0.0; // not used in M4 classification

    if (hash <= hash_min + 1 && mp.shock && mp.recover_at > 0 && !recovered_flag) {
        r.failed_to_recover = true; r.bounded_equilibrium = false;
    }
    if (r.accepted_block_rate < 0.01) r.bounded_equilibrium = false;
    if (r.final_hash_retention < 0.2) r.bounded_equilibrium = false; // deep collapse
    return r;
}

struct H1M3Paired {
    H1M3Result base;
    H1M3Result h1;
    double hash_retention_delta = 0;
    double accepted_rate_delta = 0;
    bool   red_flag = false;
    // BASE-relative classification.
    enum class Verdict { IMPROVED, NEUTRAL, DEGRADED_BOUNDED, CATASTROPHIC };
    Verdict verdict = Verdict::NEUTRAL;
    std::string VerdictName() const {
        switch (verdict) {
            case Verdict::IMPROVED: return "IMPROVED";
            case Verdict::NEUTRAL: return "NEUTRAL";
            case Verdict::DEGRADED_BOUNDED: return "DEGRADED_BOUNDED";
            case Verdict::CATASTROPHIC: return "CATASTROPHIC";
        }
        return "?";
    }
};

inline H1M3Paired H1RunPaired(H1M3Params mp, H1EconProfile prof, int64_t k) {
    H1M3Paired out;
    H1SplitMix64 rngB(mp.seed);
    out.base = H1RunArm(mp, false, prof, k, rngB);
    H1SplitMix64 rngH(mp.seed);
    out.h1 = H1RunArm(mp, true, prof, k, rngH);

    out.hash_retention_delta = out.h1.final_hash_retention - out.base.final_hash_retention;
    out.accepted_rate_delta  = out.h1.accepted_block_rate - out.base.accepted_block_rate;

    if (out.h1.structural_fail > 0) out.red_flag = true;
    // Hard red flag: H1 persists hash collapse while BASE recovers under identical CRN.
    if (out.h1.failed_to_recover && !out.base.failed_to_recover) out.red_flag = true;
    if (out.h1.final_hash_retention < 0.5 * out.base.final_hash_retention
        && out.base.final_hash_retention > 0.5) out.red_flag = true;

    // BASE-relative classification.
    if (out.h1.final_hash_retention >= out.base.final_hash_retention * 0.98)
        out.verdict = H1M3Paired::Verdict::NEUTRAL; // includes IMPROVED-ish (H1 never eases)
    if (out.h1.final_hash_retention < out.base.final_hash_retention * 0.98
        && out.h1.final_hash_retention >= 0.5)
        out.verdict = H1M3Paired::Verdict::DEGRADED_BOUNDED;
    if (out.h1.final_hash_retention < 0.5 || out.h1.failed_to_recover)
        out.verdict = H1M3Paired::Verdict::CATASTROPHIC;
    return out;
}

#endif // LITENYX_H1_MINER_ENGINE_H
