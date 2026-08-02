// Litenyx H1-SHADOW-ENG — Experimental Economic-Target Interface (H1 ONLY).
//
// This header defines a DETERMINISTIC, PARAMETERIZED H1-only interface for the
// counterfactual economic target T_econ(W_t). It is NOT consensus. It is NOT a
// production difficulty controller. No mapping here is canonical or a production
// recommendation.
//
// All arithmetic is bounded fixed-point / integer. NO floating-point consensus
// semantics. The mathematical descriptions in the plan (Phi(e;k) etc.) are
// realized here with explicit fixed-point scaling and documented rounding.
//
// Locked invariants (preserved by construction):
//   W_t  -/-> N_h      G_c -/-> N_h
//   W_t  -/-> T_time^PROD   W_t -/-> T_PoW^PROD
// This module receives W_t / W_t^* only as SIMULATION INPUTS (int64). It has no
// handle to topology state, no transition function, no production PoW/target.
//
// Every parameter below is classified H1 EXPERIMENTAL PARAMETER.

#ifndef LITENYX_H1_POW_TARGET_H
#define LITENYX_H1_POW_TARGET_H

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>   // only for fabs-like integer abs; we use std::abs on int64
#include <cstdlib> // abs

#include <litenyx/LITENYX_types.h> // for LITENYX_MIN_CHAINS etc if needed

// Fixed-point scale for the normalized wallet deviation e_t^W.
// e_t^W = (W_t - W_t^*) / max(W_t^*, 1), represented at Q_W = 1e6 (parts per million
// of normalized deviation). Integer-only.
static const int64_t H1_QW = 1000000; // fixed-point scale for e_t^W

// The normalized target representation used by H1 simulation. T is a fixed-point
// "acceptance probability-ish" scalar in (0, H1_T_SCALE]. Higher T => easier (more
// work accepted). This is T^SIM, never T^PROD.
static const int64_t H1_T_SCALE = 1000000; // T represented in [0, 1e6] fixed-point

// Economic-target mapping profile selector (H1 EXPERIMENTAL; none canonical).
enum class H1EconProfile {
    P0_NULL_CONTROL = 0, // T_econ >= T_time always => T_PoW_CF = T_time (regression control)
    P1_LINEAR = 1,
    P2_SATURATING = 2,
    P3_HYSTERETIC = 3
};

// Experimental parameter bundle for T_econ. ALL fields are H1 EXPERIMENTAL
// PARAMETERS. None is frozen, locked, or a production recommendation.
struct H1EconParams {
    H1EconProfile profile = H1EconProfile::P0_NULL_CONTROL;
    int64_t T_ref = H1_T_SCALE;       // counterfactual reference target (fixed-point)
    int64_t T_min_CF = 1;             // lower clamp on T_econ (fixed-point)
    int64_t T_max_CF = H1_T_SCALE;    // upper clamp on T_econ (fixed-point)
    int64_t k = 0;                    // sensitivity (fixed-point, scaled by H1_QW)
    int64_t deadband = 0;             // hysteretic deadband on e_t^W (fixed-point)
    int64_t window = 1;               // observation window (blocks); reserved
    int64_t shock_magnitude = 0;      // reserved for shock scenarios (fixed-point)

    std::string ProfileName() const {
        switch (profile) {
            case H1EconProfile::P0_NULL_CONTROL: return "P0_NULL_CONTROL";
            case H1EconProfile::P1_LINEAR:       return "P1_LINEAR";
            case H1EconProfile::P2_SATURATING:   return "P2_SATURATING";
            case H1EconProfile::P3_HYSTERETIC:   return "P3_HYSTERETIC";
        }
        return "UNKNOWN";
    }
};

// Compute normalized wallet deviation e_t^W in fixed-point (Q_W).
// e_t^W = (W_t - W_t^*) / max(W_t^*, 1). W_t, W_t^* are SIMULATION INPUTS only.
inline int64_t H1WalletDeviation(int64_t W_t, int64_t W_t_star) {
    int64_t denom = (W_t_star > 0) ? W_t_star : 1; // max(W_t^*, 1)
    // (W_t - W_t^*) scaled by Q_W; integer multiplication then division.
    int64_t num = (W_t - W_t_star) * H1_QW;
    // Floor division toward zero is acceptable for a deviation metric; sign preserved.
    return num / denom;
}

// Hysteresis state retained across steps (P3 only). Internal to the engine.
struct H1EconHysteresisState {
    int64_t lastT_econ = H1_T_SCALE; // previously emitted T_econ (fixed-point)
    bool    initialized = false;
};

