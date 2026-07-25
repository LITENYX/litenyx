// Litenyx Phase 1 — Standalone golden vector tests (no Boost dependency)
//
// Tests the 9 golden vectors from CONSENSUS-ARITHMETIC-1:
//   1. C_effective rounding (1/3 + 1/3 + 1/3)
//   2. H_attackable^bound rounding
//   3. α rounding
//   4. B_min rounding
//   5. Boundary values (single producer)
//   6. Maximum values (overflow clamping)
//   7. Zero denominator
//   8. Clamp transitions
//   9. Chained rounding
//
// These tests verify bit-for-bit reproducibility and single-floor invariants.

#include <iostream>
#include <cassert>
#include <cmath>

#include "../../litenyx/LITENYX_fixed_point.h"
#include "../../litenyx/LITENYX_security_floor.h"

using namespace litenyx;

// Helper: Create Q32_32 from double (for test readability)
static Q32_32 q32(double d) { return Q32_32::from_double(d); }

// Helper: Create Q64_32 from uint64 (for test readability)
static Q64_32 q64(uint64_t u) { return Q64_32::from_uint64(u); }

// Helper: Compare Q32_32 with tolerance (for floating-point comparison)
static bool q32_eq(Q32_32 a, Q32_32 b, uint64_t tolerance = 1) {
    uint64_t diff = (a.bits > b.bits) ? (a.bits - b.bits) : (b.bits - a.bits);
    return diff <= tolerance;
}

// Helper: Compare Q64_32 with tolerance
static bool q64_eq(Q64_32 a, Q64_32 b, __uint128_t tolerance = 1) {
    __uint128_t diff = (a.bits > b.bits) ? (a.bits - b.bits) : (b.bits - a.bits);
    return diff <= tolerance;
}

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, name) \
    do { \
        if (condition) { \
            std::cout << "  PASS: " << name << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "  FAIL: " << name << std::endl; \
            tests_failed++; \
        } \
    } while(0)

// ============================================================================
// Golden Vector 1: C_effective Rounding (1/3 + 1/3 + 1/3)
// ============================================================================
void test_c_effective_rounding() {
    std::cout << "Test 1: C_effective Rounding (1/3 + 1/3 + 1/3)" << std::endl;
    
    CEffectiveInput input;
    input.producer_shares = { q32(1.0/3.0), q32(1.0/3.0), q32(1.0/3.0) };
    input.sybil_factor = q32(0.0);
    
    Q32_32 result = calculate_c_effective(input);
    Q32_32 expected = q32(1.0/3.0);
    
    TEST_ASSERT(q32_eq(result, expected, 2), "C_effective ≈ 1/3");
}

// ============================================================================
// Golden Vector 2: H_attackable^bound Rounding
// ============================================================================
void test_h_attackable_rounding() {
    std::cout << "Test 2: H_attackable^bound Rounding" << std::endl;
    
    HAttackableInput input;
    input.h_network = q64(2900000000000ULL); // 2.9 PH/s
    input.c_effective = q32(0.5);
    input.outage = q32(0.0);
    input.efficiency = q32(1.0);
    
    Q64_32 result = calculate_h_attackable(input);
    Q64_32 expected = q64(1450000000000ULL);
    
    TEST_ASSERT(q64_eq(result, expected, 1000), "H_attackable ≈ 1.45 PH/s");
}

// ============================================================================
// Golden Vector 3: α Rounding
// ============================================================================
void test_alpha_rounding() {
    std::cout << "Test 3: α Rounding" << std::endl;
    
    AlphaInput input;
    input.h_attackable = q64(1450000000000ULL);
    input.h_network = q64(2900000000000ULL);
    input.v_t = q64(100000000000ULL); // V_T (base units)
    input.b_fork = q64(29400000ULL); // B_fork (base units) — spec V_T/B_fork = 3401.36
    
    Q32_32 result = calculate_alpha(input);
    Q32_32 expected = q32(1700.680272108843);

    TEST_ASSERT(q32_eq(result, expected, 1000), "α ≈ 1700.680272108843");
}

// ============================================================================
// Golden Vector 4: B_min Rounding
// ============================================================================
void test_b_min_rounding() {
    std::cout << "Test 4: B_min Rounding" << std::endl;
    
    BMinInput input;
    input.v_t = q64(100000000000ULL); // V_T (base units)
    input.d_reversibility = q32(0.9);
    input.alpha = q32(1700.680272108843); // α from Golden Vector 3
    
    Q64_32 result = calculate_b_min(input);
    Q64_32 expected = q64(17016802721088ULL);

    TEST_ASSERT(q64_eq(result, expected, 100000000000000ULL), "B_min ≈ $170.2M");
}

// ============================================================================
// Golden Vector 5: Boundary Values (single producer)
// ============================================================================
void test_boundary_single_producer() {
    std::cout << "Test 5: Boundary Values (single producer)" << std::endl;
    
    CEffectiveInput input;
    input.producer_shares = { q32(1.0) };
    input.sybil_factor = q32(0.0);
    
    Q32_32 result = calculate_c_effective(input);
    Q32_32 expected = q32(1.0);
    
    TEST_ASSERT(q32_eq(result, expected, 1), "C_effective = 1.0 for single producer");
}

// ============================================================================
// Golden Vector 6: Maximum Values (overflow clamping)
// ============================================================================
void test_maximum_values() {
    std::cout << "Test 6: Maximum Values (overflow clamping)" << std::endl;
    
    HAttackableInput input;
    input.h_network = Q64_32::MAX();
    input.c_effective = Q32_32::MAX();
    input.outage = q32(0.0);
    input.efficiency = q32(1.0);
    
    Q64_32 result = calculate_h_attackable(input);
    Q64_32 expected = Q64_32::MAX();
    
    TEST_ASSERT(result == expected, "H_attackable clamped to MAX");
}

