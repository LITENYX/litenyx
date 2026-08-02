// MSF Observational Adapter M03: Effective Security (S_i) - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M03 = S_i = H_N * (1 - lambda_A) + H_A * lambda_A
// Authority class: OBSERVATIONAL_ADAPTER

#ifndef MSF_ADAPTER_M03_H
#define MSF_ADAPTER_M03_H

#include <string>
#include "msf_scenario_engine.h"

namespace msf {

class AdapterM03 {
public:
    static std::string Compute(const BlockObservation& obs);
    static std::string ComputeFromParams(const ScenarioParams& params);
    static bool ValidateGolden(const std::string& computed, const std::string& expected);
    static constexpr const char* MetricID() { return "M03"; }
    static constexpr const char* MetricName() { return "Effective Security (S_i)"; }
    static constexpr const char* AuthorityClass() { return "OBSERVATIONAL_ADAPTER"; }

private:
    static const int64_t Q32_32_SCALE = 0x100000000LL;
    static std::string ToQ32_32(int64_t integer_part, int64_t fractional_part);
    static int64_t MulQ32_32(int64_t a, int64_t b);
};

} // namespace msf
#endif // MSF_ADAPTER_M03_H