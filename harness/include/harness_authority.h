#ifndef HARNESS_AUTHORITY_H
#define HARNESS_AUTHORITY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace harness {

// ============================================================================
// O1: Authority Envelope
//
// Machine-readable task authority. Each worker task receives an authority
// envelope that defines what it can and cannot do.
// ============================================================================

enum class AuthorityState {
    BLOCKED,
    PREPARATION_ONLY,
    EXECUTABLE
};

enum class Capability {
    DISCOVER,   // Read-only exploration
    MUTATE,     // Allowed to modify within scope
    PROMOTE     // Allowed to promote to integration queue
};

struct AuthorityEnvelope {
    // Identity
    std::string task_id;
    std::string operation_class;
    
    // Authority source
    struct {
        std::string source;  // e.g., "ORCH-HARNESS-BUILD-1"
        AuthorityState state;
    } authority;
    
    // Capabilities
    struct {
        bool discover;
        bool mutate;
        bool promote;
    } capabilities;
    
    // Operations
    std::vector<std::string> allowed_operations;
    std::vector<std::string> prohibited_operations;
    
    // Immutable inputs (must match exactly)
    std::vector<std::string> immutable_inputs;
    
    // Dependencies
    std::vector<std::string> dependencies;
    
    // Mutation scope
    struct {
        std::string worktree;
        std::vector<std::string> allowed_paths;
        std::vector<std::string> prohibited_paths;
    } mutation_scope;
    
    // Evidence requirements
    struct {
        std::vector<std::string> required_evidence;
        std::string evidence_manifest_schema;
    } evidence_requirements;
    
    // Promotion gate
    struct {
        std::vector<std::string> required_passes;
        std::vector<std::string> required_controls;
    } promotion_gate;
    
    // Invalidation conditions
    std::vector<std::string> invalidation_conditions;
};

// ============================================================================
// Authority Envelope Factory
//
// Creates pre-configured authority envelopes for each harness worker.
// ============================================================================

class AuthorityFactory {
public:
    // Create authority envelope for H1: Canonical State Generator
    static AuthorityEnvelope CreateH1();
    
    // Create authority envelope for H2: NativePoW/AuxPoW Executor
    static AuthorityEnvelope CreateH2();
    
    // Create authority envelope for H3: Fork/Reorg/Displacement Engine
    static AuthorityEnvelope CreateH3();
    
    // Create authority envelope for H4: Rentability + External Telemetry
    static AuthorityEnvelope CreateH4();
    
    // Create authority envelope for H5: Missing-Telemetry Handler
    static AuthorityEnvelope CreateH5();
    
    // Create authority envelope for H6: Anti-Gaming Executor
    static AuthorityEnvelope CreateH6();
    
    // Create authority envelope for H7: Deterministic Evidence Emitter
    static AuthorityEnvelope CreateH7();
    
    // Create authority envelope for H8: Independent Contract-Conformance Verifier
    static AuthorityEnvelope CreateH8();
    
    // Validate an authority envelope
    static bool Validate(const AuthorityEnvelope& envelope);
    
    // Check if an operation is allowed
    static bool IsOperationAllowed(
        const AuthorityEnvelope& envelope,
        const std::string& operation
    );
    
    // Check if a path is within mutation scope
    static bool IsPathInScope(
        const AuthorityEnvelope& envelope,
        const std::string& path
    );
};

// ============================================================================
// Authority State Machine
//
// Manages state transitions for authority envelopes.
// ============================================================================

class AuthorityStateMachine {
public:
    // Transition authority state
    static bool Transition(
        AuthorityEnvelope& envelope,
        AuthorityState new_state
    );
    
    // Check if transition is valid
    static bool IsValidTransition(
        AuthorityState from,
        AuthorityState to
    );
    
    // Get state name
    static const char* StateName(AuthorityState state);
};

} // namespace harness

#endif // HARNESS_AUTHORITY_H
