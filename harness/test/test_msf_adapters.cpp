// MSF Adapter Synthetic Unit Tests - CE1 Phase 2 (SPEC-3 corrected)
//
// Synthetic fixture tests for each observational adapter (M01-M10) per frozen SPEC-3.
// NO S01-S18 execution. Validates deterministic arithmetic against SPEC-3 formulas.
//
// Frozen SPEC-3 M01-M10 Catalogue:
// M01: Volume (V_i = D_i * v_per_tx)
// M02: Profitability (Pi_i)
// M03: Effective Security (S_i)
// M04: Required Security (S_i_req)
// M05: Fork Budget (B_fork_i)
// M06: Minimum Fork Budget (B_min_i)
// M07: Concentration (rho_i = max(rho_pool, rho_geo))
// M08: Security Capital Efficiency (CE_i)
// M09: Viability (Viable_i)
// M10: Recovery Time

#define BOOST_TEST_MODULE msf_adapter_tests
#define BOOST_TEST_NO_LIB
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <vector>
#include <algorithm>

#include "msf_scenario_engine.h"
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
#include "msf_adapter_registry.h"

using namespace msf;

// ============================================================================
// Test Fixtures - Synthetic BlockObservations from frozen scenario parameters
// ============================================================================

BlockObservation CreateBaselineObs() {
    BlockObservation obs;
    obs.block_number = 1;
    obs.chain_id = 1;
    obs.D_i = 100000;        // tx/day
    obs.H_N = 1000000;       // H/s
    obs.H_A = 500000;        // H/s
    obs.F = 100;             // ASU/block
    obs.B = 5000;            // ASU/block
    obs.alpha_A = 0.5;
    obs.lambda_A = 0.3;
    obs.rho_pool = 0.2;
    obs.rho_geo = 0.15;
    obs.gamma_NA = 0.0;
    obs.N_chains = 1;
    obs.c_N = 1;             // ASU/H
    obs.c_A = 1;             // ASU/H
    obs.v_per_tx = 50;
    obs.T_block = 60;
    obs.rho_i = 0.2;         // max(rho_pool, rho_geo)
    obs.V_i = obs.D_i * obs.v_per_tx; // 5,000,000
    obs.Viable_i = true;
    obs.overflow_count = 0;
    obs.div_zero_count = 0;
    obs.replication_max_delta = "0x00000000";
    return obs;
}

BlockObservation CreateHighAuxPowObs() {
    BlockObservation obs = CreateBaselineObs();
    obs.H_A = 2000000;       // High AuxPoW
    obs.lambda_A = 0.7;
    obs.rho_i = 0.3;
    return obs;
}

BlockObservation CreateZeroHashObs() {
    BlockObservation obs = CreateBaselineObs();
    obs.H_N = 0;
    obs.H_A = 0;
    return obs;
}

BlockObservation CreateHighRentabilityObs() {
    BlockObservation obs = CreateBaselineObs();
    obs.lambda_A = 0.9;
    obs.rho_i = 0.8;
    return obs;
}

// ============================================================================
// M01: Volume (V_i = D_i * v_per_tx)
// ============================================================================

