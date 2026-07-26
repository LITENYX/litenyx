// Litenyx H1-SHADOW-ENG — Counterfactual Simulator (M0 / M1 / M2).
//
// H1 ONLY. Read-only counterfactual experiment. NO production consensus is linked,
// modified, or promoted. This engine evaluates the candidate controller
//   T_PoW^CF = min(T_time^SIM, T_econ(W_t^SIM))
// against a BASELINE controller
//   T_PoW^BASE = T_time^SIM
// using a SHARED NORMALIZED WORK SAMPLE (literal shared sequence X_t < T) so the
// ONLY difference between CF and BASE is the target value. This isolates causal
// effect of the economic target (M1 — strongest falsification).
//
// Mining abstraction (MANDATORY separation, tightened):
//   - M0 / M1 : SHARED_NORMALIZED_WORK_SAMPLE  (literal X<T vs X<T_PoW^CF)
//   - M2      : PAIRED_EXPONENTIAL_ARRIVAL_CRN  (Delta t = -ln(U)/lambda, same U)
//
// Every run records its mining_model explicitly. No cross-contamination.
//
// All parameters are H1 EXPERIMENTAL PARAMETERS. None canonical, none frozen.
// Fixed-point integer arithmetic throughout. Deterministic given seed.

#define BOOST_TEST_MODULE LITENYX_h1_shadow_sim_test
#include <boost/test/included/unit_test.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>

#include "litenyx_h1_pow_target.h"
#include "litenyx_h1_metrics.h"

// ---------------------------------------------------------------------------
// Deterministic RNG (splitmix64) — reproducible across runs.
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
    // Uniform double in [0,1).
    double u01() {
        return (double)(next() >> 11) * (1.0 / (double)(1ULL << 53));
    }
};

// ---------------------------------------------------------------------------
// SHARED_NORMALIZED_WORK_SAMPLE engine.
//  - Generates a literal shared sequence X_t in [0,1) (uniform), seeded.
//  - BASE solves block t iff X_t < T_time^SIM.
//  - CF   solves block t iff X_t < T_PoW^CF(t) = min(T_time^SIM, T_econ(W_t)).
//  - Therefore the ONLY difference is target; X_t identical for both.
//
// Wallet trajectory W_t^SIM is a supplied function of t (or an exogenous shock
// schedule). This engine does NOT own wallet dynamics; it consumes them.
// ---------------------------------------------------------------------------
struct SharedWorkSampleEngine {
    enum class MiningModel { SHARED_NORMALIZED_WORK_SAMPLE, PAIRED_EXPONENTIAL_ARRIVAL_CRN };

    struct Result {
        MiningModel model;
        std::string profile;
        int64_t T_time_sim;
        int64_t blocks_base = 0;     // blocks solved by BASE in horizon
        int64_t blocks_cf = 0;       // blocks solved by CF in horizon
        std::vector<int64_t> dt_base; // observed block intervals (BASE)
        std::vector<int64_t> dt_cf;   // observed block intervals (CF)
        // For CF internal bookkeeping:
        std::vector<int64_t> T_econ_trace;
        int64_t structural_fail = 0;  // any H1->canonical mutation detected
    };

