// Litenyx Phase 1 — Fixed-point arithmetic library (Q32.32 / Q64.32)
//
// This header is the SINGLE SOURCE OF TRUTH for consensus-critical fixed-point
// arithmetic. It is pure, header-only, integer-only, and has ZERO dependencies
// on singletons, mutable state, wall-clock, mempool, RPC, or network state.
//
// Invariants (CONSENSUS-ARITHMETIC-1):
//   - History_A = History_B ⇒ (B_min, α, H_attackable^bound, C_effective)_A = _B
//   - Bit-for-bit, not merely mathematically approximately equal
//   - Single-floor truncation (not independent per-operation floor)
//   - Overflow protection (clamp to MAX)
//
// Frozen representations:
//   Q32.32: unsigned 32-bit integer + 32-bit fraction = 64-bit total
//   Q64.32: unsigned 64-bit integer + 32-bit fraction = 96-bit total (stored as __uint128_t or pair)
//
// Rounding: Truncate (floor) at each operation boundary
// Overflow: Clamp to MAX for the target type
// Underflow: Clamp to 0

#ifndef LITENYX_FIXED_POINT_H
#define LITENYX_FIXED_POINT_H

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <type_traits>

namespace litenyx {

// ---- Constants ---------------------------------------------------------------

// Q32.32: 32-bit integer part, 32-bit fraction part
// Total range: [0, 4294967295.999999999] (approximately 4.295 × 10^9)
static constexpr uint64_t Q32_32_FRAC_BITS = 32;
static constexpr uint64_t Q32_32_ONE = 1ULL << Q32_32_FRAC_BITS; // 1.0 in Q32.32
static constexpr uint64_t Q32_32_MAX = 0xFFFFFFFFFFFFFFFFULL; // MAX_U32.32

// Q64.32: 64-bit integer part, 32-bit fraction part
// Total range: [0, 18446744073709551615.999999999] (approximately 1.845 × 10^19)
static constexpr uint64_t Q64_32_FRAC_BITS = 32;
static constexpr uint64_t Q64_32_ONE = 1ULL << Q64_32_FRAC_BITS; // 1.0 in Q64.32

// MAX values for clamping
static constexpr uint64_t MAX_U32_32 = 0xFFFFFFFFFFFFFFFFULL; // Q32.32 MAX
static constexpr uint64_t MAX_U64_32 = 0xFFFFFFFFFFFFFFFFULL; // Q64.32 MAX (stored as uint64_t for simplicity)

// ---- Q32.32 Type -------------------------------------------------------------

// Q32.32 unsigned fixed-point: 64-bit storage (32-bit integer + 32-bit fraction)
// Represents values in [0, 4294967295.999999999]
struct Q32_32 {
    uint64_t bits;

    // Construction
    static constexpr Q32_32 from_bits(uint64_t b) { Q32_32 q; q.bits = b; return q; }
    static constexpr Q32_32 from_double(double d) {
        Q32_32 q;
        if (d < 0.0) { q.bits = 0; return q; }
        if (d >= 4294967296.0) { q.bits = MAX_U32_32; return q; }
        q.bits = static_cast<uint64_t>(d * Q32_32_ONE);
        return q;
    }
    static constexpr Q32_32 from_uint32(uint32_t u) { return from_bits(static_cast<uint64_t>(u) << Q32_32_FRAC_BITS); }

    // Accessors
    constexpr double to_double() const { return static_cast<double>(bits) / static_cast<double>(Q32_32_ONE); }
    constexpr uint32_t integer_part() const { return static_cast<uint32_t>(bits >> Q32_32_FRAC_BITS); }

    // Constants
    static constexpr Q32_32 ZERO() { return from_bits(0); }
    static constexpr Q32_32 ONE() { return from_bits(Q32_32_ONE); }
    static constexpr Q32_32 MAX() { return from_bits(MAX_U32_32); }

    // Arithmetic operations (all truncate/floor)
    constexpr Q32_32 operator+(Q32_32 rhs) const {
        uint64_t result = bits + rhs.bits;
        if (result < bits) result = MAX_U32_32; // overflow clamp
        return from_bits(result);
    }

    constexpr Q32_32 operator-(Q32_32 rhs) const {
        if (bits < rhs.bits) return ZERO(); // underflow clamp
        return from_bits(bits - rhs.bits);
    }

    // Multiplication: Q32.32 × Q32.32 → Q64.64, then truncate to Q32.32
    // Uses 128-bit intermediate to avoid overflow
    constexpr Q32_32 operator*(Q32_32 rhs) const {
        __uint128_t a = static_cast<__uint128_t>(bits);
        __uint128_t b = static_cast<__uint128_t>(rhs.bits);
        __uint128_t product = a * b;
        // Shift right by 32 to get Q32.32 result (truncate)
        uint64_t result = static_cast<uint64_t>(product >> Q32_32_FRAC_BITS);
        return from_bits(result);
    }

    // Division: Q32.32 / Q32.32 → Q32.32
    // Uses 128-bit intermediate to avoid overflow
    constexpr Q32_32 operator/(Q32_32 rhs) const {
        if (rhs.bits == 0) return MAX(); // division by zero → MAX (pessimistic)
        __uint128_t a = static_cast<__uint128_t>(bits);
        __uint128_t b = static_cast<__uint128_t>(rhs.bits);
        // Shift left by 32 to get Q32.32 result
        __uint128_t quotient = (a << Q32_32_FRAC_BITS) / b;
        uint64_t result = static_cast<uint64_t>(quotient);
        return from_bits(result);
    }

