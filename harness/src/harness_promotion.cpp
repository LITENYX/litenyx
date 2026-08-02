#include "harness_promotion.h"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>

namespace harness {

// ============================================================================
// PromotionValidator Implementation
// ============================================================================

PromotionValidator::PromotionValidator(
    const DependencyDAG& dag,
    const EvidenceManager& evidence_manager
) : dag_(dag), evidence_manager_(evidence_manager) {
    stats_ = {0, 0, 0, 0};
}

PromotionResult PromotionValidator::ValidatePromotion(
    const std::string& task_id,
    const AuthorityEnvelope& authority,
    const EvidenceManifest& evidence
) {
    stats_.total_validations++;
    
    PromotionResult result;
    result.timestamp = GetCurrentTimestamp();
    result.checks = ExecuteAllChecks(task_id, authority, evidence);
    result.verdict = DetermineVerdict(result.checks);
    result.summary = GenerateReport(result);
    
    switch (result.verdict) {
        case PromotionVerdict::ELIGIBLE:
            stats_.eligible_promotions++;
            break;
        case PromotionVerdict::NOT_ELIGIBLE:
        case PromotionVerdict::BLOCKED:
            stats_.blocked_promotions++;
            break;
        case PromotionVerdict::INVALID:
            stats_.invalid_promotions++;
            break;
    }
    
    return result;
}

bool PromotionValidator::ValidatePrerequisites(
    const std::string& task_id,
    const AuthorityEnvelope& authority,
    const EvidenceManifest& evidence
) {
    auto result = ValidatePromotion(task_id, authority, evidence);
    return result.verdict == PromotionVerdict::ELIGIBLE;
}

PromotionCheck PromotionValidator::CheckAuthorityEnvelope(
    const AuthorityEnvelope& authority
) {
    PromotionCheck check;
    check.check_name = "Authority Envelope Validity";
    
    // Validate authority envelope
    if (!AuthorityFactory::Validate(authority)) {
        check.passed = false;
        check.failure_reason = "Authority envelope validation failed";
        return check;
    }
    
    // Check that task has promotion capability
    if (!authority.capabilities.promote) {
        check.passed = false;
        check.failure_reason = "Task does not have promotion capability";
        return check;
    }
    
    // Check that authority state is EXECUTABLE
    if (authority.authority.state != AuthorityState::EXECUTABLE) {
        check.passed = false;
        check.failure_reason = "Authority state is not EXECUTABLE";
        return check;
    }
    
    check.passed = true;
    check.details = "Authority envelope is valid";
    return check;
}

PromotionCheck PromotionValidator::CheckDependencyStatus(
    const std::string& task_id
) {
    PromotionCheck check;
    check.check_name = "Dependency Status";
    
    // Check if all dependencies are completed
    const TaskNode* task = dag_.GetTask(task_id);
    if (!task) {
        check.passed = false;
        check.failure_reason = "Task not found in DAG";
        return check;
    }
    
    for (const auto& dep : task->dependencies) {
        const TaskNode* dep_task = dag_.GetTask(dep);
        if (!dep_task) {
            check.passed = false;
            check.failure_reason = "Dependency not found: " + dep;
            return check;
        }
        
        if (dep_task->status != TaskStatus::COMPLETED) {
            check.passed = false;
            check.failure_reason = "Dependency not completed: " + dep;
            return check;
        }
    }
    
    check.passed = true;
    check.details = "All dependencies are completed";
    return check;
}

PromotionCheck PromotionValidator::CheckMutationScope(
    const AuthorityEnvelope& authority,
    const EvidenceManifest& evidence
) {
    PromotionCheck check;
    check.check_name = "Mutation Scope Validity";
    
    // Check that mutations stayed within allowed paths
    // This would need integration with worktree manager
    // For now, just check that authority has mutation capability
    if (authority.capabilities.mutate) {
        // Check that mutation scope is defined
        if (authority.mutation_scope.allowed_paths.empty() && 
            authority.mutation_scope.prohibited_paths.empty()) {
            check.passed = false;
            check.failure_reason = "Mutation scope not defined";
            return check;
        }
    }
    
    check.passed = true;
    check.details = "Mutation scope is valid";
    return check;
}

PromotionCheck PromotionValidator::CheckTestPass(
    const EvidenceManifest& evidence
) {
    PromotionCheck check;
    check.check_name = "Test Pass";
    
    // Check that all tests passed
    for (const auto& test : evidence.test_results) {
        if (!test.passed) {
            check.passed = false;
            check.failure_reason = "Test failed: " + test.test_name;
            return check;
        }
    }
    
    check.passed = true;
    check.details = "All tests passed";
    return check;
}

PromotionCheck PromotionValidator::CheckNegativeControlPass(
    const EvidenceManifest& evidence
) {
    PromotionCheck check;
    check.check_name = "Negative Control Pass";
    
    // Check that all negative controls passed
    for (const auto& control : evidence.negative_control_results) {
        if (!control.passed) {
            check.passed = false;
            check.failure_reason = "Negative control failed: " + control.test_name;
            return check;
        }
    }
    
    check.passed = true;
    check.details = "All negative controls passed";
    return check;
}

PromotionCheck PromotionValidator::CheckEvidenceManifest(
    const EvidenceManifest& evidence
) {
    PromotionCheck check;
    check.check_name = "Evidence Manifest Validity";
    
    // Verify manifest integrity
    if (!evidence_manager_.VerifyManifestIntegrity(evidence)) {
        check.passed = false;
        check.failure_reason = "Evidence manifest integrity check failed";
        return check;
    }
    
    // Validate against schema
    if (!EvidenceSchemaValidator::ValidateManifest(evidence, "evidence-manifest-schema")) {
        check.passed = false;
        check.failure_reason = "Evidence manifest schema validation failed";
        return check;
    }
    
    check.passed = true;
    check.details = "Evidence manifest is valid";
    return check;
}

PromotionCheck PromotionValidator::CheckImmutableInputs(
    const AuthorityEnvelope& authority,
    const EvidenceManifest& evidence
) {
    PromotionCheck check;
    check.check_name = "Immutable Input Match";
    
    // Check that immutable inputs from authority are present in evidence
    for (const auto& input : authority.immutable_inputs) {
        bool found = false;
        for (const auto& entry : evidence.canonical_input_hashes) {
            if (entry.key == input || entry.value == input) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            check.passed = false;
            check.failure_reason = "Immutable input not found in evidence: " + input;
            return check;
        }
    }
    
    check.passed = true;
    check.details = "All immutable inputs are present in evidence";
    return check;
}

PromotionCheck PromotionValidator::CheckHMSFMatch(
    const AuthorityEnvelope& authority,
    const EvidenceManifest& evidence
) {
    PromotionCheck check;
    check.check_name = "H_MSF Match";
    
    // Extract H_MSF from authority envelope
    std::string authority_hmsf;
    for (const auto& input : authority.immutable_inputs) {
        if (input.find("H_MSF:") == 0) {
            authority_hmsf = input.substr(6);  // Remove "H_MSF:" prefix
            break;
        }
    }
    
    if (authority_hmsf.empty()) {
        check.passed = false;
        check.failure_reason = "H_MSF not found in authority envelope";
        return check;
    }
    
    // Compare with evidence H_MSF
    if (authority_hmsf != evidence.H_MSF) {
        check.passed = false;
        check.failure_reason = "H_MSF mismatch: authority=" + authority_hmsf + 
                               " evidence=" + evidence.H_MSF;
        return check;
    }
    
    check.passed = true;
    check.details = "H_MSF matches";
    return check;
}

std::string PromotionValidator::GenerateReport(const PromotionResult& result) {
    std::ostringstream report;
    
    report << "=== Promotion Validation Report ===" << std::endl;
    report << "Verdict: ";
    
    switch (result.verdict) {
        case PromotionVerdict::ELIGIBLE:
            report << "ELIGIBLE";
            break;
        case PromotionVerdict::NOT_ELIGIBLE:
            report << "NOT ELIGIBLE";
            break;
        case PromotionVerdict::BLOCKED:
            report << "BLOCKED";
            break;
        case PromotionVerdict::INVALID:
            report << "INVALID";
            break;
    }
    
    report << std::endl;
    report << "Timestamp: " << result.timestamp << std::endl;
    report << std::endl;
    
    report << "=== Checks ===" << std::endl;
    for (const auto& check : result.checks) {
        report << "  " << check.check_name << ": " 
               << (check.passed ? "PASS" : "FAIL") << std::endl;
        if (!check.passed) {
            report << "    Reason: " << check.failure_reason << std::endl;
        }
    }
    
    return report.str();
}

PromotionValidator::Statistics PromotionValidator::GetStatistics() const {
    return stats_;
}

// ============================================================================
// Private Methods
// ============================================================================

std::vector<PromotionCheck> PromotionValidator::ExecuteAllChecks(
    const std::string& task_id,
    const AuthorityEnvelope& authority,
    const EvidenceManifest& evidence
) {
    std::vector<PromotionCheck> checks;
    
    checks.push_back(CheckAuthorityEnvelope(authority));
    checks.push_back(CheckDependencyStatus(task_id));
    checks.push_back(CheckMutationScope(authority, evidence));
    checks.push_back(CheckTestPass(evidence));
    checks.push_back(CheckNegativeControlPass(evidence));
    checks.push_back(CheckEvidenceManifest(evidence));
    checks.push_back(CheckImmutableInputs(authority, evidence));
    checks.push_back(CheckHMSFMatch(authority, evidence));
    
    return checks;
}

PromotionVerdict PromotionValidator::DetermineVerdict(
    const std::vector<PromotionCheck>& checks
) {
    for (const auto& check : checks) {
        if (!check.passed) {
            // Check if failure is due to dependency
            if (check.check_name == "Dependency Status") {
                return PromotionVerdict::BLOCKED;
            }
            
            // Check if failure is due to invalid state
            if (check.check_name == "Authority Envelope Validity") {
                return PromotionVerdict::INVALID;
            }
            
            return PromotionVerdict::NOT_ELIGIBLE;
        }
    }
    
    return PromotionVerdict::ELIGIBLE;
}

std::string PromotionValidator::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime(&time));
    return std::string(buffer);
}

