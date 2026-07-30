// MSF Observational Adapter M03: Effective Security - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M03 = S_i = H_N * (1 - lambda_A) + H_A * lambda_A
// Authority class: OBSERVATIONAL_ADAPTER

#include "msf_adapter_m03.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace msf {

std::string AdapterM03::Compute(const BlockObservation& obs) {
    // S_i = H_N * (1 - lambda_A) + H_A * lambda_A
    int64_t H_N = obs.H_N;
    int64_t H_A = obs.H_A;
    double lambda_A = obs.lambda_A;

    int64_t native_int = H_N * (Q32_32_SCALE - (int64_t)(lambda_A * Q32_32_SCALE)) / Q32_32_SCALE;
    int64_t auxpow_int = H_A * (int64_t)(lambda_A * Q32_32_SCALE) / Q32_32_SCALE;

    int64_t total_int = native_int + auxpow_int;

    return ToQ32_32(total_int, 0);
}

std::string AdapterM03::ComputeFromParams(const ScenarioParams& params) {
    BlockObservation obs;
    obs.H_N = params.H_N;
    obs.H_A = params.H_A;
    obs.lambda_A = params.lambda_A;
    return Compute(obs);
}

bool AdapterM03::ValidateGolden(const std::string& computed, const std::string& expected) {
    std::string comp = computed;
    std::string exp = expected;
    std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);
    std::transform(exp.begin(), exp.end(), exp.begin(), ::tolower);
    return comp == exp;
}

std::string AdapterM03::ToQ32_32(int64_t integer_part, int64_t fractional_part) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0')
        << std::setw(8) << std::uppercase << (integer_part & 0xFFFFFFFF)
        << std::setw(8) << std::uppercase << (fractional_part & 0xFFFFFFFF);
    return oss.str();
}

int64_t AdapterM03::MulQ32_32(int64_t a, int64_t b) {
    return a * b;
}

} // namespace msf