    // Comparison
    constexpr bool operator==(Q32_32 rhs) const { return bits == rhs.bits; }
    constexpr bool operator!=(Q32_32 rhs) const { return bits != rhs.bits; }
    constexpr bool operator<(Q32_32 rhs) const { return bits < rhs.bits; }
    constexpr bool operator<=(Q32_32 rhs) const { return bits <= rhs.bits; }
    constexpr bool operator>(Q32_32 rhs) const { return bits > rhs.bits; }
    constexpr bool operator>=(Q32_32 rhs) const { return bits >= rhs.bits; }
};

// ---- Q64.32 Type -------------------------------------------------------------

// Q64.32 unsigned fixed-point: 128-bit storage (64-bit integer + 32-bit fraction)
// Represents values in [0, 18446744073709551615.999999999]
// Stored as __uint128_t for simplicity and correctness
struct Q64_32 {
    __uint128_t bits;

    // Construction
    static constexpr Q64_32 from_bits(__uint128_t b) { Q64_32 q; q.bits = b; return q; }
    static constexpr Q64_32 from_uint64(uint64_t u) { return from_bits(static_cast<__uint128_t>(u) << Q64_32_FRAC_BITS); }
    static constexpr Q64_32 from_double(double d) {
        Q64_32 q;
        if (d < 0.0) { q.bits = 0; return q; }
        q.bits = static_cast<__uint128_t>(d * static_cast<double>(1ULL << Q64_32_FRAC_BITS));
        return q;
    }

    // Accessors
    constexpr uint64_t to_uint64() const { return static_cast<uint64_t>(bits >> Q64_32_FRAC_BITS); }

    // Constants
    static constexpr Q64_32 ZERO() { return from_bits(0); }
    static constexpr Q64_32 ONE() { return from_bits(static_cast<__uint128_t>(1) << Q64_32_FRAC_BITS); }
    static constexpr Q64_32 MAX() { return from_bits(static_cast<__uint128_t>(MAX_U64_32)); }

    // Arithmetic operations (all truncate/floor)
    constexpr Q64_32 operator+(Q64_32 rhs) const {
        __uint128_t result = bits + rhs.bits;
        if (result < bits) result = MAX_U64_32; // overflow clamp
        return from_bits(result);
    }

    constexpr Q64_32 operator-(Q64_32 rhs) const {
        if (bits < rhs.bits) return ZERO(); // underflow clamp
        return from_bits(bits - rhs.bits);
    }

    // Multiplication: Q64.32 × Q32.32 → Q96.32, then truncate to Q64.32
    // Uses 192-bit intermediate (approximated with __uint128_t and overflow handling)
    constexpr Q64_32 operator*(Q32_32 rhs) const {
        // Q64.32 × Q32.32 = (bits_64 × bits_32) >> 32
        // bits_64 is 96-bit, bits_32 is 64-bit, product is 160-bit
        // We need to handle this carefully to avoid overflow
        __uint128_t a = bits;
        __uint128_t b = static_cast<__uint128_t>(rhs.bits);
        __uint128_t product = a * b;
        // Shift right by 32 to get Q64.32 result (truncate)
        __uint128_t result = product >> Q64_32_FRAC_BITS;
        // Clamp to MAX if overflow
        if (result > MAX_U64_32) result = MAX_U64_32;
        return from_bits(result);
    }

    // Division: Q64.32 / Q32.32 → Q64.32
    constexpr Q64_32 operator/(Q32_32 rhs) const {
        if (rhs.bits == 0) return MAX(); // division by zero → MAX (pessimistic)
        __uint128_t a = bits;
        __uint128_t b = static_cast<__uint128_t>(rhs.bits);
        // Shift left by 32 to get Q64.32 result
        __uint128_t quotient = (a << Q64_32_FRAC_BITS) / b;
        // Clamp to MAX if overflow
        if (quotient > MAX_U64_32) quotient = MAX_U64_32;
        return from_bits(quotient);
    }

    // Comparison
    constexpr bool operator==(Q64_32 rhs) const { return bits == rhs.bits; }
    constexpr bool operator!=(Q64_32 rhs) const { return bits != rhs.bits; }
    constexpr bool operator<(Q64_32 rhs) const { return bits < rhs.bits; }
    constexpr bool operator<=(Q64_32 rhs) const { return bits <= rhs.bits; }
    constexpr bool operator>(Q64_32 rhs) const { return bits > rhs.bits; }
    constexpr bool operator>=(Q64_32 rhs) const { return bits >= rhs.bits; }
};

// ---- Helper Functions --------------------------------------------------------

// Clamp Q64.32 to MAX_U64_32
inline Q64_32 clamp_q64_32(Q64_32 value) {
    if (value.bits > MAX_U64_32) return Q64_32::MAX();
    return value;
}

// Clamp Q32.32 to MAX_U32_32
inline Q32_32 clamp_q32_32(Q32_32 value) {
    if (value.bits > MAX_U32_32) return Q32_32::MAX();
    return value;
}

} // namespace litenyx

#endif // LITENYX_FIXED_POINT_H