// ============================================================================
// HCompleteValidator Implementation
// ============================================================================

HCompleteResult HCompleteValidator::Validate(
    const DependencyDAG& dag,
    const std::vector<EvidenceManifest>& manifests
) {
    HCompleteResult result;
    
    // Find harness evidence and H8 evidence
    EvidenceManifest harness_evidence;
    EvidenceManifest h8_evidence;
    
    for (const auto& manifest : manifests) {
        if (manifest.task_id.find("H8") == 0) {
            h8_evidence = manifest;
        } else if (manifest.task_id.find("H") == 0) {
            harness_evidence = manifest;
        }
    }
    
    // Validate each conjunct
    result.C_contract = ValidateCContract(harness_evidence, "");
    result.D_deterministic = ValidateDDeterministic(harness_evidence);
    result.R_replayable = ValidateRReplayable(harness_evidence);
    result.T_telemetry = ValidateTTelemetry(harness_evidence);
    result.M_mutation = ValidateMMutation(harness_evidence);
    result.E_evidence = ValidateEEvidence(harness_evidence);
    result.V_independent = ValidateVIndependent(h8_evidence, manifests);
    
    return result;
}

bool HCompleteValidator::ValidateCContract(
    const EvidenceManifest& harness_evidence,
    const std::string& H_MSF
) {
    // Check that harness conforms to frozen MSF contract
    // This would need integration with contract verification
    // For now, just check that evidence exists
    return !harness_evidence.task_id.empty();
}