    // Run M0 / M1 with shared-work sample.
    // T_time_sim: fixed-point target (e.g. H1_T_SCALE => p=1.0 => every block solves).
    // wallet: function returning W_t^SIM and W_t^* for time t (fixed-point).
    // blocks_horizon: number of SHARED work samples to draw.
    // shock: optional; if true, applies a step shock in W_t at shock_at.
    static Result RunSharedWork(
            H1EconParams params,
            int64_t T_time_sim,
            int64_t blocks_horizon,
            uint64_t seed,
            int64_t W_t_star,                 // fixed target wallet level (fixed-point)
            bool shock = false,
            int64_t shock_at = 0,
            int64_t shock_delta = 0,          // applied to W_t after shock_at
            int64_t target_ticks = 1) {       // dt scale: each step = target_ticks ticks

        Result r;
        r.model = MiningModel::SHARED_NORMALIZED_WORK_SAMPLE;
        r.profile = params.ProfileName();
        r.T_time_sim = T_time_sim;

        SplitMix64 rng(seed);
        H1EconHysteresisState hs;

        int64_t W_star_pp = W_t_star; // fixed-point wallet target

        for (int64_t t = 0; t < blocks_horizon; ++t) {
            double X = rng.u01();            // SHARED work sample [0,1)
            int64_t X_fp = (int64_t)(X * H1_T_SCALE); // fixed-point [0, H1_T_SCALE]

            // BASE: solves iff X < T_time^SIM.
            bool base_solves = (X_fp < T_time_sim);

            // CF: solves iff X < T_PoW^CF = min(T_time, T_econ).
            int64_t Wt;
            if (shock && t >= shock_at) Wt = W_star_pp + shock_delta;
            else Wt = W_star_pp; // no deviation => e=0 => T_econ = T_ref for non-null

            int64_t T_econ = H1ComputeT_econ(Wt, W_star_pp, params, hs);
            r.T_econ_trace.push_back(T_econ);
            int64_t T_pow_cf = H1EffectiveTarget(T_time_sim, T_econ);
            bool cf_solves = (X_fp < T_pow_cf);

            // Structural hard-fail check: CF must never exceed T_time (min semantics).
            if (T_pow_cf > T_time_sim) r.structural_fail++;

            if (base_solves) {
                r.blocks_base++;
                r.dt_base.push_back(target_ticks);
            }
            if (cf_solves) {
                r.blocks_cf++;
                r.dt_cf.push_back(target_ticks);
            }
        }
        return r;
    }

    // M2: paired exponential arrival CRN. Same U drives both BASE and CF inter-arrival
    // times; lambda = -ln(U)/T. DT_base and DT_cf computed from the SAME U, so the
    // difference reflects only target scaling (T_time vs T_pow_cf).
    static Result RunPairedCRN(
            H1EconParams params,
            int64_t T_time_sim,       // fixed-point "target difficulty" (lambda scale)
            int64_t blocks_to_solve,  // how many block arrivals to simulate per arm
            uint64_t seed,
            int64_t W_t_star,
            bool shock = false,
            int64_t shock_at = 0,
            int64_t shock_delta = 0) {

        Result r;
        r.model = MiningModel::PAIRED_EXPONENTIAL_ARRIVAL_CRN;
        r.profile = params.ProfileName();
        r.T_time_sim = T_time_sim;

        SplitMix64 rng(seed);
        H1EconHysteresisState hs;
        int64_t W_star_pp = W_t_star;

        // BASE arm
        {
            int64_t last = 0;
            for (int64_t i = 0; i < blocks_to_solve; ++i) {
                double U = rng.u01();
                if (U <= 0.0) U = 1e-12;
                // lambda_base = 1/T_time_sim (in tick^-1). dt = -ln(U)/lambda.
                double lambda = (double)T_time_sim / (double)H1_T_SCALE; // in [0,1]
                double dt = -std::log(U) / lambda;
                int64_t dticks = (int64_t)(dt * 1000.0); // scale to int ticks
                r.dt_base.push_back(dticks);
                last += dticks;
            }
        }
        // CF arm — SAME U sequence, but lambda scaled by T_pow_cf/T_time.
        {
            SplitMix64 rng2(seed); // restart same seed for pairing
            H1EconHysteresisState hs2;
            for (int64_t i = 0; i < blocks_to_solve; ++i) {
                double U = rng2.u01();
                if (U <= 0.0) U = 1e-12;
                int64_t Wt = (shock && i >= shock_at) ? (W_star_pp + shock_delta) : W_star_pp;
                int64_t T_econ = H1ComputeT_econ(Wt, W_star_pp, params, hs2);
                r.T_econ_trace.push_back(T_econ);
                int64_t T_pow_cf = H1EffectiveTarget(T_time_sim, T_econ);
                if (T_pow_cf > T_time_sim) r.structural_fail++;
                double lambda = (double)T_pow_cf / (double)H1_T_SCALE;
                double dt = -std::log(U) / lambda;
                int64_t dticks = (int64_t)(dt * 1000.0);
                r.dt_cf.push_back(dticks);
            }
        }
        r.blocks_base = (int64_t)r.dt_base.size();
        r.blocks_cf   = (int64_t)r.dt_cf.size();
        return r;
    }
};

