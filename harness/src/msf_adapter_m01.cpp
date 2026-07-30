// MSF Observational Adapter M01: Volume - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M01 = V_i = D_i × v_per_tx
// Authority class: OBSERVATIONAL_ADAPTER

#include "msf_adapter_m01.h"
#include <sstream>
#include <iomanip>

namespace msf {

std::string AdapterM01::Compute(const BlockObservation& obs) {
    // M01 = V_i = D_i × v_per_tx
    int64_t D_i = obs.D_i;
    int64_t v_per_tx = obs.v_per_tx;

    int64_t volume_int = D_i * v_per_tx;
    int64_t volume_frac = 0;

    return ToQ32_32(volume_int, volume_frac);
}

std::string AdapterM01::ComputeFromParams(const ScenarioParams& params) {
    BlockObservation obs;
    obs.D_i = params.D_i;
    obs.v_per_tx = params.v_per_tx;
    return Compute(obs);
}

bool AdapterM01::ValidateGolden(const std::string& computed, const std::string& expected) {
    std::string comp = computed;
    std::string exp = expected;
    std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);
    std::transform(exp.begin(), exp.end(), exp.begin(), ::tolower);
    return comp == exp;
}

std::string AdapterM01::ToQ32_32(int64_t integer_part, int64_t fractional_part) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0')
        << std::setw(8) << std::uppercase << (integer_part & 0xFFFFFFFF)
        << std::setw(8) << std::uppercase << (fractional_part & 0xFFFFFFFF);
    return oss.str();
}

int64_t AdapterM01::MulQ32_32(int64_t a, int64_t b) {
    return a * b;
}

} // namespace msf