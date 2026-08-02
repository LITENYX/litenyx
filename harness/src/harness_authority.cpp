#include "harness_authority.h"
#include <algorithm>
#include <stdexcept>

namespace harness {

// ============================================================================
// AuthorityFactory Implementation
// ============================================================================

AuthorityEnvelope AuthorityFactory::CreateH1() {
    AuthorityEnvelope env;
    env.task_id = "H1-canonical-state-generator";
    env.operation_class = "state-generation";
    env.authority.source = "ORCH-HARNESS-BUILD-1";
    env.authority.state = AuthorityState::EXECUTABLE;
    env.capabilities.discover = true;
    env.capabilities.mutate = false;  // State generation is read-only
    env.capabilities.promote = false;
    env.allowed_operations = {
        "generate-genesis-state",
        "generate-block-state",
        "generate-utxo-set",
        "generate-work-source-composition"
    };
    env.prohibited_operations = {
        "modify-consensus-parameters",
        "modify-activation-state",
        "modify-mempool"
    };
    env.immutable_inputs = {
        "H_MSF:fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc",
        "SPEC-WORK-ADAPTER-1",
        "CONSENSUS-ARITHMETIC-1"
    };
    env.dependencies = {};
    env.mutation_scope.worktree = "";
    env.mutation_scope.allowed_paths = {};
    env.mutation_scope.prohibited_paths = {"*"};  // No mutations allowed
    env.evidence_requirements.required_evidence = {
        "state-hash",
        "utxo-root",
        "work-source-composition"
    };
    env.evidence_requirements.evidence_manifest_schema = "state-generation-manifest";
    env.promotion_gate.required_passes = {
        "state-determinism-test",
        "state-reproducibility-test"
    };
    env.promotion_gate.required_controls = {};
    env.invalidation_conditions = {
        "non-deterministic-state-generation",
        "utxo-set-corruption",
        "work-source-mismatch"
    };
    return env;
}

AuthorityEnvelope AuthorityFactory::CreateH2() {
    AuthorityEnvelope env;
    env.task_id = "H2-nativepow-auxpow-executor";
    env.operation_class = "work-execution";
    env.authority.source = "ORCH-HARNESS-BUILD-1";
    env.authority.state = AuthorityState::EXECUTABLE;
    env.capabilities.discover = true;
    env.capabilities.mutate = true;  // Executor binaries
    env.capabilities.promote = false;
    env.allowed_operations = {
        "execute-nativepow-validation",
        "execute-auxpow-validation",
        "execute-carrier-validation",
        "compute-work-hash",
        "verify-work-validity"
    };
    env.prohibited_operations = {
        "modify-consensus-rules",
        "modify-validation-logic"
    };
    env.immutable_inputs = {
        "H_MSF:fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc",
        "SPEC-WORK-ADAPTER-1",
        "test_work_adapter_eng1.exe"
    };
    env.dependencies = {"H1-canonical-state-generator"};
    env.mutation_scope.worktree = "h2-executor-worktree";
    env.mutation_scope.allowed_paths = {
        "harness/src/h2_*.cpp",
        "harness/src/h2_*.h",
        "harness/test/h2_*.cpp"
    };
    env.mutation_scope.prohibited_paths = {
        "litenyx/litenyx/*.h",
        "litenyx/cpp_reference/test/*.cpp"
    };
    env.evidence_requirements.required_evidence = {
        "validation-result",
        "work-hash",
        "target-comparison"
    };
    env.evidence_requirements.evidence_manifest_schema = "work-execution-manifest";
    env.promotion_gate.required_passes = {
        "49-work-adapter-tests",
        "reproducibility-test"
    };
    env.promotion_gate.required_controls = {
        "invalid-work-injection",
        "malformed-header-injection"
    };
    env.invalidation_conditions = {
        "non-deterministic-validation",
        "consensus-rule-violation",
        "incorrect-accepted-work-accounting"
    };
    return env;
}

AuthorityEnvelope AuthorityFactory::CreateH3() {
    AuthorityEnvelope env;
    env.task_id = "H3-fork-reorg-displacement-engine";
    env.operation_class = "fork-simulation";
    env.authority.source = "ORCH-HARNESS-BUILD-1";
    env.authority.state = AuthorityState::EXECUTABLE;
    env.capabilities.discover = true;
    env.capabilities.mutate = true;
    env.capabilities.promote = false;
    env.allowed_operations = {
        "create-competing-histories",
        "simulate-reorganization",
        "simulate-displacement",
        "compute-reorg-depth",
        "compute-displacement-cost"
    };
    env.prohibited_operations = {
        "modify-fork-choice-rules",
        "modify-daAlgorithm"
    };
    env.immutable_inputs = {
        "H_MSF:fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc",
        "SPEC-WORK-ADAPTER-1",
        "FORK-AUTHORITY-1"
    };
    env.dependencies = {
        "H1-canonical-state-generator",
        "H2-nativepow-auxpow-executor"
    };
    env.mutation_scope.worktree = "h3-fork-engine-worktree";
    env.mutation_scope.allowed_paths = {
        "harness/src/h3_*.cpp",
        "harness/src/h3_*.h",
        "harness/test/h3_*.cpp"
    };
    env.mutation_scope.prohibited_paths = {
        "litenyx/litenyx/*.h",
        "litenyx/cpp_reference/test/*.cpp"
    };
    env.evidence_requirements.required_evidence = {
        "competing-history-hashes",
        "reorg-depth",
        "displacement-cost",
        "fork-winner"
    };
    env.evidence_requirements.evidence_manifest_schema = "fork-simulation-manifest";
    env.promotion_gate.required_passes = {
        "reorg-consistency-test",
        "displacement-cost-test",
        "fork-choice-test"
    };
    env.promotion_gate.required_controls = {
        "wrong-fork-winner-injection",
        "incorrect-disconnect-state-injection"
    };
    env.invalidation_conditions = {
        "non-deterministic-fork-behavior",
        "incorrect-reorg-depth",
        "utxo-rollback-corruption"
    };
    return env;
}

AuthorityEnvelope AuthorityFactory::CreateH4() {
    AuthorityEnvelope env;
    env.task_id = "H4-rentability-external-telemetry";
    env.operation_class = "telemetry-collection";
    env.authority.source = "ORCH-HARNESS-BUILD-1";
    env.authority.state = AuthorityState::PREPARATION_ONLY;  // No mutation
    env.capabilities.discover = true;
    env.capabilities.mutate = false;
    env.capabilities.promote = false;
    env.allowed_operations = {
        "collect-hash-rental-rates",
        "collect-exchange-rates",
        "collect-bandwidth-costs",
        "collect-capital-costs",
        "compute-rentability-discount"
    };
    env.prohibited_operations = {
        "modify-telemetry-sources",
        "modify-aggregation-rules"
    };
    env.immutable_inputs = {
        "H_MSF:fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc",
        "MSF-CONTRACT-FREEZE-1:telemetry-section"
    };
    env.dependencies = {"H1-canonical-state-generator"};
    env.mutation_scope.worktree = "";
    env.mutation_scope.allowed_paths = {};
    env.mutation_scope.prohibited_paths = {"*"};  // No mutations
    env.evidence_requirements.required_evidence = {
        "telemetry-sources",
        "aggregation-rules",
        "missing-data-treatment"
    };
    env.evidence_requirements.evidence_manifest_schema = "telemetry-manifest";
    env.promotion_gate.required_passes = {
        "telemetry-availability-test",
        "aggregation-consistency-test"
    };
    env.promotion_gate.required_controls = {
        "missing-telemetry-injection",
        "stale-telemetry-injection"
    };
    env.invalidation_conditions = {
        "non-canonical-telemetry-source",
        "incorrect-aggregation",
        "missing-data-without-conservative-default"
    };
    return env;
}

AuthorityEnvelope AuthorityFactory::CreateH5() {
    AuthorityEnvelope env;
    env.task_id = "H5-missing-telemetry-handler";
    env.operation_class = "missing-data-handling";
    env.authority.source = "ORCH-HARNESS-BUILD-1";
    env.authority.state = AuthorityState::PREPARATION_ONLY;
    env.capabilities.discover = true;
    env.capabilities.mutate = false;
    env.capabilities.promote = false;
    env.allowed_operations = {
        "detect-missing-telemetry",
        "apply-conservative-defaults",
        "document-missing-data"
    };
    env.prohibited_operations = {
        "modify-conservative-defaults",
        "modify-documentation-format"
    };
    env.immutable_inputs = {
        "H_MSF:fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc",
        "MSF-CONTRACT-FREEZE-1:missing-data-section"
    };
    env.dependencies = {"H4-rentability-external-telemetry"};
    env.mutation_scope.worktree = "";
    env.mutation_scope.allowed_paths = {};
    env.mutation_scope.prohibited_paths = {"*"};
    env.evidence_requirements.required_evidence = {
        "missing-data-report",
        "conservative-default-application"
    };
    env.evidence_requirements.evidence_manifest_schema = "missing-data-manifest";
    env.promotion_gate.required_passes = {
        "missing-data-detection-test",
        "conservative-default-test"
    };
    env.promotion_gate.required_controls = {};
    env.invalidation_conditions = {
        "incorrect-missing-data-detection",
        "non-conservative-default"
    };
    return env;
}

AuthorityEnvelope AuthorityFactory::CreateH6() {
    AuthorityEnvelope env;
    env.task_id = "H6-anti-gaming-executor";
    env.operation_class = "anti-gaming-testing";
    env.authority.source = "ORCH-HARNESS-BUILD-1";
    env.authority.state = AuthorityState::EXECUTABLE;
    env.capabilities.discover = true;
    env.capabilities.mutate = true;
    env.capabilities.promote = false;
    env.allowed_operations = {
        "apply-metric-mutations",
        "test-gameability",
        "compute-statistical-significance",
        "detect-gaming"
    };
    env.prohibited_operations = {
        "modify-anti-gaming-rules",
        "modify-significance-threshold"
    };
    env.immutable_inputs = {
        "H_MSF:fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc",
        "MSF-CONTRACT-FREEZE-1:anti-gaming-section"
    };
    env.dependencies = {
        "H2-nativepow-auxpow-executor",
        "H3-fork-reorg-displacement-engine"
    };
    env.mutation_scope.worktree = "h6-anti-gaming-worktree";
    env.mutation_scope.allowed_paths = {
        "harness/src/h6_*.cpp",
        "harness/src/h6_*.h",
        "harness/test/h6_*.cpp"
    };
    env.mutation_scope.prohibited_paths = {
        "litenyx/litenyx/*.h",
        "litenyx/cpp_reference/test/*.cpp"
    };
    env.evidence_requirements.required_evidence = {
        "mutation-results",
        "gameability-detection",
        "statistical-significance"
    };
    env.evidence_requirements.evidence_manifest_schema = "anti-gaming-manifest";
    env.promotion_gate.required_passes = {
        "gameability-detection-test",
        "mutation-sensitivity-test"
    };
    env.promotion_gate.required_controls = {
        "known-gameable-metric-injection"
    };
    env.invalidation_conditions = {
        "non-deterministic-mutation",
        "incorrect-gameability-detection"
    };
    return env;
}

AuthorityEnvelope AuthorityFactory::CreateH7() {
    AuthorityEnvelope env;
    env.task_id = "H7-deterministic-evidence-emitter";
    env.operation_class = "evidence-emission";
    env.authority.source = "ORCH-HARNESS-BUILD-1";
    env.authority.state = AuthorityState::EXECUTABLE;
    env.capabilities.discover = true;
    env.capabilities.mutate = true;
    env.capabilities.promote = false;
    env.allowed_operations = {
        "emit-evidence-manifest",
        "compute-evidence-hash",
        "verify-evidence-integrity",
        "replay-evidence"
    };
    env.prohibited_operations = {
        "modify-evidence-schema",
        "modify-hash-algorithm"
    };
    env.immutable_inputs = {
        "H_MSF:fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc",
        "MSF-CONTRACT-FREEZE-1:evidence-section"
    };
    env.dependencies = {
        "H1-canonical-state-generator",
        "H2-nativepow-auxpow-executor",
        "H3-fork-reorg-displacement-engine",
        "H4-rentability-external-telemetry",
        "H5-missing-telemetry-handler",
        "H6-anti-gaming-executor"
    };
    env.mutation_scope.worktree = "h7-evidence-worktree";
    env.mutation_scope.allowed_paths = {
        "harness/src/h7_*.cpp",
        "harness/src/h7_*.h",
        "harness/test/h7_*.cpp",
        "harness/evidence/*.json"
    };
    env.mutation_scope.prohibited_paths = {
        "litenyx/litenyx/*.h",
        "litenyx/cpp_reference/test/*.cpp"
    };
    env.evidence_requirements.required_evidence = {
        "evidence-manifest-hash",
        "replay-verification"
    };
    env.evidence_requirements.evidence_manifest_schema = "evidence-manifest-schema";
    env.promotion_gate.required_passes = {
        "evidence-determinism-test",
        "evidence-replay-test"
    };
    env.promotion_gate.required_controls = {
        "corrupted-evidence-injection",
        "mismatched-hash-injection"
    };
    env.invalidation_conditions = {
        "non-deterministic-evidence",
        "evidence-integrity-failure"
    };
    return env;
}

AuthorityEnvelope AuthorityFactory::CreateH8() {
    AuthorityEnvelope env;
    env.task_id = "H8-independent-conformance-verifier";
    env.operation_class = "independent-verification";
    env.authority.source = "ORCH-HARNESS-BUILD-1";
    env.authority.state = AuthorityState::EXECUTABLE;
    env.capabilities.discover = true;
    env.capabilities.mutate = true;
    env.capabilities.promote = true;  // Can promote to integration queue
    env.allowed_operations = {
        "verify-contract-conformance",
        "verify-determinism",
        "verify-replayability",
        "verify-telemetry",
        "verify-mutation-sensitivity",
        "verify-evidence-integrity",
        "verify-independent-implementation"
    };
    env.prohibited_operations = {
        "modify-verification-logic",
        "modify-contract-reference"
    };
    env.immutable_inputs = {
        "H_MSF:fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc",
        "MSF-CONTRACT-FREEZE-1",
        "MSF-CONTRACT-REFREEZE-1"
    };
    env.dependencies = {
        "H1-canonical-state-generator",
        "H2-nativepow-auxpow-executor",
        "H3-fork-reorg-displacement-engine",
        "H4-rentability-external-telemetry",
        "H5-missing-telemetry-handler",
        "H6-anti-gaming-executor",
        "H7-deterministic-evidence-emitter"
    };
    env.mutation_scope.worktree = "h8-verifier-worktree";
    env.mutation_scope.allowed_paths = {
        "harness/src/h8_*.cpp",
        "harness/src/h8_*.h",
        "harness/test/h8_*.cpp"
    };
    env.mutation_scope.prohibited_paths = {
        "harness/src/h1_*.cpp",
        "harness/src/h2_*.cpp",
        "harness/src/h3_*.cpp",
        "harness/src/h4_*.cpp",
        "harness/src/h5_*.cpp",
        "harness/src/h6_*.cpp",
        "harness/src/h7_*.cpp"
    };
    env.evidence_requirements.required_evidence = {
        "verification-result",
        "independent-implementation-hash"
    };
    env.evidence_requirements.evidence_manifest_schema = "verification-manifest";
    env.promotion_gate.required_passes = {
        "contract-conformance-test",
        "determinism-test",
        "replayability-test",
        "telemetry-test",
        "mutation-sensitivity-test",
        "evidence-integrity-test",
        "independent-implementation-test"
    };
    env.promotion_gate.required_controls = {
        "mismatched-immutable-input-injection",
        "unauthorized-mutation-injection"
    };
    env.invalidation_conditions = {
        "verification-logic-defect",
        "shared-implementation-with-h1-h7"
    };
    return env;
}

bool AuthorityFactory::Validate(const AuthorityEnvelope& envelope) {
    // Check required fields
    if (envelope.task_id.empty()) return false;
    if (envelope.operation_class.empty()) return false;
    if (envelope.authority.source.empty()) return false;
    
    // Check immutable inputs
    if (envelope.immutable_inputs.empty()) return false;
    
    // Check that H_MSF is present in immutable inputs
    bool has_hmsf = false;
    for (const auto& input : envelope.immutable_inputs) {
        if (input.find("H_MSF:") == 0) {
            has_hmsf = true;
            break;
        }
    }
    if (!has_hmsf) return false;
    
    // Check capabilities consistency
    if (!envelope.capabilities.discover) return false;  // Must always be able to discover
    if (envelope.capabilities.promote && !envelope.capabilities.mutate) return false;
    
    return true;
}

bool AuthorityFactory::IsOperationAllowed(
    const AuthorityEnvelope& envelope,
    const std::string& operation
) {
    // Check if operation is in allowed list
    bool allowed = false;
    for (const auto& op : envelope.allowed_operations) {
        if (op == operation) {
            allowed = true;
            break;
        }
    }
    
    if (!allowed) return false;
    
    // Check if operation is in prohibited list
    for (const auto& op : envelope.prohibited_operations) {
        if (op == operation) {
            return false;
        }
    }
    
    return true;
}

bool AuthorityFactory::IsPathInScope(
    const AuthorityEnvelope& envelope,
    const std::string& path
) {
    // Check if path is in prohibited list
    for (const auto& prohibited : envelope.mutation_scope.prohibited_paths) {
        if (prohibited == "*") return false;  // All paths prohibited
        // Simple prefix matching
        if (path.find(prohibited) == 0) return false;
    }
    
    // If no allowed paths specified, check if not prohibited
    if (envelope.mutation_scope.allowed_paths.empty()) {
        return true;
    }
    
    // Check if path is in allowed list
    for (const auto& allowed : envelope.mutation_scope.allowed_paths) {
        // Simple prefix matching
        if (path.find(allowed) == 0) return true;
    }
    
    return false;
}

// ============================================================================
// AuthorityStateMachine Implementation
// ============================================================================

bool AuthorityStateMachine::Transition(
    AuthorityEnvelope& envelope,
    AuthorityState new_state
) {
    if (!IsValidTransition(envelope.authority.state, new_state)) {
        return false;
    }
    envelope.authority.state = new_state;
    return true;
}

bool AuthorityStateMachine::IsValidTransition(
    AuthorityState from,
    AuthorityState to
) {
    // Valid transitions:
    // BLOCKED -> PREPARATION_ONLY
    // PREPARATION_ONLY -> EXECUTABLE
    // EXECUTABLE -> BLOCKED (invalidation)
    // Any -> BLOCKED (invalidation)
    
    if (to == AuthorityState::BLOCKED) return true;  // Always can be blocked
    
    switch (from) {
        case AuthorityState::BLOCKED:
            return to == AuthorityState::PREPARATION_ONLY;
        case AuthorityState::PREPARATION_ONLY:
            return to == AuthorityState::EXECUTABLE;
        case AuthorityState::EXECUTABLE:
            return false;  // Cannot go back to preparation
    }
    
    return false;
}

const char* AuthorityStateMachine::StateName(AuthorityState state) {
    switch (state) {
        case AuthorityState::BLOCKED: return "BLOCKED";
        case AuthorityState::PREPARATION_ONLY: return "PREPARATION_ONLY";
        case AuthorityState::EXECUTABLE: return "EXECUTABLE";
    }
    return "UNKNOWN";
}

} // namespace harness
