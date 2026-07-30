// MSF Observational Adapter M08: Security Capital Efficiency (CE_i) - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M08 = CE_i = S_i / (c_N * H_N + c_A * H_A)
// Authority class: OBSERVATIONAL_ADAPTER

#include "msf_adapter_m08.h"
#include <sstream>
#include <iomanip>

namespace msf {

std::string AdapterM08::Compute(const BlockObservation& obs) {
    // CE_i = S_i / (c_N * H_N + c_A * H_A)
    // S_i = H_N * (1 - lambda_A) + H_A * lambda_A
    
    int64_t S_i_int = obs.H_N * (Q32_32_SCALE - (int64_t)(obs.lambda_A * Q32_32_SCALE)) / Q32_32_SCALE;
    S_i_int += obs.H_A * (int64_t)(obs.lambda_A * Q32_32_SCALE) / Q32_32_SCALE;
    
    int64_t cost_int = obs.c_N * obs.H_N + obs.c_A * obs.H_A;
    
    if (cost_int == 0) {
        return "0x0000000000000000";
    }
    
    int64_t ce_int = S_i_int / cost_int;
    int64_t ce_frac = ((S_i_int % cost_int) * Q32_32_SCALE) / cost_int;
    
    return ToQ32_32(ce_int, ce_frac);
}

std::string AdapterM08::ComputeFromParams(const ScenarioParams& params) {
    BlockObservation obs;
    obs.H_N = params.H_N;
    obs.H_A = params.H_A;
    obs.c_N = params.c_N;
    obs.c_A = params.c_A;
    obs.lambda_A = params.lambda_A;
    return Compute(obs);
}

bool AdapterM08::ValidateGolden(const std::string& computed, const std::string& expected) {
    std::string comp = computed;
    std::string exp = expected;
    std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);
    std::transform(exp.begin(), exp.end(), exp.begin(), ::tolower);
    return comp == exp;
}

std::string AdapterM08::ToQ32_32(int64_t integer_part, int64_t fractional_part) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0')
        << std::setw(8) << std::uppercase << (integer_part & 0xFFFFFFFF)
        << std::setw(8) << std::uppercase << (fractional_part & 0xFFFFFFFF);
    return oss.str();
}

} // namespace msf