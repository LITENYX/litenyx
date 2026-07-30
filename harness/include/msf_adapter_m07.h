// MSF Observational Adapter M07: Concentration (rho_i) - CE1 Phase 2 Repair
//
// Frozen SPEC-3: M07 = rho_i = max(rho_pool, rho_geo)
// Authority class: OBSERVATIONAL_ADAPTER

#ifndef MSF_ADAPTER_M07_H
#define MSF_ADAPTER_M07_H

#include <string>
#include "msf_scenario_engine.h"

namespace msf {

class AdapterM07 {
public:
    static std::string Compute(const BlockObservation& obs);
    static std::string ComputeFromParams(const ScenarioParams& params);
    static bool ValidateGolden(const std::string& computed, const std::string& expected);
    static constexpr const char* MetricID() { return "M07"; }
    static constexpr const char* MetricName() { return "Concentration (rho_i)"; }
    static constexpr const char* AuthorityClass() { return "OBSERVATIONAL_ADAPTER"; }

private:
    static const int64_t Q32_32_SCALE = 0x100000000LL;
    static std::string ToQ32_32(int64_t integer_part, int64_t fractional_part);
};

} // namespace msf
#endif // MSF_ADAPTER_M07_H