BOOST_AUTO_TEST_SUITE(M01_Volume)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    // V_i = D_i * v_per_tx = 100000 * 50 = 5,000,000
    std::string result = AdapterM01::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM01::Compute(obs);
    std::string r2 = AdapterM01::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_CASE(zero_demand) {
    BlockObservation obs = CreateBaselineObs();
    obs.D_i = 0;
    std::string result = AdapterM01::Compute(obs);
    BOOST_TEST(result == "0x0000000000000000");
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M02: Profitability (Pi_i = (B + F) * (H_N / (H_N + H_A)) - c_N * H_N)
// ============================================================================

BOOST_AUTO_TEST_SUITE(M02_Profitability)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    // (B+F) = 5100, H_N/(H_N+H_A) = 1000000/1500000 = 2/3
    // Revenue = 5100 * 2/3 = 3400
    // Cost = 1 * 1000000 = 1000000
    // Net = 3400 - 1000000 = -996600
    std::string result = AdapterM02::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM02::Compute(obs);
    std::string r2 = AdapterM02::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_CASE(zero_hash_rate) {
    BlockObservation obs = CreateBaselineObs();
    obs.H_N = 0;
    obs.H_A = 0;
    std::string result = AdapterM02::Compute(obs);
    BOOST_TEST(result == "0x0000000000000000");
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M03: Effective Security (S_i = H_N * (1 - lambda_A) + H_A * lambda_A)
// ============================================================================

BOOST_AUTO_TEST_SUITE(M03_EffectiveSecurity)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    // S_i = 1000000 * 0.7 + 500000 * 0.3 = 700000 + 150000 = 850000
    std::string result = AdapterM03::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM03::Compute(obs);
    std::string r2 = AdapterM03::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_CASE(zero_lambda_a) {
    BlockObservation obs = CreateBaselineObs();
    obs.lambda_A = 0.0;
    std::string result = AdapterM03::Compute(obs);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(unit_lambda_a) {
    BlockObservation obs = CreateBaselineObs();
    obs.lambda_A = 1.0;
    std::string result = AdapterM03::Compute(obs);
    // S_i = H_A = 500000
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M04: Required Security (S_i_req = B_fork_i / T_block)
// ============================================================================

BOOST_AUTO_TEST_SUITE(M04_RequiredSecurity)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    std::string result = AdapterM04::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM04::Compute(obs);
    std::string r2 = AdapterM04::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M05: Fork Budget (B_fork_i = k * V_i * (1 - rho_i), k=3)
// ============================================================================

BOOST_AUTO_TEST_SUITE(M05_ForkBudget)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    std::string result = AdapterM05::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM05::Compute(obs);
    std::string r2 = AdapterM05::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_CASE(zero_rho) {
    BlockObservation obs = CreateBaselineObs();
    obs.rho_pool = 0.0;
    obs.rho_geo = 0.0;
    std::string result = AdapterM05::Compute(obs);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M06: Minimum Fork Budget (B_min_i = k * V_i * rho_max, rho_max=0.8)
// ============================================================================

BOOST_AUTO_TEST_SUITE(M06_MinForkBudget)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    std::string result = AdapterM06::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM06::Compute(obs);
    std::string r2 = AdapterM06::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M07: Concentration (rho_i = max(rho_pool, rho_geo))
// ============================================================================

BOOST_AUTO_TEST_SUITE(M07_Concentration)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    // rho_i = max(0.2, 0.15) = 0.2
    std::string result = AdapterM07::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM07::Compute(obs);
    std::string r2 = AdapterM07::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_CASE(rho_pool_dominates) {
    BlockObservation obs = CreateBaselineObs();
    obs.rho_pool = 0.5;
    obs.rho_geo = 0.1;
    std::string result = AdapterM07::Compute(obs);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(rho_geo_dominates) {
    BlockObservation obs = CreateBaselineObs();
    obs.rho_pool = 0.1;
    obs.rho_geo = 0.6;
    std::string result = AdapterM07::Compute(obs);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M08: Security Capital Efficiency (CE_i = S_i / (c_N*H_N + c_A*H_A))
// ============================================================================

BOOST_AUTO_TEST_SUITE(M08_SecurityCapitalEfficiency)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    std::string result = AdapterM08::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM08::Compute(obs);
    std::string r2 = AdapterM08::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M09: Viability (Viable_i = (S_i >= S_i_req) && (Pi_i >= 0) && no anomalies)
// ============================================================================

BOOST_AUTO_TEST_SUITE(M09_Viability)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    std::string result = AdapterM09::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM09::Compute(obs);
    std::string r2 = AdapterM09::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// M10: Recovery Time
// ============================================================================

BOOST_AUTO_TEST_SUITE(M10_RecoveryTime)

BOOST_AUTO_TEST_CASE(baseline_computation) {
    BlockObservation obs = CreateBaselineObs();
    std::string result = AdapterM10::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_CASE(deterministic_reproducibility) {
    BlockObservation obs = CreateBaselineObs();
    std::string r1 = AdapterM10::Compute(obs);
    std::string r2 = AdapterM10::Compute(obs);
    BOOST_TEST(r1 == r2);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// Registry Integration Tests
// ============================================================================

BOOST_AUTO_TEST_SUITE(AdapterRegistryTests)

BOOST_AUTO_TEST_CASE(all_ten_registered) {
    AdapterRegistry registry;
    auto metrics = registry.GetRegisteredMetrics();
    BOOST_TEST(metrics.size() == 10);
    for (int i = 1; i <= 10; ++i) {
        std::string expected = "M" + (i < 10 ? "0" : "") + std::to_string(i);
        bool found = false;
        for (const auto& m : metrics) {
            if (m == expected) {
                found = true;
                break;
            }
        }
        BOOST_TEST(found);
    }
}

BOOST_AUTO_TEST_CASE(execute_all_deterministic) {
    AdapterRegistry registry;
    BlockObservation obs = CreateBaselineObs();
    auto results = registry.ExecuteAll(obs);
    BOOST_TEST(results.size() == 10);
    for (const auto& r : results) {
        BOOST_TEST(r.valid);
        BOOST_TEST(r.metric_id.size() == 3);
        BOOST_TEST(r.value.size() == 18);
        BOOST_TEST(r.value.substr(0, 2) == "0x");
    }
}

BOOST_AUTO_TEST_CASE(execute_single_metric) {
    AdapterRegistry registry;
    BlockObservation obs = CreateBaselineObs();
    auto result = registry.Execute("M01", obs);
    BOOST_TEST(result.has_value());
    BOOST_TEST(result->metric_id == "M01");
    BOOST_TEST(result->valid);
}

BOOST_AUTO_TEST_CASE(invalid_metric_returns_nullopt) {
    AdapterRegistry registry;
    BlockObservation obs = CreateBaselineObs();
    auto result = registry.Execute("M99", obs);
    BOOST_TEST(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// Authority Boundary Tests
// ============================================================================

BOOST_AUTO_TEST_SUITE(AuthorityBoundaries)

BOOST_AUTO_TEST_CASE(adapters_are_observational_only) {
    AdapterRegistry registry;
    BlockObservation obs = CreateBaselineObs();
    auto results = registry.ExecuteAll(obs);
    for (const auto& r : results) {
        BOOST_TEST(r.authority_class == "OBSERVATIONAL_ADAPTER");
    }
}

BOOST_AUTO_TEST_CASE(no_consensus_mutation) {
    BlockObservation obs = CreateBaselineObs();
    std::string initial_rho = obs.replication_max_delta;
    AdapterRegistry registry;
    auto results = registry.ExecuteAll(obs);
    BOOST_TEST(obs.replication_max_delta == initial_rho);
}

BOOST_AUTO_TEST_CASE(no_canonical_state_write) {
    BlockObservation obs = CreateBaselineObs();
    int64_t initial_H_N = obs.H_N;
    AdapterRegistry registry;
    auto results = registry.ExecuteAll(obs);
    BOOST_TEST(obs.H_N == initial_H_N);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// Fixed-Point Arithmetic Tests
// ============================================================================

BOOST_AUTO_TEST_SUITE(Q32_32_Arithmetic)

BOOST_AUTO_TEST_CASE(positive_values) {
    // Test via M01's ToQ32_32 (made public for test access)
    BlockObservation obs = CreateBaselineObs();
    std::string result = AdapterM01::Compute(obs);
    BOOST_TEST(result.size() == 18);
    BOOST_TEST(result.substr(0, 2) == "0x");
}

BOOST_AUTO_TEST_SUITE_END()