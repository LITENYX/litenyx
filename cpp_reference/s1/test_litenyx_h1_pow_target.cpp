// Litenyx H1-SHADOW-ENG — Unit tests for H1 economic-target interface.
// Validates H1EconParams profiles (P0..P3), fixed-point mapping, clamping,
// hysteresis, and T_econ -> T_PoW^CF effective-target semantics.
//
// H1 ONLY. No consensus, no topology. Every parameter here is EXPERIMENTAL.

#define BOOST_TEST_MODULE LITENYX_h1_pow_target_test
#include <boost/test/included/unit_test.hpp>
#include <cstdint>
#include <cmath>

#include "litenyx_h1_pow_target.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(litenyx_h1_pow_target_suite)

BOOST_AUTO_TEST_CASE(p0_null_control_returns_tmax) {
    H1EconParams p; p.profile = H1EconProfile::P0_NULL_CONTROL;
    p.T_max_CF = H1_T_SCALE; p.T_min_CF = 1; p.T_ref = H1_T_SCALE;
    H1EconHysteresisState hs;
    int64_t Te = H1ComputeT_econ(500, 1000, p, hs);
    BOOST_CHECK_EQUAL(Te, H1_T_SCALE);
}

BOOST_AUTO_TEST_CASE(p0_effective_target_equals_ttime) {
    H1EconParams p; p.profile = H1EconProfile::P0_NULL_CONTROL;
    p.T_max_CF = H1_T_SCALE;
    H1EconHysteresisState hs;
    int64_t Te = H1ComputeT_econ(0, 0, p, hs);
    int64_t T_time_sim = H1_T_SCALE / 2;
    int64_t T_pow_cf = H1EffectiveTarget(T_time_sim, Te);
    // T_econ = T_max >= T_time => effective = T_time
    BOOST_CHECK_EQUAL(T_pow_cf, T_time_sim);
}

BOOST_AUTO_TEST_CASE(p1_linear_positive_deviation_lowers_target) {
    H1EconParams p; p.profile = H1EconProfile::P1_LINEAR;
    p.T_ref = H1_T_SCALE / 2; p.k = 200000; // k = 0.2 in fixed-point; headroom to ease
    p.T_max_CF = H1_T_SCALE;
    H1EconHysteresisState hs;
    // W_t > W_t^* => e > 0 => Phi>1 => T_econ > T_ref (eased, easier)
    int64_t Te_high = H1ComputeT_econ(1200, 1000, p, hs);
    // W_t < W_t^* => e < 0 => Phi<1 => T_econ < T_ref (tightened)
    int64_t Te_low  = H1ComputeT_econ(800, 1000, p, hs);
    BOOST_CHECK_GT(Te_high, p.T_ref);
    BOOST_CHECK_LT(Te_low, p.T_ref);
    BOOST_CHECK_LE(Te_high, p.T_max_CF);
}

BOOST_AUTO_TEST_CASE(p1_antisymmetry_about_zero) {
    H1EconParams p; p.profile = H1EconProfile::P1_LINEAR;
    p.T_ref = H1_T_SCALE / 2; p.k = 200000;
    H1EconHysteresisState hs1, hs2;
    int64_t Te_up = H1ComputeT_econ(1100, 1000, p, hs1);
    int64_t Te_dn = H1ComputeT_econ(900, 1000, p, hs2);
    // Because Phi = 1 + k*e, symmetric deviations invert around T_ref.
    int64_t up_delta = Te_up - p.T_ref;
    int64_t dn_delta = p.T_ref - Te_dn;
    BOOST_CHECK_EQUAL(up_delta, dn_delta);
}

BOOST_AUTO_TEST_CASE(p2_saturating_bounded) {
    H1EconParams p; p.profile = H1EconProfile::P2_SATURATING;
    p.T_ref = H1_T_SCALE / 2; p.k = 500000; // k = 0.5; headroom to ease
    p.T_max_CF = H1_T_SCALE;
    H1EconHysteresisState hs;
    int64_t Te_small = H1ComputeT_econ(1010, 1000, p, hs);
    int64_t Te_large = H1ComputeT_econ(100000, 1000, p, hs);
    int64_t Te_huge  = H1ComputeT_econ(100000000, 1000, p, hs);
    // Saturating: large deviations produce smaller increments than small ones.
    BOOST_CHECK_GT(Te_large, p.T_ref);
    BOOST_CHECK_GT(Te_huge, Te_large);
    BOOST_CHECK_LT(Te_huge - Te_large, Te_large - Te_small);
}

BOOST_AUTO_TEST_CASE(clamp_bounds_respected) {
    H1EconParams p; p.profile = H1EconProfile::P1_LINEAR;
    p.T_ref = H1_T_SCALE; p.k = 9000000; // huge slope
    p.T_min_CF = H1_T_SCALE / 4; p.T_max_CF = H1_T_SCALE; // cap
    H1EconHysteresisState hs;
    int64_t Te = H1ComputeT_econ(0, 1000, p, hs); // e negative, large magnitude
    BOOST_CHECK_GE(Te, p.T_min_CF);
    BOOST_CHECK_LE(Te, p.T_max_CF);
}

BOOST_AUTO_TEST_CASE(p3_hysteresis_holds_state_in_deadband) {
    H1EconParams p; p.profile = H1EconProfile::P3_HYSTERETIC;
    p.T_ref = H1_T_SCALE / 2; p.k = 200000; p.deadband = 50000; // deadband in e-fixed
    p.T_max_CF = H1_T_SCALE;
    H1EconHysteresisState hs;
    // First observation outside deadband sets a target.
    int64_t Te0 = H1ComputeT_econ(1200, 1000, p, hs); // e = +0.2*1e6 = 200000 > deadband
    // Tiny deviation inside deadband should retain previous T_econ.
    int64_t Te1 = H1ComputeT_econ(1001, 1000, p, hs); // e = 1000 small
    BOOST_CHECK_EQUAL(Te1, Te0);
    // Large opposite deviation should update.
    int64_t Te2 = H1ComputeT_econ(800, 1000, p, hs); // e = -0.2
    BOOST_CHECK_NE(Te2, Te0);
}

BOOST_AUTO_TEST_CASE(wallet_deviation_fixed_point) {
    // e = (W - W*)/max(W*,1) * H1_QW
    int64_t e = H1WalletDeviation(1100, 1000);
    BOOST_CHECK_EQUAL(e, 100000); // 0.1 * 1e6
    int64_t e2 = H1WalletDeviation(900, 1000);
    BOOST_CHECK_EQUAL(e2, -100000);
    // denom floors at 1 to avoid div-by-zero
    int64_t e3 = H1WalletDeviation(5, 0);
    BOOST_CHECK_EQUAL(e3, 5 * H1_QW);
}

BOOST_AUTO_TEST_CASE(effective_target_is_min) {
    BOOST_CHECK_EQUAL(H1EffectiveTarget(100, 50), 50);
    BOOST_CHECK_EQUAL(H1EffectiveTarget(50, 100), 50);
    BOOST_CHECK_EQUAL(H1EffectiveTarget(70, 70), 70);
}

BOOST_AUTO_TEST_SUITE_END()
