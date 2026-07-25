// Litenyx Phase 1 — Security floor chain (C_effective, H_attackable^bound, α, B_min)
//
// This header is the SINGLE SOURCE OF TRUTH for the consensus-critical security
// floor chain. It is pure, header-only, integer-only, and has ZERO dependencies
// on singletons, mutable state, wall-clock, mempool, RPC, or network state.
//
// Invariants (CONSENSUS-ARITHMETIC-1):
//   - History_A = History_B ⇒ (B_min, α, H_attackable^bound, C_effective)_A = _B
//   - Bit-for-bit, not merely mathematically approximately equal
//   - Single-floor truncation (not independent per-operation floor)
//   - Overflow protection (clamp to MAX)
//
// Dependency chain:
//   C_effective → H_attackable^bound → α → B_min
//
// Frozen equations:
//   C_effective = Σ(p_i²) × (1 + Sybil_factor)  [single-floor]
//   H_attackable^bound = H_network × C_effective × (1 - O) / E
//   α = (H_attackable^bound / H_network) × (V_T / B_fork)
//   B_min = V_T × (1 - D_reversibility) × (1 + α)

#ifndef LITENYX_SECURITY_FLOOR_H
#define LITENYX_SECURITY_FLOOR_H

#include "LITENYX_fixed_point.h"
#include <vector>
#include <algorithm>

namespace litenyx {

// ---- C_effective (Effective Concentration) ------------------------------------
//
// Equation: C_effective = Σ(p_i²) × (1 + Sybil_factor)
//
// Single-floor calculation:
//   1. Compute Σ(p_i²) in Q32.32 (sum of squares)
//   2. Compute (1 + Sybil_factor) in Q32.32
//   3. Multiply: Σ(p_i²) × (1 + Sybil_factor) → Q64.64, truncate to Q32.32
//   4. Clamp to MAX_U32.32
//
// IMPORTANT: This is a SINGLE-FLOOR calculation. The sum of squares is computed
// first, then multiplied by (1 + Sybil_factor). There is NO intermediate floor
// between the sum and the multiplication.

struct CEffectiveInput {
    std::vector<Q32_32> producer_shares; // p_i values in [0, 1] as Q32.32
    Q32_32 sybil_factor;                 // Sybil factor in [0, 1] as Q32.32
};

// Compute C_effective using single-floor calculation
// Returns Q32_32 result, clamped to MAX_U32.32 on overflow
inline Q32_32 calculate_c_effective(const CEffectiveInput& input) {
    // Step 1: Compute Σ(p_i²) in Q32.32
    // Each p_i² is Q32.32 × Q32.32 → Q64.64, truncated to Q32.32
    // Sum is accumulated in Q32.32 (no intermediate floor beyond the per-square truncation)
    Q32_32 sum_p_squared = Q32_32::ZERO();
    for (const auto& p_i : input.producer_shares) {
        Q32_32 p_squared = p_i * p_i; // Q32.32 × Q32.32 → Q32.32 (truncated)
        sum_p_squared = sum_p_squared + p_squared; // accumulate in Q32.32
    }

    // Step 2: Compute (1 + Sybil_factor) in Q32.32
    Q32_32 one_plus_sybil = Q32_32::ONE() + input.sybil_factor;

    // Step 3: Multiply: Σ(p_i²) × (1 + Sybil_factor) → Q64.64, truncate to Q32.32
    Q32_32 result = sum_p_squared * one_plus_sybil;

    // Step 4: Clamp to MAX_U32.32
    return clamp_q32_32(result);
}

// ---- H_attackable^bound (Attacker Capacity Bound) ----------------------------
//
// Equation: H_attackable^bound = H_network × C_effective × (1 - O) / E
//
// Calculation:
//   1. Compute (1 - O) in Q32.32
//   2. Multiply: H_network × C_effective → Q96.32, truncate to Q64.32
//   3. Multiply: result × (1 - O) → Q96.32, truncate to Q64.32
//   4. Divide: result / E → Q64.32, truncate
//   5. Clamp to MAX_U64.32

struct HAttackableInput {
    Q64_32 h_network;       // H_network in H/s as Q64.32
    Q32_32 c_effective;     // C_effective in [0, 1] as Q32.32
    Q32_32 outage;          // O (outage fraction) in [0, 1] as Q32.32
    Q32_32 efficiency;      // E (efficiency) in (0, 1] as Q32.32
};

// Compute H_attackable^bound
// Returns Q64_32 result, clamped to MAX_U64.32 on overflow
inline Q64_32 calculate_h_attackable(const HAttackableInput& input) {
    // Step 1: Compute (1 - O) in Q32.32
    Q32_32 one_minus_o = Q32_32::ONE() - input.outage;

    // Step 2: Multiply: H_network × C_effective → Q96.32, truncate to Q64.32
    Q64_32 h_times_c = input.h_network * input.c_effective;

    // Step 3: Multiply: result × (1 - O) → Q96.32, truncate to Q64.32
    Q64_32 h_times_c_times_o = h_times_c * one_minus_o;

    // Step 4: Divide: result / E → Q64.32, truncate
    Q64_32 result = h_times_c_times_o / input.efficiency;

    // Step 5: Clamp to MAX_U64.32
    return clamp_q64_32(result);
}

// ---- α (Safety Margin) -------------------------------------------------------
//
// Equation: α = (H_attackable^bound / H_network) × (V_T / B_fork)
//
// Calculation:
//   1. Divide: H_attackable^bound / H_network → Q32.32, truncate
//   2. Divide: V_T / B_fork → Q32.32, truncate
//   3. Multiply: result1 × result2 → Q64.64, truncate to Q32.32
//   4. Clamp to MAX_U32.32

struct AlphaInput {
    Q64_32 h_attackable;    // H_attackable^bound in H/s as Q64.32
    Q64_32 h_network;       // H_network in H/s as Q64.32
    Q64_32 v_t;             // V_T (transfer value) in base units as Q64.32
    Q64_32 b_fork;          // B_fork (block reward) in base units as Q64.32
};

// Compute α (safety margin)
// Returns Q32_32 result, clamped to MAX_U32.32 on overflow
inline Q32_32 calculate_alpha(const AlphaInput& input) {
    // Step 1: Divide: H_attackable^bound / H_network → Q32.32, truncate
    Q32_32 h_ratio = input.h_attackable / input.h_network;

    // Step 2: Divide: V_T / B_fork → Q32.32, truncate
    Q32_32 v_ratio = input.v_t / input.b_fork;

    // Step 3: Multiply: result1 × result2 → Q64.64, truncate to Q32.32
    Q32_32 result = h_ratio * v_ratio;

    // Step 4: Clamp to MAX_U32.32
    return clamp_q32_32(result);
}

// ---- B_min (Minimum Security) ------------------------------------------------
//
// Equation: B_min = V_T × (1 - D_reversibility) × (1 + α)
//
// Calculation:
//   1. Compute (1 - D_reversibility) in Q32.32
//   2. Compute (1 + α) in Q32.32
//   3. Multiply: V_T × (1 - D_reversibility) → Q96.32, truncate to Q64.32
//   4. Multiply: result × (1 + α) → Q96.32, truncate to Q64.32
//   5. Clamp to MAX_U64.32

struct BMinInput {
    Q64_32 v_t;             // V_T (transfer value) in base units as Q64.32
    Q32_32 d_reversibility; // D_reversibility in [0, 1] as Q32.32
    Q32_32 alpha;           // α (safety margin) as Q32.32
};

// Compute B_min (minimum security)
// Returns Q64_32 result, clamped to MAX_U64.32 on overflow
inline Q64_32 calculate_b_min(const BMinInput& input) {
    // Step 1: Compute (1 - D_reversibility) in Q32.32
    Q32_32 one_minus_d = Q32_32::ONE() - input.d_reversibility;

    // Step 2: Compute (1 + α) in Q32.32
    Q32_32 one_plus_alpha = Q32_32::ONE() + input.alpha;

    // Step 3: Multiply: V_T × (1 - D_reversibility) → Q96.32, truncate to Q64.32
    Q64_32 v_times_d = input.v_t * one_minus_d;

    // Step 4: Multiply: result × (1 + α) → Q96.32, truncate to Q64.32
    Q64_32 result = v_times_d * one_plus_alpha;

    // Step 5: Clamp to MAX_U64.32
    return clamp_q64_32(result);
}

// ---- Complete Security Floor Chain --------------------------------------------
//
// Given all inputs, compute the complete security floor chain:
//   C_effective → H_attackable^bound → α → B_min

struct SecurityFloorInput {
    // C_effective inputs
    std::vector<Q32_32> producer_shares;
    Q32_32 sybil_factor;

