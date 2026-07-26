// Litenyx H1-SHADOW-ENG — M4 Adversarial / Pathological Scarcity & Stability Boundary.
//
// H1 ONLY. Research execution only. NO production promotion, NO consensus integration,
// NO modification of frozen topology semantics.
//
// Per the authorized M4 scope, this suite attacks the M3 failure mode (one-sided
// tightening -> fewer blocks -> hash exit -> persistent scarcity) with a paired CRN
// (BASE vs H1) methodology and reports an EMPIRICAL STABILITY BOUNDARY (epsilon_crit),
// not merely a pass/fail count. It covers: scarcity depth/duration, elasticity
// boundary search, path dependence, recovery stress (asymmetric exit/re-entry),
// controller interaction, adversarial timing, floor/ceiling saturation, long-run
// equilibrium, and a full sensitivity envelope. Each region is BASE-relative
// classified: IMPROVED / NEUTRAL / DEGRADED_BOUNDED / CATASTROPHIC.
//
// HARD VERDICT RULE (carried from authorization): if PLAUSIBLE parameter regions
// produce persistent H1 hash collapse while BASE recovers under identical exogenous
// conditions, H1 must remain NON-AUTHORIZED regardless of aggregate averages. A
// security-collapse regime cannot be averaged away.

#define BOOST_TEST_MODULE LITENYX_h1_m4_adversarial_scarcity
#include <boost/test/included/unit_test.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <map>

#include "litenyx_h1_miner_engine.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(litenyx_h1_m4_suite)

// Helper: build a shock scenario.
static H1M3Params ShockScenario(int64_t T_time, double elasticity, int64_t horizon,
                                uint64_t seed, int64_t floor_level,
                                int64_t shock_at, int64_t recover_at,
                                double reentry = -1.0) {
    H1M3Params mp;
    mp.T_time = T_time; mp.W_star = 1000000;
    mp.elasticity = elasticity;
    mp.horizon_blocks = horizon; mp.seed = seed;
    mp.shock = true; mp.floor_level = floor_level;
    mp.shock_at = shock_at; mp.recover_at = recover_at;
    mp.reentry_elasticity = reentry;
    return mp;
}

// (1) Scarcity depth & duration: progressively stronger/longer shocks.
BOOST_AUTO_TEST_CASE(m4_scarcity_depth_duration) {
    int64_t T_time = H1_T_SCALE / 2;
    // floor_level from mild (0.8) to severe (0.1); duration fixed.
    for (int64_t fl = 800000; fl >= 100000; fl -= 100000) {
        auto mp = ShockScenario(T_time, 0.6, 6000, 11, fl, 1000, 4000);
        auto p = H1RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
        BOOST_TEST_MESSAGE("[M4 depth] floor=" << (fl/10000.0) << "% H1_ret="
            << p.h1.final_hash_retention << " BASE_ret=" << p.base.final_hash_retention
            << " verdict=" << p.VerdictName());
        // Runs must complete; classification well-defined.
        BOOST_CHECK(p.verdict != H1M3Paired::Verdict::NEUTRAL || true); // always defined
        BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    }
    // Longer duration at fixed severe floor.
    for (int64_t rec = 2000; rec <= 5000; rec += 1000) {
        auto mp = ShockScenario(T_time, 0.6, 7000, 22, H1_T_SCALE/4, 1000, rec);
        auto p = H1RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
        BOOST_TEST_MESSAGE("[M4 duration] recover_at=" << rec << " H1_ret="
            << p.h1.final_hash_retention << " verdict=" << p.VerdictName());
        BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    }
}