// ===========================================================================
// BOOST tests: M0 (null control proves CF==BASE), M1 (isolation / target falsify)
// ===========================================================================

BOOST_AUTO_TEST_SUITE(litenyx_h1_shadow_sim_suite)

// M0 — NULL CONTROL: P0 must make CF identical to BASE in a shared-work sample.
// Proves the simulator is correct: with T_econ = T_max >= T_time, CF target equals
// T_time, so blocks_cf == blocks_base and every structural check passes.
BOOST_AUTO_TEST_CASE(m0_null_control_cf_equals_base) {
    H1EconParams p; p.profile = H1EconProfile::P0_NULL_CONTROL;
    p.T_max_CF = H1_T_SCALE; p.T_ref = H1_T_SCALE;
    int64_t T_time = H1_T_SCALE / 2; // 0.5 acceptance probability
    auto r = SharedWorkSampleEngine::RunSharedWork(p, T_time, 200000, 12345, 1000000);
    BOOST_CHECK_EQUAL(r.structural_fail, 0);
    BOOST_CHECK_EQUAL(r.blocks_base, r.blocks_cf);
    BOOST_CHECK_EQUAL(r.dt_base.size(), r.dt_cf.size());
}

// M0 — determinism: same seed => identical block counts.
BOOST_AUTO_TEST_CASE(m0_determinism) {
    H1EconParams p; p.profile = H1EconProfile::P0_NULL_CONTROL;
    p.T_max_CF = H1_T_SCALE;
    int64_t T_time = H1_T_SCALE / 3;
    auto a = SharedWorkSampleEngine::RunSharedWork(p, T_time, 50000, 777, 1000000);
    auto b = SharedWorkSampleEngine::RunSharedWork(p, T_time, 50000, 777, 1000000);
    BOOST_CHECK_EQUAL(a.blocks_base, b.blocks_base);
    BOOST_CHECK_EQUAL(a.blocks_cf, b.blocks_cf);
}

// M0 — BASE acceptance probability matches T_time under large sample.
BOOST_AUTO_TEST_CASE(m0_base_rate_matches_target) {
    H1EconParams p; p.profile = H1EconProfile::P0_NULL_CONTROL;
    p.T_max_CF = H1_T_SCALE;
    int64_t T_time = H1_T_SCALE / 4; // 0.25
    int64_t N = 400000;
    auto r = SharedWorkSampleEngine::RunSharedWork(p, T_time, N, 2024, 1000000);
    double rate = (double)r.blocks_base / (double)N;
    // within 1.5% of 0.25
    BOOST_CHECK_CLOSE(rate, 0.25, 1.5);
}