// Core mapping Phi(e; params) returning a multiplicative factor in fixed-point
// (scale H1_QW relative to 1.0; we represent factor*H1_QW). Then
// T_econ = Clamp(T_ref * factor / H1_QW, T_min_CF, T_max_CF).
// Returns factor in fixed-point (factor*H1_QW), i.e. 1.0 => H1_QW.
inline int64_t H1PhiFactor(int64_t e_fixed, const H1EconParams& p) {
    // e_fixed is e_t^W * H1_QW already.
    switch (p.profile) {
        case H1EconProfile::P1_LINEAR:
        case H1EconProfile::P3_HYSTERETIC: { // P3 uses P1-style sensitivity outside deadband
            // Phi = 1 + k*e, with k already scaled such that (k*e)/H1_QW is the increment.
            // Here p.k is the linear slope in fixed-point (k already at Q_W scale).
            int64_t inc = (p.k * e_fixed) / H1_QW; // (k * e) / H1_QW
            int64_t f = H1_QW + inc;
            return f;
        }
        case H1EconProfile::P2_SATURATING: {
            // Phi = 1 + k * e / (1 + |e|), e in fixed-point.
            int64_t ae = (e_fixed < 0) ? -e_fixed : e_fixed;
            int64_t denom = H1_QW + ae; // 1 + |e| in fixed-point
            int64_t num = p.k * e_fixed; // k * e (both fixed-point => /H1_QW after)
            int64_t frac = (denom != 0) ? (num / denom) : 0; // k*e/(1+|e|) fixed-point
            int64_t f = H1_QW + frac;
            return f;
        }
        case H1EconProfile::P0_NULL_CONTROL:
        default:
            return H1_QW; // factor 1.0; actual profile handled in T_econ computation
    }
}

// Compute T_econ(t) for a given wallet deviation, with hysteresis state for P3.
// T_econ = Clamp(T_ref * Phi(e;k) / H1_QW, T_min_CF, T_max_CF).
// For P0_NULL_CONTROL, returns T_max_CF (so T_PoW_CF = T_time when T_time<=T_max_CF).
// For P3, inside the deadband retains previous T_econ; outside, recomputes (use P1/P2
//   style recompute with k as the active sensitivity, deadband gating the update).
// All integer/fixed-point; rounding: truncating division (documented).
inline int64_t H1ComputeT_econ(int64_t W_t, int64_t W_t_star,
                               const H1EconParams& p,
                               H1EconHysteresisState& hs) {
    if (p.profile == H1EconProfile::P0_NULL_CONTROL) {
        hs.lastT_econ = p.T_max_CF;
        hs.initialized = true;
        return p.T_max_CF; // => T_PoW_CF = min(T_time, T_max_CF) = T_time (if T_time<=T_max)
    }

    int64_t e = H1WalletDeviation(W_t, W_t_star); // fixed-point

    if (p.profile == H1EconProfile::P3_HYSTERETIC) {
        // Inside deadband: retain previous T_econ state (no update).
        if (hs.initialized && (e > -p.deadband && e < p.deadband)) {
            return hs.lastT_econ; // deadband: hold previous counterfactual target
        }
        // Outside deadband: recompute using P1-style linear sensitivity (k active).
        int64_t f = H1PhiFactor(e, p); // uses P1 formula via k
        int64_t Te = (p.T_ref * f) / H1_QW;
        if (Te < p.T_min_CF) Te = p.T_min_CF;
        if (Te > p.T_max_CF) Te = p.T_max_CF;
        hs.lastT_econ = Te;
        hs.initialized = true;
        return Te;
    }

    // P1 / P2: standard recompute each step.
    int64_t f = H1PhiFactor(e, p);
    int64_t Te = (p.T_ref * f) / H1_QW;
    if (Te < p.T_min_CF) Te = p.T_min_CF;
    if (Te > p.T_max_CF) Te = p.T_max_CF;
    hs.lastT_econ = Te;
    hs.initialized = true;
    return Te;
}

// The candidate scalar effective target. T_PoW^CF = min(T_time^SIM, T_econ).
// T_time_SIM is a SIMULATION CONSTANT (modeled target), passed by value.
inline int64_t H1EffectiveTarget(int64_t T_time_sim, int64_t T_econ) {
    return (T_econ < T_time_sim) ? T_econ : T_time_sim;
}

#endif // LITENYX_H1_POW_TARGET_H