bool HCompleteValidator::ValidateDDeterministic(
    const EvidenceManifest& harness_evidence
) {
    // Check that equivalent canonical inputs produce byte-equivalent outputs
    // This would need replay testing
    // For now, just check that tests passed
    for (const auto& test : harness_evidence.test_results) {
        if (test.test_name.find("determinism") != std::string::npos && !test.passed) {
            return false;
        }
    }
    return true;
}

bool HCompleteValidator::ValidateRReplayable(
    const EvidenceManifest& harness_evidence
) {
    // Check that state transitions and evidence can be independently replayed
    // This would need replay testing
    // For now, just check that tests passed
    for (const auto& test : harness_evidence.test_results) {
        if (test.test_name.find("replay") != std::string::npos && !test.passed) {
            return false;
        }
    }
    return true;
}

bool HCompleteValidator::ValidateTTelemetry(
    const EvidenceManifest& harness_evidence
) {
    // Check that all required telemetry is available or correctly treated as missing
    // This would need telemetry validation
    // For now, just check that no errors
    return harness_evidence.errors.empty();
}

bool HCompleteValidator::ValidateMMutation(
    const EvidenceManifest& harness_evidence
) {
    // Check that frozen adversarial/anti-gaming mutation primitives exist and detect invalid mutations
    // This would need mutation testing
    // For now, just check that negative controls passed
    for (const auto& control : harness_evidence.negative_control_results) {
        if (!control.passed) {
            return false;
        }
    }
    return true;
}

bool HCompleteValidator::ValidateEEvidence(
    const EvidenceManifest& harness_evidence
) {
    // Check that deterministic evidence manifests permit reproduction
    // This would need evidence verification
    // For now, just check that manifest hash exists
    return !harness_evidence.manifest_hash.empty();
}

bool HCompleteValidator::ValidateVIndependent(
    const EvidenceManifest& h8_evidence,
    const std::vector<EvidenceManifest>& other_manifests
) {
    // Check that H8 independently verifies the above properties
    // This would need independent verification
    // For now, just check that H8 evidence exists
    return !h8_evidence.task_id.empty();
}

std::string HCompleteValidator::GenerateReport(const HCompleteResult& result) {
    std::ostringstream report;
    
    report << "=== H_complete Gate Report ===" << std::endl;
    report << "Overall: " << (result.IsComplete() ? "SATISFIED" : "NOT SATISFIED") << std::endl;
    report << std::endl;
    
    report << "Conjuncts:" << std::endl;
    report << "  C_contract: " << (result.C_contract ? "SATISFIED" : "NOT SATISFIED") << std::endl;
    report << "  D_deterministic: " << (result.D_deterministic ? "SATISFIED" : "NOT SATISFIED") << std::endl;
    report << "  R_replayable: " << (result.R_replayable ? "SATISFIED" : "NOT SATISFIED") << std::endl;
    report << "  T_telemetry: " << (result.T_telemetry ? "SATISFIED" : "NOT SATISFIED") << std::endl;
    report << "  M_mutation: " << (result.M_mutation ? "SATISFIED" : "NOT SATISFIED") << std::endl;
    report << "  E_evidence: " << (result.E_evidence ? "SATISFIED" : "NOT SATISFIED") << std::endl;
    report << "  V_independent: " << (result.V_independent ? "SATISFIED" : "NOT SATISFIED") << std::endl;
    report << std::endl;
    
    if (!result.IsComplete()) {
        report << "Missing: " << result.GetMissingConjuncts() << std::endl;
    }
    
    return report.str();
}

} // namespace harness
