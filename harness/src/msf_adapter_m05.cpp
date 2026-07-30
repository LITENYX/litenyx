// MSF Observational Adapter M05: Fork Budget (B_fork_i) - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M05 = B_fork_i = k * V_i * (1 - rho_i), k = 3.0
// Authority class: OBSERVATIONAL_ADAPTER

#include "msf_adapter_m05.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace msf {

std::string AdapterM05::Compute(const BlockObservation& obs) {
    // B_fork_i = k * V_i * (1 - rho_i), k = 3.0
    int64_t V_i = obs.D_i * obs.v_per_tx; // V_i = D_i * v_per_tx
    double rho_i = std::max(obs.rho_pool, obs.rho_geo);
    
    int64_t k = 3; // k = 3.0 from SPEC-3
    
    int64_t budget_int = k * V_i * (Q32_32_SCALE - (int64_t)(rho_i * Q32_32_SCALE)) / Q32_32_SCALE;
    int64_t budget_frac = 0;
    
    return ToQ32_32(budget_int, budget_frac);
}

std::string AdapterM05::ComputeFromParams(const ScenarioParams& params) {
    BlockObservation obs;
    obs.D_i = params.D_i;
    obs.v_per_tx = params.v_per_tx;
    obs.rho_pool = params.rho_pool;
    obs.rho_geo = params.rho_geo;
    return Compute(obs);
}

bool AdapterM05::ValidateGolden(const std::string& computed, const std::string& expected) {
    std::string comp = computed;
    std::string exp = expected;
    std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);
    std::transform(exp.begin(), exp.end(), exp.begin(), ::tolower);
    return comp == exp;
}

std::string AdapterM05::ToQ32_32(int64_t integer_part, int64_t fractional_part) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0')
        << std::setw(8) << std::uppercase << (integer_part & 0xFFFFFFFF)
        << std::setw(8) << std::uppercase << (fractional_part & 0xFFFFFFFF);
    return oss.str();
}

int64_t AdapterM05::MulQ32_32(int64_t a, int64_t b) {
    return a * b;
}

} // namespace msf