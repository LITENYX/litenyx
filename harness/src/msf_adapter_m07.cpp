// MSF Observational Adapter M07: Concentration (rho_i) - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M07 = rho_i = max(rho_pool, rho_geo)
// Authority class: OBSERVATIONAL_ADAPTER

#include "msf_adapter_m07.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace msf {

std::string AdapterM07::Compute(const BlockObservation& obs) {
    // rho_i = max(rho_pool, rho_geo)
    double rho_i = std::max(obs.rho_pool, obs.rho_geo);
    
    int64_t integer_part = (int64_t)rho_i;
    int64_t fractional_part = (int64_t)((rho_i - integer_part) * Q32_32_SCALE);
    
    return ToQ32_32(integer_part, fractional_part);
}

std::string AdapterM07::ComputeFromParams(const ScenarioParams& params) {
    BlockObservation obs;
    obs.rho_pool = params.rho_pool;
    obs.rho_geo = params.rho_geo;
    return Compute(obs);
}

bool AdapterM07::ValidateGolden(const std::string& computed, const std::string& expected) {
    std::string comp = computed;
    std::string exp = expected;
    std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);
    std::transform(exp.begin(), exp.end(), exp.begin(), ::tolower);
    return comp == exp;
}

std::string AdapterM07::ToQ32_32(int64_t integer_part, int64_t fractional_part) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0')
        << std::setw(8) << std::uppercase << (integer_part & 0xFFFFFFFF)
        << std::setw(8) << std::uppercase << (fractional_part & 0xFFFFFFFF);
    return oss.str();
}

} // namespace msf