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
    std::cout << "=== Debug Test 4 (B_min) ===" << std::endl;
    
    BMinInput input;
    input.v_t = q64(100000000000ULL);
    input.d_reversibility = q32(0.9);
    input.alpha = q32(1700.680272108843);
    
    std::cout << "alpha bits: " << input.alpha.bits << std::endl;
    std::cout << "alpha double: " << input.alpha.to_double() << std::endl;
    
    Q32_32 one_minus_d = Q32_32::ONE() - input.d_reversibility;
    Q32_32 one_plus_alpha = Q32_32::ONE() + input.alpha;
    
    std::cout << "(1-D) bits: " << one_minus_d.bits << " val: " << one_minus_d.to_double() << std::endl;
    std::cout << "(1+alpha) bits: " << one_plus_alpha.bits << " val: " << one_plus_alpha.to_double() << std::endl;
    
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
    
    return 0;
}