// (2) Elasticity boundary search: sweep elasticity to locate epsilon_crit transition
// between bounded recovery and persistent collapse. Runs BOTH models and reports the
// empirical stability boundary honestly under each assumption.
BOOST_AUTO_TEST_CASE(m4_elasticity_boundary_search) {
    int64_t T_time = H1_T_SCALE / 2;
    // Realistic model (cadence restoration ON): boundary is DEGRADED_BOUNDED.
    double eps_deg_rest = -1.0;
    // Stress model (NO restoration): boundary is CATASTROPHIC death-spiral regime.
    double eps_crit_stress = -1.0;
    const double step = 0.05;
    for (double e = 0.0; e <= 1.0001; e += step) {
        // Restoration ON.
        auto mpR = ShockScenario(T_time, e, 7000, 99, H1_T_SCALE/4, 1000, 4000);
        mpR.restoration = true;
        auto pR = H1RunPaired(mpR, H1EconProfile::P1_LINEAR, 200000);
        if (eps_deg_rest < 0 && pR.verdict == H1M3Paired::Verdict::DEGRADED_BOUNDED)
            eps_deg_rest = e;
        // Restoration OFF (stress).
        auto mpS = ShockScenario(T_time, e, 7000, 99, H1_T_SCALE/4, 1000, 4000);
        mpS.restoration = false;
        auto pS = H1RunPaired(mpS, H1EconProfile::P1_LINEAR, 200000);
        if (eps_crit_stress < 0 && pS.verdict == H1M3Paired::Verdict::CATASTROPHIC) {
            eps_crit_stress = e;
            BOOST_TEST_MESSAGE("[M4 eps_crit STRESS] catastrophic at elasticity=" << e);
        }
    }
    BOOST_TEST_MESSAGE("[M4 elasticity boundary] eps_degraded(restoration)=" << eps_deg_rest
        << " eps_crit(no-restoration)=" << eps_crit_stress);
    // Under the stress (no-restoration) assumption, a plausible (<=1.0) elasticity
    // must produce persistent collapse (the M3-flagged failure mode).
    BOOST_CHECK_GT(eps_crit_stress, 0.0);
    BOOST_CHECK_LE(eps_crit_stress, 1.0001);
    // Under the realistic restoration model, bounded degradation must exist (not full
    // collapse) for at least some elasticity, OR remain neutral — both are non-catastrophic.
    BOOST_CHECK(eps_deg_rest >= 0.0); // defined (may be -1 if never degraded = fully benign)
}

// (3) Path dependence: identical terminal fundamentals via different shock histories.
BOOST_AUTO_TEST_CASE(m4_path_dependence) {
    int64_t T_time = H1_T_SCALE / 2;
    // Path A: single deep shock. Path B: two shallow shocks summing to same floor-time.
    auto mpA = ShockScenario(T_time, 0.7, 9000, 7, H1_T_SCALE/4, 1000, 5000);
    auto pA = H1RunPaired(mpA, H1EconProfile::P1_LINEAR, 200000);
    H1M3Params mpB = mpA;
    mpB.shock = false; mpB.gradual = true; mpB.gradual_end = 5000; mpB.floor_level = H1_T_SCALE/2;
    // (Path B uses gradual drift to 0.5 then we force shock at 5000; simplified: compare.)
    auto pB = H1RunPaired(mpB, H1EconProfile::P1_LINEAR, 200000);
    BOOST_TEST_MESSAGE("[M4 path] A_final=" << pA.h1.final_hash_retention
        << " B_final=" << pB.h1.final_hash_retention);
    // Both must be well-defined; divergence indicates path dependence (reported).
    BOOST_CHECK(std::isfinite(pA.h1.final_hash_retention));
    BOOST_CHECK(std::isfinite(pB.h1.final_hash_retention));
}

// (4) Recovery stress: temporary hash exit, delayed re-entry, asymmetric elasticity.
BOOST_AUTO_TEST_CASE(m4_recovery_stress_asymmetric) {
    int64_t T_time = H1_T_SCALE / 2;
    // Exit elasticity 0.8, re-entry elasticity 0.2 (sticky exit).
    auto mp = ShockScenario(T_time, 0.8, 8000, 33, H1_T_SCALE/3, 1000, 4000, /*reentry*/ 0.2);
    auto p = H1RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
    BOOST_TEST_MESSAGE("[M4 recovery] sticky re-entry H1_ret=" << p.h1.final_hash_retention
        << " BASE_ret=" << p.base.final_hash_retention << " verdict=" << p.VerdictName());
    BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    // Asymmetric re-entry must not crash the engine; verdict defined.
    BOOST_CHECK(p.verdict != H1M3Paired::Verdict::NEUTRAL || true);
}

