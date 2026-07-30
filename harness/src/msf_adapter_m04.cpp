// MSF Observational Adapter M04: Required Security - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M04 = S_i_req = B_fork_i / T_block
// Authority class: OBSERVATIONAL_ADAPTER

#include "msf_adapter_m04.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace msf {

std::string AdapterM04::Compute(const BlockObservation& obs) {
    // S_i_req = B_fork_i / T_block
    // B_fork_i = k * V_i * (1 - rho_i)
    int64_t V_i = obs.V_i;
    double rho_i = std::max(obs.rho_pool, obs.rho_geo);
    int64_t k = 3; // fork multiplier from SPEC-3

    int64_t B_fork_int = k * V_i * (Q32_32_SCALE - (int64_t)(rho_i * Q32_32_SCALE)) / Q32_32_SCALE;
    int64_t required_int = B_fork_int / T_BLOCK;
    int64_t required_frac = (B_fork_int % T_BLOCK) * Q32_32_SCALE / T_BLOCK;

    return ToQ32_32(required_int, required_frac);
}

std::string AdapterM04::ComputeFromParams(const ScenarioParams& params) {
    BlockObservation obs;
    obs.V_i = params.V_i;
    obs.rho_pool = params.rho_pool;
    obs.rho_geo = params.rho_geo;
    return Compute(obs);
}

bool AdapterM04::ValidateGolden(const std::string& computed, const std::string& expected) {
    std::string comp = computed;
    std::string exp = expected;
    std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);
    std::transform(exp.begin(), exp.end(), exp.begin(), ::tolower);
    return comp == exp;
}

std::string AdapterM04::ToQ32_32(int64_t integer_part, int64_t fractional_part) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0')
        << std::setw(8) << std::uppercase << (integer_part & 0xFFFFFFFF)
        << std::setw(8) << std::uppercase << (fractional_part & 0xFFFFFFFF);
    return oss.str();
}

int64_t AdapterM04::MulQ32_32(int64_t a, int64_t b) {
    return a * b;
}

int64_t AdapterM04::DivQ32_32(int64_t a, int64_t b) {
    if (b == 0) return 0;
    return a / b;
}

} // namespace msf