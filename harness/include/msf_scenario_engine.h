// MSF Scenario Execution Engine (CE1 Phase 1.1)
//
// Builds the executable representation of the frozen MSF experiment.
// Does NOT execute the scientific experiment.
// Implements: 18 scenarios × 3 replications × 1000 blocks
// Authority class: EXPERIMENT_INFRASTRUCTURE

#ifndef MSF_SCENARIO_ENGINE_H
#define MSF_SCENARIO_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include <memory>

#include "harness_evidence.h"
#include "harness_dag.h"

namespace msf {

// ============================================================================
// SPEC-3 Frozen Constants (from §3.5, §4.1)
// ============================================================================

// Controlled variables (§3.5)
static constexpr int64_t kTBlock = 60;                    // Block time (seconds)
static constexpr const char* kHashAlgorithm = "Scrypt";   // Hash algorithm
static constexpr int64_t kVPerTx = 50;                    // Avg tx value (ASU/tx)
static constexpr int64_t kDReversibility = 0.1;           // Reversibility factor
static constexpr int64_t kSybilFactor = 1.0;              // Sybil resistance factor
static constexpr int64_t kSafetyMargin = 0.2;             // Safety margin (alpha)
static constexpr int64_t kForkMultiplier = 3.0;           // B_min = k * V_i
static constexpr int64_t kMaxConcentration = 0.8;         // rho_max
static constexpr int64_t kMaxRentability = 0.7;           // lambda_max
static constexpr int64_t kCostNative = 0.001;             // c_N (ASU/H)
static constexpr int64_t kCostAuxPow = 0.0001;            // c_A (ASU/H)

// Scenario count
static constexpr int kNumScenarios = 18;
static constexpr int kReplications = 3;
static constexpr int kBlocksPerScenario = 1000;

// ============================================================================
// Scenario Parameter Vector (frozen from SPEC-3 §4.1)
// ============================================================================

struct ScenarioParams {
    // Identity
    std::string scenario_id;      // e.g., "MSF-S01"
    std::string scenario_hex_id;  // e.g., "0x01"
    
    // Independent variables (§3.2)
    int64_t D_i;                  // Demand level (tx/day)
    int64_t H_N;                  // Native hash rate (H/s)
    int64_t H_A;                  // AuxPoW hash rate (H/s)
    int64_t c_N;                  // Native cost per hash (ASU/H)
    int64_t c_A;                  // AuxPoW cost per hash (ASU/H)
    int64_t F;                    // Fee level (ASU/block)
    int64_t B;                    // Block reward (ASU/block)
    double alpha_A;               // AuxPoW availability [0,1]
    double lambda_A;              // AuxPoW rentability [0,1]
    double rho_pool;              // Pool concentration [0,1]
    double rho_geo;               // Geographic concentration [0,1]
    double gamma_NA;              // Native-AuxPoW correlation [-1,1]
    int N_chains;                 // Chain count [1,10]
    
    // Shock parameters (§3.6)
    bool has_shock = false;
    int64_t t_shock = 0;          // Shock timing (block)
    int64_t D_i_shock = 0;        // Demand shock magnitude
    int64_t H_A_shock = 0;        // AuxPoW hash shock magnitude
    double alpha_A_shock = 0;     // AuxPoW availability shock
    
    // Derived (computed at construction)
    int64_t V_i;                  // Volume = D_i * v_per_tx
    int64_t X_i;                  // Inter-chain traffic
    double rho_i;                 // Concentration = max(rho_pool, rho_geo)
    
    // Metadata
    std::vector<std::string> hypotheses_tested;  // e.g., ["MSF-H1", "MSF-H3"]
    
    // Construct with derived values
    ScenarioParams() = default;
    
    void ComputeDerived() {
        V_i = D_i * 50;  // v_per_tx = 50
        rho_i = (rho_pool > rho_geo) ? rho_pool : rho_geo;
        if (scenario_id == "MSF-S14") {
            X_i = D_i * 0.3;
        } else {
            X_i = 0;
        }
    }
};

// ============================================================================
// Per-Block Observations (raw observations for evidence)
// ============================================================================

struct BlockObservation {
    int64_t block_number;
    int64_t chain_id;
    
