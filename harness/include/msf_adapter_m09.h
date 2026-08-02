// MSF Observational Adapter M09: Viability (Viable_i) - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M09 = Viable_i = (S_i >= S_i_req) && (Pi_i >= 0) && (no anomalies)
// Authority class: OBSERVATIONAL_ADAPTER

#ifndef MSF_ADAPTER_M09_H
#define MSF_ADAPTER_M09_H

#include <string>
#include "msf_scenario_engine.h"

namespace msf {

class AdapterM09 {
public:
    static std::string Compute(const BlockObservation& obs);
    static std::string ComputeFromParams(const ScenarioParams& params);
    static bool ValidateGolden(const std::string& computed, const std::string& expected);
    static constexpr const char* MetricID() { return "M09"; }
    static constexpr const char* MetricName() { return "Viability (Viable_i)"; }
    static constexpr const char* AuthorityClass() { return "OBSERVATIONAL_ADAPTER"; }

private:
    static const int64_t Q32_32_SCALE = 0x100000000LL;
    static std::string ToQ32_32(int64_t integer_part, int64_t fractional_part);
};

} // namespace msf
#endif // MSF_ADAPTER_M09_H