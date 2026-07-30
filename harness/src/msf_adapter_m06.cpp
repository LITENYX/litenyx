// MSF Observational Adapter M06: Minimum Fork Budget (B_min_i) - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M06 = B_min_i = k * V_i * rho_max, rho_max = 0.8
// Authority class: OBSERVATIONAL_ADAPTER

#include "msf_adapter_m06.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace msf {

std::string AdapterM06::Compute(const BlockObservation& obs) {
    // B_min_i = k * V_i * rho_max, rho_max = 0.8
    int64_t V_i = obs.D_i * obs.v_per_tx;
    double rho_max = 0.8;
    int64_t k = 3;
    
    int64_t min_budget_int = (int64_t)(k * V_i * rho_max);
    int64_t min_budget_frac = 0;
    
    return ToQ32_32(min_budget_int, min_budget_frac);
}

std::string AdapterM06::ComputeFromParams(const ScenarioParams& params) {
    BlockObservation obs;
    obs.D_i = params.D_i;
    obs.v_per_tx = params.v_per_tx;
    return Compute(obs);
}

bool AdapterM06::ValidateGolden(const std::string& computed, const std::string& expected) {
    std::string comp = computed;
    std::string exp = expected;
    std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);
    std::transform(exp.begin(), exp.end(), exp.begin(), ::tolower);
    return comp == exp;
}

std::string AdapterM06::ToQ32_32(int64_t integer_part, int64_t fractional_part) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0')
        << std::setw(8) << std::uppercase << (integer_part & 0xFFFFFFFF)
        << std::setw(8) << std::uppercase << (fractional_part & 0xFFFFFFFF);
    return oss.str();
}

int64_t AdapterM06::MulQ32_32(int64_t a, int64_t b) {
    return a * b;
}

} // namespace msf