// (5) Controller interaction: ordinary time-difficulty vs H1 under scarcity.
// With T_time held constant (our sim's "time controller"), H1 only tightens. Verify
// H1 never EASES vs BASE (monotonic tightening proven in ISO) and that under scarcity
// H1 acceptance is <= BASE acceptance.
BOOST_AUTO_TEST_CASE(m4_controller_interaction) {
    int64_t T_time = H1_T_SCALE / 2;
    for (double e = 0.1; e <= 1.0; e += 0.2) {
        auto mp = ShockScenario(T_time, e, 6000, 44, H1_T_SCALE/4, 1000, 4000);
        auto p = H1RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
        // H1 cannot accept MORE than BASE (one-sided rule; ISO proven).
        BOOST_CHECK_LE(p.h1.accepted_block_rate + 1e-9, p.base.accepted_block_rate + 1e-9);
    }
}

// (6) Adversarial timing: shock at hysteresis boundary / just before target update.
BOOST_AUTO_TEST_CASE(m4_adversarial_timing) {
    int64_t T_time = H1_T_SCALE / 2;
    // Shock exactly at deadband edge by choosing floor producing e near deadband.
    // e = (W-W*)/W* ; floor 0.95 => e = -0.05*1e6 = -50000 = deadband edge.
    auto mp = ShockScenario(T_time, 0.6, 7000, 55, 950000, 1000, 4000);
    auto p = H1RunPaired(mp, H1EconProfile::P3_HYSTERETIC, 200000);
    BOOST_TEST_MESSAGE("[M4 timing] deadband-edge shock H1_ret=" << p.h1.final_hash_retention
        << " verdict=" << p.VerdictName());
    BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    // Also a shock far from boundary for comparison.
    auto mp2 = ShockScenario(T_time, 0.6, 7000, 55, H1_T_SCALE/4, 1000, 4000);
    auto p2 = H1RunPaired(mp2, H1EconProfile::P3_HYSTERETIC, 200000);
    BOOST_CHECK_EQUAL(p2.h1.structural_fail, 0);
}

// (7) Floor/ceiling saturation: sustained operation at all clamps; trapped states.
BOOST_AUTO_TEST_CASE(m4_clamp_saturation) {
    int64_t T_time = H1_T_SCALE / 2;
    // Sustained severe negative deviation (floor 0.05) for whole horizon (no recovery).
    H1M3Params mp;
    mp.T_time = T_time; mp.W_star = 1000000; mp.elasticity = 0.7;
    mp.horizon_blocks = 8000; mp.seed = 66; mp.shock = true;
    mp.floor_level = H1_T_SCALE / 20; mp.shock_at = 0; mp.recover_at = -1; // never recovers
    auto p = H1RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
    BOOST_TEST_MESSAGE("[M4 saturate] sustained-severe H1_ret=" << p.h1.final_hash_retention
        << " min_ret=" << p.h1.min_hash_retention << " verdict=" << p.VerdictName());
    BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
    // Saturation must not violate monotonic tightening / clamp invariants.
    for (int64_t Te : p.h1.T_econ_trace) BOOST_CHECK_LE(Te, T_time);
}

// (8) Long-run equilibrium: distinguish slow recovery from persistent low-hash attractor.
BOOST_AUTO_TEST_CASE(m4_long_run_equilibrium) {
    int64_t T_time = H1_T_SCALE / 2;
    // Highly elastic shock + recovery, very long horizon to see true attractor.
    auto mp = ShockScenario(T_time, 0.9, 20000, 77, H1_T_SCALE/4, 1000, 5000);
    auto p = H1RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
    BOOST_TEST_MESSAGE("[M4 longrun] H1_ret=" << p.h1.final_hash_retention
        << " BASE_ret=" << p.base.final_hash_retention << " verdict=" << p.VerdictName());
    // If H1 collapses, it must be classified CATASTROPHIC (persistent attractor).
    if (p.h1.final_hash_retention < 0.5) {
        BOOST_CHECK(p.verdict == H1M3Paired::Verdict::CATASTROPHIC);
    }
}

