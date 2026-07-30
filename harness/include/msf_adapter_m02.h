// MSF Observational Adapter M02: Profitability (Pi_i) - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M02 = Pi_i = (B + F) * (H_N / (H_N + H_A)) - c_N * H_N
// Authority class: OBSERVATIONAL_ADAPTER

#ifndef MSF_ADAPTER_M02_H
#define MSF_ADAPTER_M02_H

#include <string>
#include "msf_scenario_engine.h"

namespace msf {

class AdapterM02 {
public:
    static std::string Compute(const BlockObservation& obs);
    static std::string ComputeFromParams(const ScenarioParams& params);
    static bool ValidateGolden(const std::string& computed, const std::string& expected);
    static constexpr const char* MetricID() { return "M02"; }
    static constexpr const char* MetricName() { return "Profitability (Pi_i)"; }
    static constexpr const char* AuthorityClass() { return "OBSERVATIONAL_ADAPTER"; }

private:
    static const int64_t Q32_32_SCALE = 0x100000000LL;
    static std::string ToQ32_32(int64_t integer_part, int64_t fractional_part);
    static int64_t MulQ32_32(int64_t a, int64_t b);
    static int64_t DivQ32_32(int64_t a, int64_t b);
};

} // namespace msf
#endif // MSF_ADAPTER_M02_H