    // Independent variables at this block (post-shock if applicable)
    int64_t D_i;
    int64_t H_N;
    int64_t H_A;
    int64_t F;
    int64_t B;
    double alpha_A;
    double lambda_A;
    double rho_pool;
    double rho_geo;
    double gamma_NA;
    int N_chains;
    
    // Controlled constants
    int64_t c_N;
    int64_t c_A;
    int64_t v_per_tx;
    int64_t T_block;
    
    // Computed metrics (Q32.32 fixed-point hex strings)
    std::string Pi_i;         // Profitability
    std::string S_i;          // Effective Security
    std::string S_i_req;      // Required Security
    std::string B_fork_i;     // Fork Budget
    std::string B_min_i;      // Min Fork Budget
    std::string rho_i;        // Concentration
    std::string CE_i;         // Security Capital Efficiency
    bool Viable_i;            // Viability
    
    // Anomaly counters
    int overflow_count = 0;
    int div_zero_count = 0;
    std::string replication_max_delta;
    std::string anomaly_notes;
};

// ============================================================================
// Scenario Replication Result
// ============================================================================

struct ReplicationResult {
    int replication_id;           // 1, 2, or 3
    std::vector<BlockObservation> blocks;
    std::string replication_max_delta;  // Max divergence from other replications
    bool bit_identical_to_replication_1 = false;
};

// ============================================================================
// Scenario Execution Result
// ============================================================================

struct ScenarioResult {
    std::string scenario_id;
    std::vector<ReplicationResult> replications;  // Size = 3
    bool all_replications_bit_identical = false;
    
    // Hypothesis evidence
    struct HypothesisEvidence {
        std::string hypothesis;  // MSF-H1, MSF-H2, MSF-H3, MSF-H4
        std::string verdict;     // SUPPORTED, FALSIFIED, INCONCLUSIVE, NOT_EVALUABLE
        std::string evidence;
        std::vector<std::string> supporting_scenarios;
        std::vector<std::string> falsifying_scenarios;
    };
    std::vector<HypothesisEvidence> hypothesis_evidence;
    
    // Completeness checks
    struct CompletenessCheck {
        bool C1_all_scenarios_executed = false;
        bool C2_three_replications = false;
        bool C3_no_parameter_substitution = false;
        bool C4_output_schema_valid = false;
        bool C5_viability_check_present = false;
        bool C6_deterministic_replay = false;
        bool C7_computable_metric = false;
        bool C8_hypothesis_verdicts = false;
        bool C9_non_trivial = false;
    } completeness;
};

// ============================================================================
// Scenario Engine Interface
// ============================================================================

class ScenarioEngine {
public:
    ScenarioEngine();
    
    // Load frozen scenario matrix from SPEC-3 §4.1
    bool LoadFrozenScenarios();
    
    // Execute all 18 scenarios (build executable, don't run experiment)
    bool BuildExecutionPlan();
    
    // Get scenario by ID
    const ScenarioParams* GetScenario(const std::string& id) const;
    
    // Get all scenarios
    const std::vector<ScenarioParams>& GetAllScenarios() const;
    
    // Validate scenario parameters against frozen SPEC-3
    bool ValidateScenario(const ScenarioParams& scenario) const;
    
    // Generate execution plan for evidence manifest
    std::string GenerateExecutionPlanJSON() const;
    
    // Validate execution plan against SPEC-3 completeness (C1-C9)
    bool ValidateCompleteness(const ScenarioResult& result, 
                              std::vector<std::string>& errors) const;

private:
    std::unordered_map<std::string, ScenarioParams> scenarios_;
    std::vector<ScenarioParams> scenario_list_;
    
    // Initialize the 18 frozen scenarios from SPEC-3 §4.1
    void InitializeFrozenScenarios();
    
    // Apply shock at block boundary
    void ApplyShock(ScenarioParams& scenario, int64_t block_num);
    
    // Validate single parameter against frozen value
    bool ValidateParameter(const std::string& param_name,
                           const std::string& expected,
                           const std::string& actual) const;
};

} // namespace msf

#endif // MSF_SCENARIO_ENGINE_H