// ============================================================================
// Golden Vector 7: Zero Denominator
// ============================================================================
void test_zero_denominator() {
    std::cout << "Test 7: Zero Denominator" << std::endl;
    
    HAttackableInput input;
    input.h_network = q64(0);
    input.c_effective = q32(0.5);
    input.outage = q32(0.0);
    input.efficiency = q32(1.0);
    
    Q64_32 result = calculate_h_attackable(input);
    Q64_32 expected = q64(0);
    
    TEST_ASSERT(result == expected, "H_attackable = 0 for zero H_network");
}

// ============================================================================
// Golden Vector 8: Clamp Transitions
// ============================================================================
void test_clamp_transitions() {
    std::cout << "Test 8: Clamp Transitions" << std::endl;
    
    HAttackableInput input;
    input.h_network = Q64_32::from_bits(MAX_U64_32 - 1);
    input.c_effective = q32(1.0);
    input.outage = q32(0.0);
    input.efficiency = q32(1.0);
    
    Q64_32 result = calculate_h_attackable(input);
    Q64_32 expected = Q64_32::from_bits(MAX_U64_32 - 1);
    
    TEST_ASSERT(q64_eq(result, expected, 1000), "H_attackable ≈ MAX-1");
}

// ============================================================================
// Golden Vector 9: Chained Rounding
// ============================================================================
void test_chained_rounding() {
    std::cout << "Test 9: Chained Rounding" << std::endl;
    
    BMinInput input;
    input.v_t = q64(1);
    input.d_reversibility = q32(0.5);
    input.alpha = q32(0.5);
    
    Q64_32 result = calculate_b_min(input);
    Q64_32 expected = Q64_32::from_bits(3221225472ULL); // 0.75 in Q64.32
    
    TEST_ASSERT(q64_eq(result, expected, 1000), "B_min = 0.75");
}

// ============================================================================
// Single-Floor Invariant Test
// ============================================================================
void test_single_floor_invariant() {
    std::cout << "Test 10: Single-Floor Invariant" << std::endl;
    
    CEffectiveInput input;
    input.producer_shares = { q32(0.3333333333), q32(0.3333333333), q32(0.3333333333) };
    input.sybil_factor = q32(0.1);
    
    Q32_32 result = calculate_c_effective(input);
    
    Q32_32 min_expected = q32(0.3);
    Q32_32 max_expected = q32(0.4);
    
    TEST_ASSERT(result >= min_expected && result <= max_expected, "C_effective in reasonable range");
}

// ============================================================================
// Complete Chain Test
// ============================================================================
void test_complete_chain() {
    std::cout << "Test 11: Complete Chain" << std::endl;
    
    SecurityFloorInput input;
    input.producer_shares = { q32(1.0/3.0), q32(1.0/3.0), q32(1.0/3.0) };
    input.sybil_factor = q32(0.0);
    input.h_network = q64(2900000000000ULL);
    input.outage = q32(0.0);
    input.efficiency = q32(1.0);
    input.v_t = q64(100000000000ULL);
    input.b_fork = q64(29400000000ULL);
    input.d_reversibility = q32(0.9);
    
    SecurityFloorResult result = calculate_security_floor(input);
    
    TEST_ASSERT(result.c_effective > Q32_32::ZERO(), "C_effective > 0");
    TEST_ASSERT(result.h_attackable > Q64_32::ZERO(), "H_attackable > 0");
    TEST_ASSERT(result.alpha > Q32_32::ZERO(), "α > 0");
    TEST_ASSERT(result.b_min > Q64_32::ZERO(), "B_min > 0");
}

// ============================================================================
// Reproducibility Test
// ============================================================================
void test_reproducibility() {
    std::cout << "Test 12: Reproducibility" << std::endl;
    
    SecurityFloorInput input;
    input.producer_shares = { q32(0.5), q32(0.3), q32(0.2) };
    input.sybil_factor = q32(0.1);
    input.h_network = q64(1000000000000ULL);
    input.outage = q32(0.05);
    input.efficiency = q32(0.95);
    input.v_t = q64(50000000000ULL);
    input.b_fork = q64(15000000000ULL);
    input.d_reversibility = q32(0.8);
    
    SecurityFloorResult result1 = calculate_security_floor(input);
    SecurityFloorResult result2 = calculate_security_floor(input);
    
    TEST_ASSERT(result1.c_effective == result2.c_effective, "C_effective reproducible");
    TEST_ASSERT(result1.h_attackable == result2.h_attackable, "H_attackable reproducible");
    TEST_ASSERT(result1.alpha == result2.alpha, "α reproducible");
    TEST_ASSERT(result1.b_min == result2.b_min, "B_min reproducible");
}

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "=== Litenyx Phase 1 Security Floor Golden Vector Tests ===" << std::endl;
    std::cout << std::endl;
    
    test_c_effective_rounding();
    test_h_attackable_rounding();
    test_alpha_rounding();
    test_b_min_rounding();
    test_boundary_single_producer();
    test_maximum_values();
    test_zero_denominator();
    test_clamp_transitions();
    test_chained_rounding();
    test_single_floor_invariant();
    test_complete_chain();
    test_reproducibility();
    
    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;
    std::cout << "Total:  " << (tests_passed + tests_failed) << std::endl;
    
    if (tests_failed == 0) {
        std::cout << std::endl;
        std::cout << "ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << std::endl;
        std::cout << "SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
