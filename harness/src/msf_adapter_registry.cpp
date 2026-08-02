// MSF Adapter Registry Implementation - CE1 Phase 2
//
// Unified interface for all 10 observational adapters (M01-M10) per frozen SPEC-3.
// Authority class: EXPERIMENT_INFRASTRUCTURE

#include "msf_adapter_registry.h"
#include "msf_adapter_m01.h"
#include "msf_adapter_m02.h"
#include "msf_adapter_m03.h"
#include "msf_adapter_m04.h"
#include "msf_adapter_m05.h"
#include "msf_adapter_m06.h"
#include "msf_adapter_m07.h"
#include "msf_adapter_m08.h"
#include "msf_adapter_m09.h"
#include "msf_adapter_m10.h"

#include <algorithm>

namespace msf {

AdapterRegistry::AdapterRegistry() {
    RegisterAll();
    InitializeGoldenVectors();
}

void AdapterRegistry::RegisterAll() {
    // M01: Volume (V_i = D_i * v_per_tx)
    adapters_["M01"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M01";
        r.metric_name = "Volume";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM01::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M01"] = "Volume";
    authority_classes_["M01"] = "OBSERVATIONAL_ADAPTER";

    // M02: Profitability (Pi_i)
    adapters_["M02"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M02";
        r.metric_name = "Profitability";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM02::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M02"] = "Profitability";
    authority_classes_["M02"] = "OBSERVATIONAL_ADAPTER";

    // M03: Effective Security (S_i)
    adapters_["M03"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M03";
        r.metric_name = "Effective Security";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM03::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M03"] = "Effective Security";
    authority_classes_["M03"] = "OBSERVATIONAL_ADAPTER";

    // M04: Required Security (S_i_req)
    adapters_["M04"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M04";
        r.metric_name = "Required Security";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM04::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M04"] = "Required Security";
    authority_classes_["M04"] = "OBSERVATIONAL_ADAPTER";

    // M05: Fork Budget (B_fork_i)
    adapters_["M05"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M05";
        r.metric_name = "Fork Budget";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM05::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M05"] = "Fork Budget";
    authority_classes_["M05"] = "OBSERVATIONAL_ADAPTER";

    // M06: Minimum Fork Budget (B_min_i)
    adapters_["M06"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M06";
        r.metric_name = "Minimum Fork Budget";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM06::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M06"] = "Minimum Fork Budget";
    authority_classes_["M06"] = "OBSERVATIONAL_ADAPTER";

    // M07: Concentration (rho_i)
    adapters_["M07"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M07";
        r.metric_name = "Concentration";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM07::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M07"] = "Concentration";
    authority_classes_["M07"] = "OBSERVATIONAL_ADAPTER";

    // M08: Security Capital Efficiency (CE_i)
    adapters_["M08"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M08";
        r.metric_name = "Security Capital Efficiency";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM08::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M08"] = "Security Capital Efficiency";
    authority_classes_["M08"] = "OBSERVATIONAL_ADAPTER";

    // M09: Viability (Viable_i)
    adapters_["M09"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M09";
        r.metric_name = "Viability";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM09::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M09"] = "Viability";
    authority_classes_["M09"] = "OBSERVATIONAL_ADAPTER";

    // M10: Recovery Time
    adapters_["M10"] = [](const BlockObservation& obs) -> AdapterResult {
        AdapterResult r;
        r.metric_id = "M10";
        r.metric_name = "Recovery Time";
        r.authority_class = "OBSERVATIONAL_ADAPTER";
        try {
            r.value = AdapterM10::Compute(obs);
            r.valid = true;
        } catch (const std::exception& e) {
            r.valid = false;
            r.error_message = e.what();
        }
        return r;
    };
    metric_names_["M10"] = "Recovery Time";
    authority_classes_["M10"] = "OBSERVATIONAL_ADAPTER";
}

std::vector<AdapterResult> AdapterRegistry::ExecuteAll(const BlockObservation& obs) const {
    std::vector<AdapterResult> results;
    results.reserve(adapters_.size());
    
    // Execute in order M01-M10
    for (int i = 1; i <= 10; ++i) {
        std::string metric_id = "M" + std::string(i < 10 ? "0" : "") + std::to_string(i);
        auto it = adapters_.find(metric_id);
        if (it != adapters_.end()) {
            results.push_back(it->second(obs));
        }
    }
    
    return results;
}

std::optional<AdapterResult> AdapterRegistry::Execute(const std::string& metric_id, const BlockObservation& obs) const {
    auto it = adapters_.find(metric_id);
    if (it != adapters_.end()) {
        return it->second(obs);
    }
    return std::nullopt;
}

std::vector<std::string> AdapterRegistry::GetRegisteredMetrics() const {
    std::vector<std::string> metrics;
    metrics.reserve(adapters_.size());
    for (const auto& [id, fn] : adapters_) {
        metrics.push_back(id);
    }
    return metrics;
}

void AdapterRegistry::InitializeGoldenVectors() {
    // Baseline scenario (S01-like)
    BlockObservation base_obs;
    base_obs.block_number = 1;
    base_obs.chain_id = 1;
    base_obs.D_i = 100000;
    base_obs.H_N = 1000000;
    base_obs.H_A = 500000;
    base_obs.F = 100;
    base_obs.B = 5000;
    base_obs.alpha_A = 0.5;
    base_obs.lambda_A = 0.3;
    base_obs.rho_pool = 0.2;
    base_obs.rho_geo = 0.15;
    base_obs.gamma_NA = 0.0;
    base_obs.N_chains = 1;
    base_obs.c_N = 1;
    base_obs.c_A = 1;
    base_obs.v_per_tx = 50;
    base_obs.T_block = 60;
    base_obs.rho_i = 0.2;
    base_obs.V_i = base_obs.D_i * base_obs.v_per_tx;
    base_obs.Viable_i = true;
    base_obs.overflow_count = 0;
    base_obs.div_zero_count = 0;
    base_obs.replication_max_delta = "0x00000000";
    
    // Synthetic golden vectors for CE1 build-time validation
    // These are placeholder values - real values from SPEC-3 calibration
    golden_vectors_.push_back({"M01", base_obs, "0x0000001E848000000000"}); // 100000 * 50 = 5000000 = 0x4C4B40
    golden_vectors_.push_back({"M02", base_obs, "0x0000000000000000"}); // placeholder
    golden_vectors_.push_back({"M03", base_obs, "0x0000000000000000"}); // placeholder
    golden_vectors_.push_back({"M04", base_obs, "0x0000000000000000"}); // placeholder
    golden_vectors_.push_back({"M05", base_obs, "0x0000000000000000"}); // placeholder
    golden_vectors_.push_back({"M06", base_obs, "0x0000000000000000"}); // placeholder
    golden_vectors_.push_back({"M07", base_obs, "0x0000000033333333"}); // 0.2 in Q32.32
    golden_vectors_.push_back({"M08", base_obs, "0x0000000000000000"}); // placeholder
    golden_vectors_.push_back({"M09", base_obs, "0x0000000100000000"}); // true = 1
    golden_vectors_.push_back({"M10", base_obs, "0x0000006400000000"}); // 100
}

std::vector<AdapterRegistry::ValidationReport> AdapterRegistry::ValidateAllGolden() const {
    std::vector<ValidationReport> reports;
    
    for (const auto& gv : golden_vectors_) {
        auto it = adapters_.find(gv.metric_id);
        if (it != adapters_.end()) {
            AdapterResult result = it->second(gv.input);
            ValidationReport report;
            report.metric_id = gv.metric_id;
            report.passed = result.valid && (result.value == gv.expected_output);
            report.computed = result.value;
            report.expected = gv.expected_output;
            reports.push_back(report);
        }
    }
    
    return reports;
}

void EmitAdapterEvidence(
    EvidenceManifest& manifest,
    const std::vector<AdapterResult>& results,
    const std::string& authority_envelope_hash) {
    
    for (const auto& result : results) {
        if (result.valid) {
            EvidenceEntry entry;
            entry.key = result.metric_id;
            entry.value = result.value;
            entry.hash = "0x" + std::string(64, '0'); // Placeholder hash
            entry.timestamp = "2026-07-28T00:00:00Z";
            manifest.raw_observations.push_back(entry);
        }
    }
}

} // namespace msf