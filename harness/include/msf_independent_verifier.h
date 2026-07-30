// MSF Independent Verifier (H8) - CE1 Phase 1.7
//
// Independent conformance verification with no shared implementation.
// Authority class: EVIDENCE_OUTPUT (can PROMOTE to integration queue)

#ifndef MSF_INDEPENDENT_VERIFIER_H
#define MSF_INDEPENDENT_VERIFIER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include "msf_scenario_engine.h"
#include "msf_replication_framework.h"
#include "msf_evidence_emission.h"

namespace msf {

// ============================================================================
// H8: Independent Conformance Verifier
// ============================================================================

struct VerificationConfig {
    bool verify_contract_conformance = true;
    bool verify_determinism = true;
    bool verify_replayability = true;
    bool verify_telemetry = true;
    bool verify_mutation_sensitivity = true;
    bool verify_evidence_integrity = true;
    bool verify_independent_implementation = true;
    
    double determinism_tolerance = 0.0;  // Bit-identical
    double replay_tolerance = 0.0;
    double mutation_sensitivity_threshold = 0.01;
};

struct VerificationResult {
    std::string task_id;
    bool overall_pass = false;
    
    struct CheckResult {
        std::string check_name;
        bool passed = false;
        std::string details;
        std::string evidence_hash;
    };
    std::vector<CheckResult> checks;
    
    std::string independent_implementation_hash;
    std::string verification_report;
    bool can_promote = false;
};

class IndependentVerifier {
public:
    explicit IndependentVerifier(const VerificationConfig& config = VerificationConfig());
    
    // ========================================================================
    // Main Verification Entry Points
    // ========================================================================
    
    // Full contract conformance verification
    VerificationResult VerifyContractConformance(
        const std::string& contract_spec_path,
        const std::string& implementation_path,
        const std::string& evidence_manifest_path);
    
    // Determinism verification (bit-identical replication)
    VerificationResult VerifyDeterminism(
        const ScenarioParams& scenario,
        const std::function<void(const ScenarioParams&, int, DeterministicRNG&, std::vector<std::string>&)>& block_fn);
    
    // Replayability verification
    VerificationResult VerifyReplayability(
        const EvidenceManifest& manifest,
        const std::string& implementation_path);
    
    // Telemetry availability and quality
    VerificationResult VerifyTelemetry(
        const std::string& telemetry_manifest_path);
    
    // Mutation sensitivity
    VerificationResult VerifyMutationSensitivity(
        const std::string& metric,
        const ExperimentResults& baseline);
    
    // Evidence integrity (hash chain, schema, completeness)
    VerificationResult VerifyEvidenceIntegrity(
        const EvidenceManifest& manifest);
    
    // Independent implementation verification
    VerificationResult VerifyIndependentImplementation(
        const std::string& reference_impl_path,
        const std::string& candidate_impl_path,
        const ScenarioParams& test_scenario);
    
    // Full verification suite
    VerificationResult RunFullVerificationSuite(
        const std::string& contract_spec_path,
        const std::string& implementation_path,
        const std::string& evidence_manifest_path);

private:
    VerificationConfig config_;
    
    // Individual check implementations
    struct Check {
        std::string name;
        std::function<bool()> fn;
    };
    std::vector<Check> checks_;
    
    // Helper methods
    bool RunCheck(VerificationResult::CheckResult& result);
    bool VerifySchema(const EvidenceManifest& manifest, const std::string& schema_name) const;
    bool VerifyHashChain(const EvidenceManifest& manifest) const;
    bool VerifyCompleteness(const EvidenceManifest& manifest) const;
    bool VerifyAnomalyFields(const EvidenceManifest& manifest) const;
    bool CompareOutputs(const std::string& expected, const std::string& actual, double tolerance) const;
    std::string ComputeFileHash(const std::string& filepath) const;
    std::string ComputeDirectoryHash(const std::string& dirpath) const;
    
    // Independent implementation verification
    bool VerifyBytecodeDivergence(const std::string& path1, const std::string& path2) const;
    bool VerifySourceDivergence(const std::string& path1, const std::string& path2) const;
    bool VerifyNoSharedDependencies(const std::string& path1, const std::string& path2) const;
    
    // Determinism verification
    bool VerifyReplicationBitIdentical(
        const std::vector<ReplicationResult>& results,
        const std::string& metric) const;
    
    // Replay verification
    bool ReplayFromManifest(const EvidenceManifest& manifest, const std::string& impl_path) const;
    
    // Report generation
    std::string GenerateVerificationReport(const VerificationResult& result) const;
};

// ============================================================================
// Independent Implementation Verification
// ============================================================================

class ImplementationComparator {
public:
    struct ComparisonResult {
        bool bytecode_divergent = false;
        bool source_divergent = false;
        bool no_shared_dependencies = true;
        std::string divergence_details;
    };
    
    static ComparisonResult Compare(
        const std::string& reference_path,
        const std::string& candidate_path,
        const std::vector<std::string>& excluded_paths = {});
    
private:
    static bool HasCommonDependencies(const std::string& path1, const std::string& path2);
    static bool FilesAreIdentical(const std::string& file1, const std::string& file2);
    static std::vector<std::string> ListSourceFiles(const std::string& dir);
    static std::vector<std::string> ListBinaryFiles(const std::string& dir);
    static bool IsTestFile(const std::string& path);
    static bool IsGeneratedFile(const std::string& path);
};

} // namespace msf

#endif // MSF_INDEPENDENT_VERIFIER_H