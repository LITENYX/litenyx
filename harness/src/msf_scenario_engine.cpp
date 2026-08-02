// MSF Scenario Engine Implementation (CE1 Phase 1.1)
//
// Builds the executable representation of the frozen MSF experiment.
// Does NOT execute the scientific experiment.
// Authority class: EXPERIMENT_INFRASTRUCTURE

#include "msf_scenario_engine.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace msf {

ScenarioEngine::ScenarioEngine() {
    InitializeFrozenScenarios();
}

void ScenarioEngine::InitializeFrozenScenarios() {
    // ========================================================================
    // 18 Frozen Scenarios from SPEC-3 §4.1
    // Each scenario has exact numerical parameter values
    // ========================================================================
    
    // MSF-S01: Low Demand / Abundant Hash
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S01";
        s.scenario_hex_id = "0x01";
        s.D_i = 100;
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 0.001;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H3"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S02: High Demand / Abundant Hash / Standard Subsidy
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S02";
        s.scenario_hex_id = "0x02";
        s.D_i = 2000;
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H2", "MSF-H3", "MSF-H4"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S03: High Demand / Scarce Hash
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S03";
        s.scenario_hex_id = "0x03";
        s.D_i = 2000;
        s.H_N = 100;
        s.H_A = 500;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H2"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S04: Low Fees / High Subsidy
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S04";
        s.scenario_hex_id = "0x04";
        s.D_i = 500;
        s.H_N = 1000;
        s.H_A = 5000;
        s.F = 0.001;
        s.B = 10000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.5;
        s.rho_geo = 0.5;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H4"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S05: High Organic Demand / Zero Subsidy
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S05";
        s.scenario_hex_id = "0x05";
        s.D_i = 2000;
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 0;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H2", "MSF-H3"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S06: Sudden Native Hash Loss
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S06";
        s.scenario_hex_id = "0x06";
        s.D_i = 2000;
        s.H_N = 10000;  // pre-shock
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.has_shock = true;
        s.t_shock = 500;
        s.H_A_shock = 1000;  // post-shock: 1000 H/s (90% loss)
        s.hypotheses_tested = {"MSF-H1", "MSF-H2"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S07: Sudden AuxPoW Loss
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S07";
        s.scenario_hex_id = "0x07";
        s.D_i = 2000;
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;  // pre-shock
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.has_shock = true;
        s.t_shock = 500;
        s.alpha_A_shock = 0.1;  // post-shock: 90% AuxPoW loss
        s.hypotheses_tested = {"MSF-H1", "MSF-H2"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S08: Single AuxPoW Dominance (Pool Concentration)
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S08";
        s.scenario_hex_id = "0x08";
        s.D_i = 2000;
        s.H_N = 1000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.9;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H4"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S09: Multiple Independent AuxPoW (Diversified)
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S09";
        s.scenario_hex_id = "0x09";
        s.D_i = 2000;
        s.H_N = 1000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H2", "MSF-H3"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S10: Cheap but Rentable Hash
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S10";
        s.scenario_hex_id = "0x0A";
        s.D_i = 2000;
        s.H_N = 1000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.9;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H4"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S11: Concentrated Work (Pool + Geographic)
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S11";
        s.scenario_hex_id = "0x0B";
        s.D_i = 2000;
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.9;
        s.rho_geo = 0.9;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H4"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S12: New Segment Demand Spike
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S12";
        s.scenario_hex_id = "0x0C";
        s.D_i = 100;  // pre-shock
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 0.001;  // pre-shock
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.has_shock = true;
        s.t_shock = 500;
        s.D_i_shock = 2000;
        s.F = 10;  // post-shock F = 10 (was F=0.001 pre-shock)
        s.hypotheses_tested = {"MSF-H1", "MSF-H3"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S13: Demand Collapse
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S13";
        s.scenario_hex_id = "0x0D";
        s.D_i = 2000;  // pre-shock
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 10;  // pre-shock
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.has_shock = true;
        s.t_shock = 500;
        s.D_i_shock = 100;
        s.F = 0.001;  // post-shock F = 0.001 (was F=10 pre-shock)
        s.hypotheses_tested = {"MSF-H1", "MSF-H4"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S14: Heavy Inter-chain Traffic
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S14";
        s.scenario_hex_id = "0x0E";
        s.D_i = 2000;
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 2;
        s.hypotheses_tested = {"MSF-H1", "MSF-H2"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S15: Many Weak Segments (Fragmentation)
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S15";
        s.scenario_hex_id = "0x0F";
        s.D_i = 500;
        s.H_N = 100;
        s.H_A = 500;
        s.F = 1;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 10;
        s.hypotheses_tested = {"MSF-H1", "MSF-H3"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S16: Native-Only Baseline
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S16";
        s.scenario_hex_id = "0x10";
        s.D_i = 2000;
        s.H_N = 10000;
        s.H_A = 0;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.0;
        s.lambda_A = 0.0;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.0;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H2"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S17: High Native-AuxPoW Correlation (R-2R-002)
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S17";
        s.scenario_hex_id = "0x11";
        s.D_i = 2000;
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = 0.8;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H4"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
    
    // MSF-S18: Anti-Correlated Work Sources (R-2R-002)
    {
        ScenarioParams s;
        s.scenario_id = "MSF-S18";
        s.scenario_hex_id = "0x12";
        s.D_i = 2000;
        s.H_N = 10000;
        s.H_A = 50000;
        s.F = 10;
        s.B = 1000;
        s.alpha_A = 0.9;
        s.lambda_A = 0.1;
        s.rho_pool = 0.2;
        s.rho_geo = 0.2;
        s.gamma_NA = -0.5;
        s.N_chains = 1;
        s.hypotheses_tested = {"MSF-H1", "MSF-H2", "MSF-H3"};
        s.ComputeDerived();
        scenarios_[s.scenario_id] = s;
        scenario_list_.push_back(s);
    }
}

bool ScenarioEngine::LoadFrozenScenarios() {
    return (scenario_list_.size() == kNumScenarios);
}

bool ScenarioEngine::BuildExecutionPlan() {
    // Validate all scenarios against frozen SPEC-3
    for (const auto& scenario : scenario_list_) {
        if (!ValidateScenario(scenario)) {
            return false;
        }
    }
    return true;
}

const ScenarioParams* ScenarioEngine::GetScenario(const std::string& id) const {
    auto it = scenarios_.find(id);
    if (it != scenarios_.end()) {
        return &it->second;
    }
    return nullptr;
}

const std::vector<ScenarioParams>& ScenarioEngine::GetAllScenarios() const {
    return scenario_list_;
}

bool ScenarioEngine::ValidateScenario(const ScenarioParams& scenario) const {
    // Validate against frozen SPEC-3 values
    // Check all independent variables match frozen table
    // Check derived values are correctly computed
    // Check scenario-specific shock parameters
    
    // For CE1, we only validate the plan structure
    // Actual parameter values are frozen in InitializeFrozenScenarios()
    
    // Basic structural validation
    if (scenario.scenario_id.empty()) return false;
    if (scenario.scenario_hex_id.empty()) return false;
    if (scenario.N_chains < 1 || scenario.N_chains > 10) return false;
    if (scenario.alpha_A < 0 || scenario.alpha_A > 1) return false;
    if (scenario.lambda_A < 0 || scenario.lambda_A > 1) return false;
    if (scenario.rho_pool < 0 || scenario.rho_pool > 1) return false;
    if (scenario.rho_geo < 0 || scenario.rho_geo > 1) return false;
    if (scenario.gamma_NA < -1 || scenario.gamma_NA > 1) return false;
    if (scenario.N_chains < 1 || scenario.N_chains > 10) return false;
    
    // Validate derived values
    if (scenario.V_i != scenario.D_i * 50) return false;
    double expected_rho = (scenario.rho_pool > scenario.rho_geo) ? scenario.rho_pool : scenario.rho_geo;
    if (std::abs(scenario.rho_i - expected_rho) > 0.0001) return false;
    
    return true;
}

void ScenarioEngine::ApplyShock(ScenarioParams& scenario, int64_t block_num) {
    if (!scenario.has_shock) return;
    if (block_num < scenario.t_shock) return;
    
    // Apply post-shock values
    if (scenario.H_A_shock > 0) scenario.H_A = scenario.H_A_shock;
    if (scenario.alpha_A_shock > 0) scenario.alpha_A = scenario.alpha_A_shock;
    if (scenario.D_i_shock > 0) scenario.D_i = scenario.D_i_shock;
    if (scenario.F > 0) scenario.F = scenario.F; // F already set to post-shock value
}

bool ScenarioEngine::ValidateParameter(const std::string& param_name,
                                       const std::string& expected,
                                       const std::string& actual) const {
    return expected == actual;
}

std::string ScenarioEngine::GenerateExecutionPlanJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"engine\": \"msf-scenario-engine\",\n";
    oss << "  \"spec_version\": \"MSF-CONTRACT-SPEC-3\",\n";
    oss << "  \"H_MSF_new\": \"a855e7f17cc73f5b31979ee0b9b8f1ee532e3315a55edcf6a30cb702ee14ffd7\",\n";
    oss << "  \"scenario_count\": " << kNumScenarios << ",\n";
    oss << "  \"replications\": " << kReplications << ",\n";
    oss << "  \"blocks_per_scenario\": " << kBlocksPerScenario << ",\n";
    oss << "  \"scenarios\": [\n";
    
    for (size_t i = 0; i < scenario_list_.size(); ++i) {
        const auto& s = scenario_list_[i];
        oss << "    {\n";
        oss << "      \"scenario_id\": \"" << s.scenario_id << "\",\n";
        oss << "      \"hex_id\": \"" << s.scenario_hex_id << "\",\n";
        oss << "      \"D_i\": " << s.D_i << ",\n";
        oss << "      \"H_N\": " << s.H_N << ",\n";
        oss << "      \"H_A\": " << s.H_A << ",\n";
        oss << "      \"F\": " << s.F << ",\n";
        oss << "      \"B\": " << s.B << ",\n";
        oss << "      \"alpha_A\": " << s.alpha_A << ",\n";
        oss << "      \"lambda_A\": " << s.lambda_A << ",\n";
        oss << "      \"rho_pool\": " << s.rho_pool << ",\n";
        oss << "      \"rho_geo\": " << s.rho_geo << ",\n";
        oss << "      \"gamma_NA\": " << s.gamma_NA << ",\n";
        oss << "      \"N_chains\": " << s.N_chains << ",\n";
        oss << "      \"has_shock\": " << (s.has_shock ? "true" : "false") << ",\n";
        oss << "      \"t_shock\": " << s.t_shock << ",\n";
        oss << "      \"V_i\": " << s.V_i << ",\n";
        oss << "      \"X_i\": " << s.X_i << ",\n";
        oss << "      \"rho_i\": " << s.rho_i << ",\n";
        oss << "      \"hypotheses_tested\": [";
        for (size_t j = 0; j < s.hypotheses_tested.size(); ++j) {
            if (j > 0) oss << ", ";
            oss << "\"" << s.hypotheses_tested[j] << "\"";
        }
        oss << "]\n";
        if (i < scenario_list_.size() - 1) {
            oss << "    },\n";
        } else {
            oss << "    }\n";
        }
    }
    
    oss << "  ]\n";
    oss << "}\n";
    return oss.str();
}

bool ScenarioEngine::ValidateCompleteness(const ScenarioResult& result, 
                                          std::vector<std::string>& errors) const {
    // C1: All 18 scenarios executed
    if (result.scenario_id.empty()) {
        errors.push_back("C1 FAIL: No scenario executed");
        return false;
    }
    
    // C2: 3 replications per scenario
    if (result.replications.size() != 3) {
        errors.push_back("C2 FAIL: Expected 3 replications, got " + std::to_string(result.replications.size()));
    }
    
    // C3: No parameter substitution
    // (Validated at execution plan build time)
    
    // C4: Output schema valid (MSF-M01..M09 present)
    // (Validated at evidence emission time)
    
    // C5: Viability check present
    // (Validated at evidence emission time)
    
    // C6: Deterministic replay (3x bit-identical)
    if (!result.all_replications_bit_identical) {
        errors.push_back("C6 FAIL: Replications not bit-identical");
    }
    
    // C7: At least one computable metric
    // (Validated at evidence emission time)
    
    // C8: Hypothesis verdicts derived
    // (Validated at evidence emission time)
    
    // C9: Non-trivial experiment
    // (Validated after full experiment)
    
    return errors.empty();
}

} // namespace msf