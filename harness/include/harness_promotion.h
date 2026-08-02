#ifndef HARNESS_PROMOTION_H
#define HARNESS_PROMOTION_H

#include <string>
#include <vector>
#include <unordered_map>
#include "harness_authority.h"
#include "harness_dag.h"
#include "harness_evidence.h"

namespace harness {

// ============================================================================
// O5: Promotion Validator
//
// Deterministically decides integration-queue eligibility based on:
// - Authority envelope validity
// - Dependency status
// - Mutation scope validity
// - Test pass
// - Negative control pass
// - Evidence manifest validity
// - Immutable input match
// ============================================================================

enum class PromotionVerdict {
    ELIGIBLE,           // Ready for integration
    NOT_ELIGIBLE,       // Not ready (specify reason)
    BLOCKED,            // Blocked by dependency
    INVALID             // Invalid state
};

struct PromotionCheck {
    std::string check_name;
    bool passed;
    std::string details;
    std::string failure_reason;
};

struct PromotionResult {
    PromotionVerdict verdict;
    std::vector<PromotionCheck> checks;
    std::string summary;
    std::string timestamp;
};

class PromotionValidator {
public:
    PromotionValidator(
        const DependencyDAG& dag,
        const EvidenceManager& evidence_manager
    );
    
    // Validate promotion for a specific task
    PromotionResult ValidatePromotion(
        const std::string& task_id,
        const AuthorityEnvelope& authority,
        const EvidenceManifest& evidence
    );
    
    // Validate all prerequisites for promotion
    bool ValidatePrerequisites(
        const std::string& task_id,
        const AuthorityEnvelope& authority,
        const EvidenceManifest& evidence
    );
    
    // Check authority envelope validity
    PromotionCheck CheckAuthorityEnvelope(
        const AuthorityEnvelope& authority
    );
    
    // Check dependency status
    PromotionCheck CheckDependencyStatus(
        const std::string& task_id
    );
    
    // Check mutation scope validity
    PromotionCheck CheckMutationScope(
        const AuthorityEnvelope& authority,
        const EvidenceManifest& evidence
    );
    
    // Check test pass
    PromotionCheck CheckTestPass(
        const EvidenceManifest& evidence
    );
    
    // Check negative control pass
    PromotionCheck CheckNegativeControlPass(
        const EvidenceManifest& evidence
    );
    
    // Check evidence manifest validity
    PromotionCheck CheckEvidenceManifest(
        const EvidenceManifest& evidence
    );
    
    // Check immutable input match
    PromotionCheck CheckImmutableInputs(
        const AuthorityEnvelope& authority,
        const EvidenceManifest& evidence
    );
    
    // Check H_MSF match
    PromotionCheck CheckHMSFMatch(
        const AuthorityEnvelope& authority,
        const EvidenceManifest& evidence
    );
    
    // Generate promotion report
    std::string GenerateReport(const PromotionResult& result);
    
    // Get promotion statistics
    struct Statistics {
        size_t total_validations;
        size_t eligible_promotions;
        size_t blocked_promotions;
        size_t invalid_promotions;
    };
    Statistics GetStatistics() const;

private:
    const DependencyDAG& dag_;
    const EvidenceManager& evidence_manager_;
    
    // Statistics
    Statistics stats_;
    
    // Execute all checks
    std::vector<PromotionCheck> ExecuteAllChecks(
        const std::string& task_id,
        const AuthorityEnvelope& authority,
        const EvidenceManifest& evidence
    );
    
    // Determine verdict from checks
    PromotionVerdict DetermineVerdict(
        const std::vector<PromotionCheck>& checks
    );
    
    // Get current timestamp
    std::string GetCurrentTimestamp() const;
};

// ============================================================================
// H_complete Gate
//
// The harness completion predicate. Must be conjunctive:
// H_complete = C_contract ∧ D_deterministic ∧ R_replayable ∧ 
//               T_telemetry ∧ M_mutation ∧ E_evidence ∧ V_independent
// ============================================================================

struct HCompleteResult {
    bool C_contract;      // Harness conforms to frozen MSF contract
    bool D_deterministic; // Equivalent canonical inputs produce byte-equivalent outputs
    bool R_replayable;    // State transitions and evidence can be independently replayed
    bool T_telemetry;     // All required telemetry is available or correctly treated as missing
    bool M_mutation;      // Frozen adversarial/anti-gaming mutation primitives exist and detect invalid mutations
    bool E_evidence;      // Deterministic evidence manifests permit reproduction
    bool V_independent;   // H8 independently verifies the above properties
    
    bool IsComplete() const {
        return C_contract && D_deterministic && R_replayable &&
               T_telemetry && M_mutation && E_evidence && V_independent;
    }
    
    std::string GetMissingConjuncts() const {
        std::vector<std::string> missing;
        if (!C_contract) missing.push_back("C_contract");
        if (!D_deterministic) missing.push_back("D_deterministic");
        if (!R_replayable) missing.push_back("R_replayable");
        if (!T_telemetry) missing.push_back("T_telemetry");
        if (!M_mutation) missing.push_back("M_mutation");
        if (!E_evidence) missing.push_back("E_evidence");
        if (!V_independent) missing.push_back("V_independent");
        
        if (missing.empty()) return "ALL CONJUNCTS SATISFIED";
        
        std::string result = "MISSING: ";
        for (size_t i = 0; i < missing.size(); ++i) {
            result += missing[i];
            if (i < missing.size() - 1) result += ", ";
        }
        return result;
    }
};

class HCompleteValidator {
public:
    static HCompleteResult Validate(
        const DependencyDAG& dag,
        const std::vector<EvidenceManifest>& manifests
    );
    
    static bool ValidateCContract(
        const EvidenceManifest& harness_evidence,
        const std::string& H_MSF
    );
    
    static bool ValidateDDeterministic(
        const EvidenceManifest& harness_evidence
    );
    
    static bool ValidateRReplayable(
        const EvidenceManifest& harness_evidence
    );
    
    static bool ValidateTTelemetry(
        const EvidenceManifest& harness_evidence
    );
    
    static bool ValidateMMutation(
        const EvidenceManifest& harness_evidence
    );
    
    static bool ValidateEEvidence(
        const EvidenceManifest& harness_evidence
    );
    
    static bool ValidateVIndependent(
        const EvidenceManifest& h8_evidence,
        const std::vector<EvidenceManifest>& other_manifests
    );
    
    static std::string GenerateReport(const HCompleteResult& result);
};

} // namespace harness

#endif // HARNESS_PROMOTION_H