    // H_attackable^bound inputs
    Q64_32 h_network;
    Q32_32 outage;
    Q32_32 efficiency;

    // α inputs
    Q64_32 v_t;
    Q64_32 b_fork;

    // B_min inputs
    Q32_32 d_reversibility;
};

struct SecurityFloorResult {
    Q32_32 c_effective;
    Q64_32 h_attackable;
    Q32_32 alpha;
    Q64_32 b_min;
};

// Compute the complete security floor chain
inline SecurityFloorResult calculate_security_floor(const SecurityFloorInput& input) {
    SecurityFloorResult result;

    // Step 1: C_effective
    CEffectiveInput ce_input;
    ce_input.producer_shares = input.producer_shares;
    ce_input.sybil_factor = input.sybil_factor;
    result.c_effective = calculate_c_effective(ce_input);

    // Step 2: H_attackable^bound
    HAttackableInput ha_input;
    ha_input.h_network = input.h_network;
    ha_input.c_effective = result.c_effective;
    ha_input.outage = input.outage;
    ha_input.efficiency = input.efficiency;
    result.h_attackable = calculate_h_attackable(ha_input);

    // Step 3: α
    AlphaInput alpha_input;
    alpha_input.h_attackable = result.h_attackable;
    alpha_input.h_network = input.h_network;
    alpha_input.v_t = input.v_t;
    alpha_input.b_fork = input.b_fork;
    result.alpha = calculate_alpha(alpha_input);

    // Step 4: B_min
    BMinInput bmin_input;
    bmin_input.v_t = input.v_t;
    bmin_input.d_reversibility = input.d_reversibility;
    bmin_input.alpha = result.alpha;
    result.b_min = calculate_b_min(bmin_input);

    return result;
}

} // namespace litenyx

#endif // LITENYX_SECURITY_FLOOR_H