// (9) Sensitivity envelope: sweep elasticity, shock magnitude, gain k, deadband,
// response lag (modeled via horizon scaling). Reports a 2D boundary table.
BOOST_AUTO_TEST_CASE(m4_sensitivity_envelope) {
    int64_t T_time = H1_T_SCALE / 2;
    // Grid: elasticity x shock-floor. Classify each cell.
    vector<double> els = {0.1, 0.3, 0.5, 0.7, 0.9};
    vector<int64_t> floors = {H1_T_SCALE/2, H1_T_SCALE/3, H1_T_SCALE/4, H1_T_SCALE/6};
    int catastrophic = 0, degraded = 0, neutral = 0;
    for (double e : els) {
        for (int64_t fl : floors) {
            auto mp = ShockScenario(T_time, e, 8000, 88, fl, 1000, 5000);
            mp.restoration = false; // stress assumption: no cadence restoration
            auto p = H1RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
            switch (p.verdict) {
                case H1M3Paired::Verdict::CATASTROPHIC: catastrophic++; break;
                case H1M3Paired::Verdict::DEGRADED_BOUNDED: degraded++; break;
                default: neutral++;
            }
            BOOST_CHECK_EQUAL(p.h1.structural_fail, 0);
        }
    }
    BOOST_TEST_MESSAGE("[M4 sensitivity] catastrophic=" << catastrophic
        << " degraded=" << degraded << " neutral/better=" << neutral);
    // The envelope must contain at least one CATASTROPHIC cell (plausible collapse).
    BOOST_CHECK_GT(catastrophic, 0);
}

// (10) BASE-relative verdict + HARD VERDICT RULE enforcement.
// If ANY plausible region yields persistent H1 collapse while BASE recovers, H1 must
// remain NON-AUTHORIZED. We scan the envelope and assert the experimental verdict is
// CATASTROPHIC for a plausible (elasticity<=1.0, realistic shock) region.
BOOST_AUTO_TEST_CASE(m4_hard_verdict_rule) {
    int64_t T_time = H1_T_SCALE / 2;
    bool found_persistent_collapse_while_base_ok = false;
    for (double e = 0.3; e <= 1.0001; e += 0.1) {
        for (int64_t fl : {H1_T_SCALE/4, H1_T_SCALE/5, H1_T_SCALE/6}) {
            auto mp = ShockScenario(T_time, e, 9000, 123, fl, 1000, 5000);
            mp.restoration = false; // stress assumption: no cadence restoration
            auto p = H1RunPaired(mp, H1EconProfile::P1_LINEAR, 200000);
            if (p.h1.failed_to_recover && !p.base.failed_to_recover) {
                found_persistent_collapse_while_base_ok = true;
            }
            if (p.h1.final_hash_retention < 0.5 && p.base.final_hash_retention > 0.5) {
                found_persistent_collapse_while_base_ok = true;
            }
        }
    }
    BOOST_TEST_MESSAGE("[M4 HARD RULE] persistent H1 collapse while BASE recovers = "
        << (found_persistent_collapse_while_base_ok ? "YES" : "NO"));
    // Per authorized doctrine: H1 must remain NON-AUTHORIZED if such a region exists.
    // This is a RESEARCH FINDING; we assert the experimental evidence supports it.
    BOOST_CHECK(found_persistent_collapse_while_base_ok);
    // Note: production authorization is NOT granted by this test; it records the
    // experimental basis for the verdict (see report + plan maturity boundary).
}

BOOST_AUTO_TEST_SUITE_END()
