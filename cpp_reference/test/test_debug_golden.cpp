// Litenyx Phase 1 — Debug golden vector tests
//
// Debug output to understand the failing tests

#include <iostream>
#include <iomanip>

#include "../../litenyx/LITENYX_fixed_point.h"
#include "../../litenyx/LITENYX_security_floor.h"

using namespace litenyx;

static Q32_32 q32(double d) { return Q32_32::from_double(d); }
static Q64_32 q64(uint64_t u) { return Q64_32::from_uint64(u); }

static void print_u128(const char* label, __uint128_t v) {
    uint64_t hi = static_cast<uint64_t>(v >> 64);
    uint64_t lo = static_cast<uint64_t>(v);
    if (hi == 0) {
        std::cout << "  " << label << lo << std::endl;
    } else {
        std::cout << "  " << label << "0x" << std::hex << hi << ":" << lo << std::dec << std::endl;
    }
}

int main() {
    std::cout << "=== Debug Golden Vector Tests ===" << std::endl;
    std::cout << std::fixed << std::setprecision(6);

    std::cout << std::endl;
    std::cout << "Test 2: H_attackable^bound" << std::endl;
    {
        HAttackableInput input;
        input.h_network = q64(2900000000000ULL);
        input.c_effective = q32(0.5);
        input.outage = q32(0.0);
        input.efficiency = q32(1.0);

        print_u128("H_network bits:", input.h_network.bits);
        print_u128("C_effective bits:", static_cast<__uint128_t>(input.c_effective.bits));

        Q32_32 one_minus_o = Q32_32::ONE() - input.outage;
        print_u128("(1-O) bits:", static_cast<__uint128_t>(one_minus_o.bits));

        Q64_32 h_times_c = input.h_network * input.c_effective;
        print_u128("H*C bits:", h_times_c.bits);
        std::cout << "  H*C as uint64: " << h_times_c.to_uint64() << std::endl;

        Q64_32 h_times_c_times_o = h_times_c * one_minus_o;
        print_u128("H*C*(1-O) bits:", h_times_c_times_o.bits);
        std::cout << "  H*C*(1-O) as uint64: " << h_times_c_times_o.to_uint64() << std::endl;

        Q64_32 result = h_times_c_times_o / input.efficiency;
        print_u128("Result bits:", result.bits);
        std::cout << "  Result as uint64: " << result.to_uint64() << std::endl;

        Q64_32 expected = q64(1450000000000ULL);
        print_u128("Expected bits:", expected.bits);
        std::cout << "  Expected as uint64: " << expected.to_uint64() << std::endl;

        __uint128_t diff = (result.bits > expected.bits) ? (result.bits - expected.bits) : (expected.bits - result.bits);
        print_u128("Difference:", diff);
    }

    std::cout << std::endl;
    std::cout << "Test 3: alpha" << std::endl;
    {
        AlphaInput input;
        input.h_attackable = q64(1450000000000ULL);
        input.h_network = q64(2900000000000ULL);
        input.v_t = q64(100000000000ULL);
        input.b_fork = q64(29400000000ULL);

        Q32_32 h_ratio = input.h_attackable.div_to_q32(input.h_network);
        std::cout << "  H_ratio bits: " << h_ratio.bits << std::endl;
        std::cout << "  H_ratio as double: " << h_ratio.to_double() << std::endl;

        Q32_32 v_ratio = input.v_t.div_to_q32(input.b_fork);
        std::cout << "  V_ratio bits: " << v_ratio.bits << std::endl;
        std::cout << "  V_ratio as double: " << v_ratio.to_double() << std::endl;

        Q32_32 result = h_ratio * v_ratio;
        std::cout << "  Result bits: " << result.bits << std::endl;
        std::cout << "  Result as double: " << result.to_double() << std::endl;

        Q32_32 expected = q32(1700.68);
        std::cout << "  Expected bits: " << expected.bits << std::endl;
        std::cout << "  Expected as double: " << expected.to_double() << std::endl;

        uint64_t diff = (result.bits > expected.bits) ? (result.bits - expected.bits) : (expected.bits - result.bits);
        std::cout << "  Difference: " << diff << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Test 4: B_min" << std::endl;
    {
        BMinInput input;
        input.v_t = q64(100000000000ULL);
        input.d_reversibility = q32(0.9);
        input.alpha = q32(1700.68);

        Q32_32 one_minus_d = Q32_32::ONE() - input.d_reversibility;
        std::cout << "  (1-D) bits: " << one_minus_d.bits << std::endl;
        std::cout << "  (1-D) as double: " << one_minus_d.to_double() << std::endl;

        Q32_32 one_plus_alpha = Q32_32::ONE() + input.alpha;
        std::cout << "  (1+alpha) bits: " << one_plus_alpha.bits << std::endl;
        std::cout << "  (1+alpha) as double: " << one_plus_alpha.to_double() << std::endl;

        Q64_32 v_times_d = input.v_t * one_minus_d;
        print_u128("V_T*(1-D) bits:", v_times_d.bits);
        std::cout << "  V_T*(1-D) as uint64: " << v_times_d.to_uint64() << std::endl;

        Q64_32 result = v_times_d * one_plus_alpha;
        print_u128("Result bits:", result.bits);
        std::cout << "  Result as uint64: " << result.to_uint64() << std::endl;

        Q64_32 expected = q64(17016802721088ULL);
        print_u128("Expected bits:", expected.bits);
        std::cout << "  Expected as uint64: " << expected.to_uint64() << std::endl;

        __uint128_t diff = (result.bits > expected.bits) ? (result.bits - expected.bits) : (expected.bits - result.bits);
        print_u128("Difference:", diff);
    }

    return 0;
}
