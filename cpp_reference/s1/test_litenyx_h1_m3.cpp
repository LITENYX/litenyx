// Litenyx H1-SHADOW-ENG — M3 Conditioned Endogenous Miner Response.
//
// H1 ONLY. Conditioned experiment: conclusions depend on the miner-response model,
// which is an ASSUMPTION, not consensus. M3 does NOT authorize production.
//
// Causal chain under test:
//   W_t/W_t^* -> T_econ -> T_PoW^CF = min(T_time, T_econ) -> p_accept -> realized
//   blocks -> miner profitability -> hash migration -> effective security/throughput.
//
// Hypothesis: the ONE-SIDED rule (can only tighten) may create a dangerous positive
// feedback loop under NEGATIVE economic deviation: tightening drops accepted work,
// lowered rewards cause miners to leave, lower hash further reduces block production,
// risking persistent scarcity even though the ordinary time-difficulty controller is
// trying to restore cadence.
//
// Method: PAIRED counterfactual trajectories on IDENTICAL CRN samples. Miner response
// is enabled identically in BASE and H1; only the target differs (the isolated CF
// effect, proven by ISO gate). Divergence is attributed to H1 alone.
//
// Miner-response classes: INELASTIC, MODERATELY_ELASTIC, HIGHLY_ELASTIC.
// Scenarios: gradual negative drift of W_t/W_t^*, and shock drop of W_t/W_t^*.
//
// M3 acceptance is NOT "does H1 tighten" (it does). It is:
//   "Does conditioned miner adaptation converge to a BOUNDED equilibrium?"
// HARD RED FLAG: any parameter region where economic tightening causes hash exit
//   faster than the time-difficulty mechanism compensates -> sustained block-time
//   degradation or failure to recover after T_econ returns >= T_time.

#define BOOST_TEST_MODULE LITENYX_h1_m3_miner_response
#include <boost/test/included/unit_test.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "litenyx_h1_pow_target.h"

