// MSF Evidence Emission (H7) - CE1 Phase 1.3
//
// Deterministic evidence manifest emission with hashing, serialization, and replay.
// Authority class: EVIDENCE_OUTPUT

#ifndef MSF_EVIDENCE_EMISSION_H
#define MSF_EVIDENCE_EMISSION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <functional>
#include <openssl/sha.h>

namespace msf {

// ============================================================================
// Evidence Manifest Schema (frozen per SPEC-3 §9)
// ============================================================================

struct EvidenceEntry {
    std::string key;
    std::string value;
    std::string hash;        // SHA-256 of value
    std::string timestamp;   // ISO 8601
};

struct TestResult {
    std::string test_name;
    bool passed;
    std::string details;
    std::string expected;
    std::string actual;
    std::string timestamp;
};

struct CommandRecord {
    std::string command;
    std::string working_directory;
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;
    std::string timestamp;
};

struct EvidenceManifest {
    // Identity
    std::string task_id;
    std::string authority_envelope_hash;
    std::string H_MSF;                    // Frozen: a855e7f17cc73f5b31979ee0b9b8f1ee532e3315a55edcf6a30cb702ee14ffd7
    std::vector<std::string> timestamps;
    
    // Provenance
    std::string repo_provenance;
    std::string executable_revision;
    std::string commit_hash;
    
    // Inputs
    std::vector<EvidenceEntry> canonical_input_hashes;
    std::vector<EvidenceEntry> telemetry_hashes;
    
    // Execution
    std::vector<CommandRecord> commands;
    std::string environment;
    std::string configuration;
    
    // Observations
    std::vector<EvidenceEntry> raw_observations;
    std::vector<EvidenceEntry> derived_quantities;
    
    // Tests
    std::vector<TestResult> test_results;
    std::vector<TestResult> negative_control_results;
    
    // Errors
    std::vector<std::string> errors;
    
    // Outputs
    std::vector<EvidenceEntry> output_hashes;
    
    // Integrity
    std::string manifest_hash;            // SHA-256 of entire manifest
    std::string previous_manifest_hash;   // For chaining
    
    // Authority class annotation
    std::string authority_class;          // FROZEN_INPUT | SCENARIO_INPUT | OBSERVATIONAL_ADAPTER | DERIVED_METRIC | EXPERIMENT_INFRASTRUCTURE | EVIDENCE_OUTPUT
};

// ============================================================================
// Evidence Manager
// ============================================================================

class EvidenceManager {
public:
    explicit EvidenceManager(const std::string& evidence_dir);
    
    // Create new evidence manifest for a task
    EvidenceManifest CreateManifest(
        const std::string& task_id,
        const std::string& authority_envelope_hash,
        const std::string& authority_class
    );
    
    // Add entry to manifest
    void AddEntry(
        EvidenceManifest& manifest,
        const std::string& key,
        const std::string& value
    );
    
    // Add test result
    void AddTestResult(
        EvidenceManifest& manifest,
        const TestResult& result
    );
    
    // Add negative control result
    void AddNegativeControlResult(
        EvidenceManifest& manifest,
        const TestResult& result
    );
    
    // Add command record
    void AddCommandRecord(
        EvidenceManifest& manifest,
        const CommandRecord& record
    );
    
    // Add error
    void AddError(
        EvidenceManifest& manifest,
        const std::string& error
    );
    
    // Finalize manifest (compute hash)
    std::string FinalizeManifest(EvidenceManifest& manifest);
    
    // Save manifest to file (JSON)
    bool SaveManifest(
        const EvidenceManifest& manifest,
        const std::string& filename
    );
    
    // Load manifest from file
    EvidenceManifest LoadManifest(const std::string& filename);
    
    // Verify manifest integrity
    bool VerifyManifestIntegrity(const EvidenceManifest& manifest) const;
    
    // Verify manifest chain
    bool VerifyManifestChain(
        const EvidenceManifest& current,
        const EvidenceManifest& previous
    );
    
    // Compute manifest hash
    std::string ComputeManifestHash(const EvidenceManifest& manifest) const;
    
    // Get all manifests for a task
    std::vector<EvidenceManifest> GetManifestsForTask(
        const std::string& task_id
    );
    
    // Get latest manifest for a task
    EvidenceManifest GetLatestManifest(const std::string& task_id);
    
    // Compare two manifests
    bool CompareManifests(
        const EvidenceManifest& a,
        const EvidenceManifest& b
    );
    
    // Generate evidence report
    std::string GenerateReport(const EvidenceManifest& manifest);
    
    // Replay verification: re-execute commands and compare outputs
    bool ReplayVerify(
        const EvidenceManifest& manifest,
        const std::function<std::string(const std::string&)>& command_executor
    );

private:
    std::string evidence_dir_;
    
    // Execute shell command
    std::string ExecuteCommand(const std::string& command) const;
    
    // Compute SHA-256 hash
    std::string ComputeSHA256(const std::string& data) const;
    
    // Serialize manifest to string (deterministic)
    std::string SerializeManifest(const EvidenceManifest& manifest) const;
    
    // Deserialize manifest from string
    EvidenceManifest DeserializeManifest(const std::string& data) const;
    
    // Get current timestamp (ISO 8601)
    std::string GetCurrentTimestamp() const;
    
    // Get file list in evidence directory for task
    std::vector<std::string> GetManifestFiles(const std::string& task_id) const;
    
    // Hash file
    std::string HashFile(const std::string& filepath) const;
};

} // namespace msf

#endif // MSF_EVIDENCE_EMISSION_H