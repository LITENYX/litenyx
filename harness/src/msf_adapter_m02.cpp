// MSF Observational Adapter M02: Profitability - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M02 = Pi_i = (B + F) * (H_N / (H_N + H_A)) - c_N * H_N
// Authority class: OBSERVATIONAL_ADAPTER

#include "msf_adapter_m02.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace msf {

std::string AdapterM02::Compute(const BlockObservation& obs) {
    // Pi_i = (B + F) * (H_N / (H_N + H_A)) - c_N * H_N
    int64_t B = obs.B;
    int64_t F = obs.F;
    int64_t H_N = obs.H_N;
    int64_t H_A = obs.H_A;
    int64_t c_N = obs.c_N;

    int64_t H_total = H_N + H_A;
    if (H_total == 0) {
        return "0x0000000000000000";
    }

    int64_t revenue_share_int = (B + F) * H_N / H_total;
    int64_t revenue_share_frac = (((B + F) * H_N) % H_total) * Q32_32_SCALE / H_total;

    int64_t cost_int = c_N * H_N;
    int64_t cost_frac = 0;

    int64_t net_int = revenue_share_int - cost_int;
    int64_t net_frac = revenue_share_frac - cost_frac;

    if (net_frac < 0) {
        net_int -= 1;
        net_frac += Q32_32_SCALE;
    }

    return ToQ32_32(net_int, net_frac);
}

std::string AdapterM02::ComputeFromParams(const ScenarioParams& params) {
    BlockObservation obs;
    obs.B = params.B;
    obs.F = params.F;
    obs.H_N = params.H_N;
    obs.H_A = params.H_A;
    obs.c_N = params.c_N;
    return Compute(obs);
}

bool AdapterM02::ValidateGolden(const std::string& computed, const std::string& expected) {
    std::string comp = computed;
    std::string exp = expected;
    std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);
    std::transform(exp.begin(), exp.end(), exp.begin(), ::tolower);
    return comp == exp;
}

std::string AdapterM02::ToQ32_32(int64_t integer_part, int64_t fractional_part) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0')
        << std::setw(8) << std::uppercase << (integer_part & 0xFFFFFFFF)
        << std::setw(8) << std::uppercase << (fractional_part & 0xFFFFFFFF);
    return oss.str();
}

int64_t AdapterM02::MulQ32_32(int64_t a, int64_t b) {
    return a * b;
}

int64_t AdapterM02::DivQ32_32(int64_t a, int64_t b) {
    if (b == 0) return 0;
    return a / b;
}

} // namespace msf