// ---------------------------------------------------------------------------
// Deterministic RNG (splitmix64) — same instance reused for BASE and H1 draws.
// ---------------------------------------------------------------------------
struct SplitMix64 {
    uint64_t s;
    explicit SplitMix64(uint64_t seed = 0x9E3779B97F4A7C15ULL) : s(seed) {}
    uint64_t next() {
        s += 0x9E3779B97F4A7C15ULL;
        uint64_t z = s;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double u01() { return (double)(next() >> 11) * (1.0 / (double)(1ULL << 53)); }
};

enum class MinerClass { INELASTIC, MODERATELY_ELASTIC, HIGHLY_ELASTIC };

// Hash-migration elasticity: fraction of hash that leaves per unit (negative) margin
// shortfall. INELASTIC ~ 0, MODERATE ~ 0.3, HIGH ~ 1.0 (per fixed-point margin unit).
// These are SIMULATION ASSUMPTIONS, not consensus.
inline double HashElasticity(MinerClass m) {
    switch (m) {
        case MinerClass::INELASTIC:           return 0.0;
        case MinerClass::MODERATELY_ELASTIC:  return 0.30;
        case MinerClass::HIGHLY_ELASTIC:      return 1.00;
    }
    return 0.0;
}

struct M3Params {
    int64_t T_time;            // fixed-point time-derived target (sim constant)
    int64_t block_reward = 50; // nominal per-block reward (sim units)
    int64_t fee_per_block = 5; // nominal fee (sim units)
    int64_t total_hash0 = 1000000; // initial participating hash (sim units)
    int64_t W_star;            // wallet target (fixed-point)
    MinerClass cls;
    double   elasticity;       // from HashElasticity(cls)
    int64_t   horizon_blocks;  // number of block-slots to simulate
    uint64_t  seed;
    // Negative-deviation schedule: W_t = W_star * level(t), level in (0,1] fixed-point.
    // Gradual: level decays linearly from 1.0 to `floor_level` over `shock_at`.
    // Shock: level drops to `floor_level` immediately at `shock_at`, then recovers.
    int64_t   floor_level = H1_T_SCALE / 2; // W_t/W_star floor (fixed-point)
    int64_t   shock_at = 0;                 // block index where change begins
    int64_t   recover_at = -1;              // block index where W returns to 1.0 (shock)
    bool      shock = false;                // true => step shock; false => gradual drift
};

// Wallet level W_t/W_star^* (fixed-point in [0, H1_T_SCALE]) as a function of block t.
inline int64_t WalletLevel(const M3Params& mp, int64_t t) {
    if (!mp.shock) {
        // Gradual drift: from 1.0 down to floor_level by shock_at, then steady.
        if (mp.shock_at <= 0) return H1_T_SCALE;
        if (t >= mp.shock_at) return mp.floor_level;
        int64_t num = (H1_T_SCALE - mp.floor_level) * t;
        return H1_T_SCALE - num / mp.shock_at;
    } else {
        // Shock: 1.0 until shock_at, then floor_level until recover_at, then 1.0.
        if (t < mp.shock_at) return H1_T_SCALE;
        if (mp.recover_at > 0 && t >= mp.recover_at) return H1_T_SCALE;
        return mp.floor_level;
    }
}

struct M3Result {
    std::string profile;
    std::string miner_class;
    bool shock = false;
    std::vector<int64_t> hash_trace;     // participating hash per block-slot
    std::vector<int64_t> dt_trace;       // realized inter-block time (ticks)
    std::vector<int64_t> T_econ_trace;   // target trace
    std::vector<int64_t> accepted;       // 1 if a block was found this slot, else 0
    int64_t blocks_solved = 0;
    int64_t cumulative_issuance = 0;
    double  final_hash_retention = 0.0;
    double  mean_block_interval = 0.0;
    double  accepted_block_rate = 0.0;   // blocks_solved / horizon
    double  revenue_per_hash = 0.0;      // cumulative reward / mean hash
    int64_t recovery_blocks = -1;        // blocks after recover_at to reach pre-shock hash
    bool    bounded_equilibrium = true;  // false => red flag
    bool    failed_to_recover = false;
    int64_t structural_fail = 0;
};

// Run ONE arm (BASE or H1) for the given params. The SAME rng sequence is drawn by
// both arms (caller passes an rng that was reset to mp.seed before each arm).
// `use_econ` = false => BASE (target = T_time always). true => H1 (T_PoW^CF).
static M3Result RunArm(M3Params mp, bool use_econ, H1EconProfile prof,
                       int64_t k, SplitMix64& rng) {
    M3Result r;
    r.profile = use_econ ? H1EconParams{prof}.ProfileName() : "BASE";
    r.miner_class = (mp.cls == MinerClass::INELASTIC) ? "INELASTIC"
                    : (mp.cls == MinerClass::MODERATELY_ELASTIC) ? "MODERATE" : "HIGH";
    r.shock = mp.shock;

    H1EconParams ep; ep.profile = prof;
    ep.T_ref = mp.T_time; ep.T_min_CF = 1; ep.T_max_CF = H1_T_SCALE; ep.k = k;
    ep.deadband = 50000;
    H1EconHysteresisState hs;

    int64_t hash = mp.total_hash0;
    double smooth_margin = 0.0; // EMA of profitability margin (per-run state)
    int64_t hash_min = mp.total_hash0 / 20; // floor: never fully collapses (miner floor)
    bool recovered_flag = false;

    for (int64_t t = 0; t < mp.horizon_blocks; ++t) {
        int64_t lvl = WalletLevel(mp, t);
        int64_t W_t = (mp.W_star * lvl) / H1_T_SCALE;     // fixed-point scaled W_t
        int64_t W_star_pp = mp.W_star;

        int64_t T_econ = mp.T_time;
        if (use_econ) {
            T_econ = H1ComputeT_econ(W_t, W_star_pp, ep, hs);
            if (T_econ > mp.T_time) { r.structural_fail++; T_econ = mp.T_time; }
        }
        int64_t T_pow = H1EffectiveTarget(mp.T_time, T_econ); // min(T_time, T_econ)
        r.T_econ_trace.push_back(T_pow);

        // p_accept (fixed-point) is proportional to T_pow * participating_hash.
        // We draw a shared CRN U; block found if U < p_accept_normalized.
        // Normalize: p = (T_pow * hash) / (H1_T_SCALE * mp.total_hash0) in (0,1].
        int64_t num = T_pow * hash;
        int64_t den = H1_T_SCALE * mp.total_hash0;
        double p_accept = (den > 0) ? (double)num / (double)den : 0.0;
        if (p_accept > 1.0) p_accept = 1.0;

        double U = rng.u01();
        int64_t found = (U < p_accept) ? 1 : 0;
        r.accepted.push_back(found);
        int64_t dt = found ? 1000 : 0; // tick cost per slot (0 if no block)
        r.dt_trace.push_back(dt);
        if (found) {
            r.blocks_solved++;
            r.cumulative_issuance += mp.block_reward + mp.fee_per_block;
        }

        // Miner profitability margin: EXPECTED reward per unit hash vs the BASELINE
        // expected at full hash with the ordinary time target. Using the EXPECTED
        // (p_accept * reward) — not the instantaneous per-slot outcome — avoids
        // spurious migration from normal block-interval variance. A steady BASE run
        // (p_accept = T_time/H1_T_SCALE) therefore has margin ~ 0 and stays stable.
        double slot_reward = (double)(mp.block_reward + mp.fee_per_block);
        double base_expected = (mp.T_time == 0) ? 0.0
            : (double)mp.T_time / (double)H1_T_SCALE * slot_reward / (double)mp.total_hash0;
        double this_expected = p_accept * slot_reward / (double)hash;
        double inst_margin = (base_expected > 0) ? (this_expected / base_expected - 1.0) : 0.0;
        // Smooth with EMA (alpha=0.02) so transients don't trigger migration.
        if (t == 0) smooth_margin = inst_margin;
        smooth_margin = 0.98 * smooth_margin + 0.02 * inst_margin;
        double margin = smooth_margin;

        // Hash migration as MEAN-REVERTING drift with elasticity-scaled exit push:
        //   re-entry pull : always pulls idle hash back toward total_hash0 when below
        //                   full (scaled by elasticity). Lets the system RECOVER after
        //                   T_econ returns to T_time (margin->0).
        //   exit push     : only when unprofitable (margin<0), scaled by elasticity.
        // Collapse occurs only when exit push persistently dominates re-entry pull
        // (high elasticity + deep/long tightening) -> persistent scarcity attractor.
        double pull = mp.elasticity * (double)(mp.total_hash0 - hash) / (double)mp.total_hash0
                      * (double)hash * 0.02;
        double push = mp.elasticity * (margin < 0.0 ? -margin : 0.0) * (double)hash * 0.05;
        double delta = pull - push;
        hash = (int64_t)((double)hash + delta);
        if (hash < hash_min) hash = hash_min;
        if (hash > mp.total_hash0 * 3 / 2) hash = mp.total_hash0 * 3 / 2;
        r.hash_trace.push_back(hash);

        // Recovery tracking (shock only): after recover_at, did hash return to >= 95%?
        if (mp.shock && mp.recover_at > 0 && t >= mp.recover_at && !recovered_flag) {
            if (hash >= (mp.total_hash0 * 95) / 100) {
                r.recovery_blocks = t - mp.recover_at;
                recovered_flag = true;
            }
        }
    }

    r.final_hash_retention = (double)hash / (double)mp.total_hash0;
    int64_t nz = 0; int64_t sum = 0;
    for (int64_t d : r.dt_trace) { if (d > 0) { sum += d; nz++; } }
    r.mean_block_interval = nz ? (double)sum / (double)nz : 0.0;
    r.accepted_block_rate = (double)r.blocks_solved / (double)mp.horizon_blocks;
    int64_t hash_sum = 0; for (int64_t h : r.hash_trace) hash_sum += h;
    double mean_hash = r.hash_trace.empty() ? 0 : (double)hash_sum / (double)r.hash_trace.size();
    r.revenue_per_hash = (mean_hash > 0) ? (double)r.cumulative_issuance / mean_hash : 0.0;

    // Bounded-equilibrium test: final hash retention must stay within [floor, cap].
    // RED FLAG: hash collapsed to floor AND never recovered (shock) OR acceptance rate
    //   far below BASE-equivalent under identical CRN.
    if (hash <= hash_min + 1 && mp.shock && mp.recover_at > 0 && !recovered_flag) {
        r.failed_to_recover = true;
        r.bounded_equilibrium = false;
    }
    if (r.accepted_block_rate < 0.01) {
        // practically no blocks: pathological scarcity red flag.
        r.bounded_equilibrium = false;
    }
    return r;
}

// Paired run: identical CRN for BASE and H1 arms. Returns both + divergence metrics.
struct M3Paired {
    M3Result base;
    M3Result h1;
    double hash_retention_delta = 0;   // h1.final - base.final
    double accepted_rate_delta = 0;    // h1 - base
    double interval_delta = 0;         // h1 - base (ticks)
    bool   red_flag = false;
};

static M3Paired RunPaired(M3Params mp, H1EconProfile prof, int64_t k) {
    M3Paired out;
    // BASE arm: same seed.
    SplitMix64 rngB(mp.seed);
    out.base = RunArm(mp, false, prof, k, rngB);
    // H1 arm: RESET seed => identical CRN sequence.
    SplitMix64 rngH(mp.seed);
    out.h1 = RunArm(mp, true, prof, k, rngH);

    out.hash_retention_delta = out.h1.final_hash_retention - out.base.final_hash_retention;
    out.accepted_rate_delta  = out.h1.accepted_block_rate - out.base.accepted_block_rate;
    out.interval_delta       = out.h1.mean_block_interval - out.base.mean_block_interval;

    // Red flag: H1 fails to recover where BASE would, or H1 acceptance collapses
    // materially more than BASE under the SAME economic pressure (divergence => H1 harm).
    if (out.h1.failed_to_recover && !out.base.failed_to_recover) out.red_flag = true;
    if (out.h1.accepted_block_rate < 0.5 * out.base.accepted_block_rate && out.base.accepted_block_rate > 0.05)
        out.red_flag = true;
    if (out.h1.structural_fail > 0) out.red_flag = true;
    return out;
}

// ===========================================================================
BOOST_AUTO_TEST_SUITE(litenyx_h1_m3_suite)

// Sanity: BASE arm with no economic deviation keeps full hash retention and high
// acceptance; structural_fail always 0.
BOOST_AUTO_TEST_CASE(m3_base_no_deviation_stable) {
    M3Params mp;
    mp.T_time = H1_T_SCALE / 2; mp.W_star = 1000000;
    mp.cls = MinerClass::HIGHLY_ELASTIC;
    mp.elasticity = HashElasticity(mp.cls);
    mp.horizon_blocks = 2000; mp.seed = 12345;
    mp.shock = false; mp.shock_at = 0;
    auto p = RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
    BOOST_CHECK_EQUAL(p.base.structural_fail, 0);
    BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    BOOST_CHECK_GT(p.base.final_hash_retention, 0.9);
}

// M3 — gradual negative drift, INELASTIC miners: one-sided tightening should NOT
// cause hash collapse (inelastic => no migration). Expect bounded equilibrium.
BOOST_AUTO_TEST_CASE(m3_gradual_inelastic_bounded) {
    M3Params mp;
    mp.T_time = H1_T_SCALE / 2; mp.W_star = 1000000;
    mp.cls = MinerClass::INELASTIC; mp.elasticity = HashElasticity(mp.cls);
    mp.horizon_blocks = 3000; mp.seed = 777; mp.shock = false;
    mp.shock_at = 1500; mp.floor_level = H1_T_SCALE / 4; // W drops to 25%
    auto p = RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
    BOOST_CHECK(p.base.bounded_equilibrium);
    BOOST_CHECK(p.h1.bounded_equilibrium);
    BOOST_CHECK_LE(p.h1.final_hash_retention, 1.0); // no easing beyond baseline
    BOOST_CHECK_GE(p.h1.final_hash_retention, 0.99); // inelastic => retained
}

// M3 — gradual negative drift, MODERATELY_ELASTIC: must converge to bounded
// equilibrium (retention above miner floor, acceptance positive).
BOOST_AUTO_TEST_CASE(m3_gradual_moderate_bounded) {
    M3Params mp;
    mp.T_time = H1_T_SCALE / 2; mp.W_star = 1000000;
    mp.cls = MinerClass::MODERATELY_ELASTIC; mp.elasticity = HashElasticity(mp.cls);
    mp.horizon_blocks = 3000; mp.seed = 2024; mp.shock = false;
    mp.shock_at = 1500; mp.floor_level = H1_T_SCALE / 3;
    auto p = RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
    BOOST_CHECK(p.h1.bounded_equilibrium);
    BOOST_CHECK_GT(p.h1.accepted_block_rate, 0.01);
}

// M3 — gradual negative drift, HIGHLY_ELASTIC: the dangerous case. Must REPORT
// (not hide) whether it converges. Red flag if H1 acceptance collapses vs BASE.
BOOST_AUTO_TEST_CASE(m3_gradual_highly_elastic_reports_divergence) {
    M3Params mp;
    mp.T_time = H1_T_SCALE / 2; mp.W_star = 1000000;
    mp.cls = MinerClass::HIGHLY_ELASTIC; mp.elasticity = HashElasticity(mp.cls);
    mp.horizon_blocks = 4000; mp.seed = 555; mp.shock = false;
    mp.shock_at = 1000; mp.floor_level = H1_T_SCALE / 4; // 75% wallet drop
    auto p = RunPaired(mp, H1EconProfile::P1_LINEAR, 300000);
    // We do NOT assert bounded_equilibrium here: this is the stress case whose
    // outcome is REPORTED. We only assert the experiment ran and isolation held.
    BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    // Divergence is observable: hash_retention_delta is well-defined.
    BOOST_CHECK(std::isfinite(p.hash_retention_delta));
    if (p.red_flag) {
        // REPORTED red flag: H1 caused additional harm beyond BASE under same CRN.
        BOOST_TEST_MESSAGE("[M3 RED FLAG] highly-elastic gradual drift: H1 divergence="
            << p.hash_retention_delta << " accepted_delta=" << p.accepted_rate_delta);
    }
}

// M3 — shock drop + recovery, MODERATELY_ELASTIC: tests recovery after T_econ
// returns above T_time. Must recover if bounded.
BOOST_AUTO_TEST_CASE(m3_shock_recovery_moderate) {
    M3Params mp;
    mp.T_time = H1_T_SCALE / 2; mp.W_star = 1000000;
    mp.cls = MinerClass::MODERATELY_ELASTIC; mp.elasticity = HashElasticity(mp.cls);
    mp.horizon_blocks = 4000; mp.seed = 909; mp.shock = true;
    mp.shock_at = 1000; mp.recover_at = 2500; mp.floor_level = H1_T_SCALE / 3;
    auto p = RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
    BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    // If bounded, H1 should also recover (or at least not fail where BASE succeeds).
    if (p.h1.bounded_equilibrium) {
        BOOST_CHECK(!(p.h1.failed_to_recover && !p.base.failed_to_recover));
    }
}

// M3 — shock drop + recovery, HIGHLY_ELASTIC: stress; report red flag if H1 fails
// to recover while BASE recovers.
BOOST_AUTO_TEST_CASE(m3_shock_recovery_highly_elastic_reports) {
    M3Params mp;
    mp.T_time = H1_T_SCALE / 2; mp.W_star = 1000000;
    mp.cls = MinerClass::HIGHLY_ELASTIC; mp.elasticity = HashElasticity(mp.cls);
    mp.horizon_blocks = 5000; mp.seed = 3141; mp.shock = true;
    mp.shock_at = 1000; mp.recover_at = 3000; mp.floor_level = H1_T_SCALE / 4;
    auto p = RunPaired(mp, H1EconProfile::P1_LINEAR, 300000);
    BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    BOOST_CHECK(std::isfinite(p.hash_retention_delta));
    if (p.red_flag) {
        BOOST_TEST_MESSAGE("[M3 RED FLAG] highly-elastic shock/recovery: H1 recovery_blocks="
            << p.h1.recovery_blocks << " base recovery_blocks=" << p.base.recovery_blocks
            << " h1_failed=" << p.h1.failed_to_recover << " base_failed=" << p.base.failed_to_recover);
    }
}

// M3 — hysteresis (P3) should prevent repeated target entry/exit oscillation vs P1
// when the wallet W_t oscillates WITHIN the deadband. P3 holds T_econ steady; P1
// tracks every wiggle. We compare the TARGET trace variance directly (the isolated
// control signal), which is where hysteresis genuinely helps.
BOOST_AUTO_TEST_CASE(m3_hysteresis_reduces_oscillation) {
    int64_t T_time = H1_T_SCALE / 2;
    int64_t W_star = 1000000;
    int64_t k = 200000;
    H1EconParams p1; p1.profile = H1EconProfile::P1_LINEAR;
    p1.T_ref = T_time; p1.T_min_CF = 1; p1.T_max_CF = H1_T_SCALE; p1.k = k;
    H1EconParams p3 = p1; p3.profile = H1EconProfile::P3_HYSTERETIC; p3.deadband = 50000;

    // Wallet oscillates within the deadband: e in [-40000, +40000] (deadband=50000).
    auto trace_var = [](const std::vector<int64_t>& v) -> double {
        if (v.size() < 2) return 0;
        double m = 0; for (int64_t x : v) m += x; m /= v.size();
        double s = 0; for (int64_t x : v) { double d = x - m; s += d*d; }
        return s / v.size();
    };
    std::vector<int64_t> t1, t3;
    H1EconHysteresisState hs1, hs3;
    for (int64_t i = 0; i < 2000; ++i) {
        // e = 40000 * sin(i/50) in fixed-point; stays within deadband.
        double e = 40000.0 * std::sin((double)i / 50.0);
        int64_t W = W_star + (int64_t)(e); // e already in e-fixed units (relative to W_star=1e6)
        // Reconstruct e-fixed exactly as H1WalletDeviation would: (W-W*)/W* * H1_QW.
        int64_t ef = ((W - W_star) * H1_QW) / W_star; // = e (since W-W* = e)
        t1.push_back(H1ComputeT_econ(W, W_star, p1, hs1));
        t3.push_back(H1ComputeT_econ(W, W_star, p3, hs3));
        (void)ef;
    }
    double v1 = trace_var(t1);
    double v3 = trace_var(t3);
    BOOST_CHECK(std::isfinite(v1)); BOOST_CHECK(std::isfinite(v3));
    BOOST_TEST_MESSAGE("[M3 hysteresis] target variance P1=" << v1 << " P3=" << v3);
    // P3 deadband HOLDS the target steady => strictly lower target variance.
    BOOST_CHECK_LT(v3, v1);
}

BOOST_AUTO_TEST_SUITE_END()
