// MSF Completeness Evaluation Engine (C1-C9) - CE1 Phase 1.8
//
// Evaluates experiment completeness per SPEC-3 §8.1.
// Authority class: EXPERIMENT_INFRASTRUCTURE

#ifndef MSF_COMPLETENESS_H
#define MSF_COMPLETENESS_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>

#include "msf_scenario_engine.h"
#include "msf_evidence_emission.h"

namespace msf {

// ============================================================================
// Completeness Check Definitions (SPEC-3 §8.1)
// ============================================================================

enum class CompletenessCheck {
    C1_ALL_SCENARIOS_EXECUTED,      // Count(ScenariosExecuted) == 18
    C2_ALL_REPLICATIONS_PRESENT,    // For each scenario: Count(Replications) == 3
    C3_NO_PARAMETER_SUBSTITUTION,   // Parameters match §4 exactly
    C4_OUTPUT_SCHEMA_VALID,         // All MSF-M01..M09 present and non-null
    C5_VIABILITY_CHECK_PRESENT,     // Viability check recorded per scenario
    C6_DETERMINISTIC_REPLAY,        // 3x bit-identical per scenario
    C7_COMPUTABLE_METRIC,           // ≥1 metric computable per scenario
    C8_HYPOTHESIS_VERDICTS_DERIVED, // Verdicts in {SUPPORTED, FALSIFIED, INCONCLUSIVE}
    C9_NON_TRIVIAL_EXPERIMENT       // ≥14/18 scenarios produce verdicts, ≥3 hypotheses have verdicts
};

struct CheckResult {
    CompletenessCheck check;
    bool passed;
    std::string details;
    std::string evidence_hash;
};

struct CompletenessResult {
    std::unordered_map<CompletenessCheck, CheckResult> results;
    bool overall_passed;
    std::string summary;
    std::vector<std::string> failures;
};

// ============================================================================
// Completeness Evaluator
// ============================================================================

class CompletenessEvaluator {
public:
    CompletenessEvaluator();
    
    // Evaluate all C1-C9 predicates
    CompletenessResult Evaluate(
        const ScenarioResult& experiment_result,
        const std::vector<ReplicationResult>& all_replications,
        const EvidenceManifest& manifest);
    
    // Individual check methods
    CheckResult CheckC1(const ScenarioResult& result) const;
    CheckResult CheckC2(const std::vector<ReplicationResult>& replications) const;
    CheckResult CheckC3(const ScenarioParams& scenario) const;
    CheckResult CheckC4(const EvidenceManifest& manifest) const;
    CheckResult CheckC5(const EvidenceManifest& manifest) const;
    CheckResult CheckC6(const std::vector<ReplicationResult>& replications) const;
    CheckResult CheckC7(const ScenarioResult& result) const;
    CheckResult CheckC8(const ScenarioResult& result) const;
    CheckResult CheckC9(const ScenarioResult& result) const;
    
    // Generate completeness report
    std::string GenerateReport(const CompletenessResult& result) const;

private:
    // Frozen scenario list from SPEC-3
    static constexpr int kRequiredScenarios = 18;
    static constexpr int kRequiredReplications = 3;
    
    // Hypothesis identifiers
    static constexpr const char* kHypotheses[4] = {"MSF-H1", "MSF-H2", "MSF-H3", "MSF-H4"};
    
    // Helper: Count scenarios with computable metrics
    int CountScenariosWithVerdicts(const ScenarioResult& result) const;
    
    // Helper: Count hypotheses with verdicts
    int CountHypothesesWithVerdicts(const ScenarioResult& result) const;
};

} // namespace msf

#endif // MSF_COMPLETENESS_H