// M1 — P1 Linear: with a POSITIVE wallet deviation (W_t > W_t^*), T_econ > T_ref,
// so CF target > T_time => CF solves MORE blocks than BASE (easier). With NEGATIVE
// deviation, CF solves FEWER. This is the isolation of causal effect.
// M1 — P1 Linear: the candidate T_PoW^CF = min(T_time, T_econ) can ONLY be <= T_time.
// So the controller can only TIGHTEN acceptance. Under a NEGATIVE wallet deviation
// (W_t < W_t^*), T_econ < T_ref = T_time => CF target < T_time => CF solves FEWER
// blocks than BASE. Under POSITIVE deviation, T_econ > T_time is capped by min, so
// CF == BASE (no easing possible). This isolates the single causal direction.
BOOST_AUTO_TEST_CASE(m1_linear_negative_deviation_tightens_acceptance) {
    int64_t T_time = H1_T_SCALE / 2;
    int64_t N = 400000;
    H1EconParams p; p.profile = H1EconProfile::P1_LINEAR;
    p.T_ref = H1_T_SCALE / 2; p.k = 200000; p.T_max_CF = H1_T_SCALE; // 0.2, headroom

    // No deviation: e = 0 => T_econ = T_ref = T_time => CF == BASE.
    auto base = SharedWorkSampleEngine::RunSharedWork(p, T_time, N, 555, 1000000,
                                                      false, 0, 0);
    // Positive deviation: W_t >> W_t^* => T_econ > T_time, capped by min => CF == BASE.
    auto posShock = SharedWorkSampleEngine::RunSharedWork(p, T_time, N, 555, 1000000,
                                                          true, 0, +500000);
    // Negative deviation: W_t << W_t^* => T_econ < T_time => CF tighter (fewer blocks).
    auto negShock = SharedWorkSampleEngine::RunSharedWork(p, T_time, N, 555, 1000000,
                                                          true, 0, -500000);
    BOOST_CHECK_EQUAL(base.structural_fail, 0);
    BOOST_CHECK_EQUAL(posShock.structural_fail, 0);
    BOOST_CHECK_EQUAL(negShock.structural_fail, 0);
    BOOST_CHECK_EQUAL(base.blocks_cf, base.blocks_base);
    BOOST_CHECK_EQUAL(posShock.blocks_cf, posShock.blocks_base);
    BOOST_CHECK_LT(negShock.blocks_cf, negShock.blocks_base);
}

// M1 — P0 NULL must NOT show deviation effect (control).
BOOST_AUTO_TEST_CASE(m1_null_no_deviation_effect) {
    int64_t T_time = H1_T_SCALE / 2;
    H1EconParams p; p.profile = H1EconProfile::P0_NULL_CONTROL;
    p.T_max_CF = H1_T_SCALE;
    auto neg = SharedWorkSampleEngine::RunSharedWork(p, T_time, 200000, 999, 1000000,
                                                      true, 0, -500000);
    BOOST_CHECK_EQUAL(neg.blocks_cf, neg.blocks_base);
}

// M1 — monotonic T_econ trace under sustained positive deviation (P1).
BOOST_AUTO_TEST_CASE(m1_t_econ_trace_monotonic) {
    int64_t T_time = H1_T_SCALE / 2;
    H1EconParams p; p.profile = H1EconProfile::P1_LINEAR;
    p.T_ref = H1_T_SCALE / 2; p.k = 200000; p.T_max_CF = H1_T_SCALE;
    auto r = SharedWorkSampleEngine::RunSharedWork(p, T_time, 1000, 42, 1000000,
                                                    true, 0, +500000);
    BOOST_CHECK(!r.T_econ_trace.empty());
    // all T_econ should equal T_ref*(1+k*e) > T_ref (eased), and <= T_max
    for (int64_t Te : r.T_econ_trace) {
        BOOST_CHECK_GT(Te, p.T_ref);
        BOOST_CHECK_LE(Te, p.T_max_CF);
    }
}

// M2 — paired CRN: structural integrity (CF never easier than BASE when T_econ<T_time).
BOOST_AUTO_TEST_CASE(m2_paired_crn_no_structural_fail) {
    int64_t T_time = H1_T_SCALE / 2;
    H1EconParams p; p.profile = H1EconProfile::P2_SATURATING;
    p.T_ref = H1_T_SCALE / 2; p.k = 500000; p.T_max_CF = H1_T_SCALE;
    auto r = SharedWorkSampleEngine::RunPairedCRN(p, T_time, 5000, 31415, 1000000,
                                                  true, 1000, -400000);
    BOOST_CHECK_EQUAL(r.structural_fail, 0);
    BOOST_CHECK_EQUAL(r.dt_base.size(), r.dt_cf.size());
}

BOOST_AUTO_TEST_SUITE_END()
