#ifndef HARNESS_EVIDENCE_H
#define HARNESS_EVIDENCE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace harness {

// ============================================================================
// O4: Evidence Manifest
//
// Records claims, commands, inputs, outputs, hashes, tests. Every worker
// task produces an evidence manifest that can be independently verified.
// ============================================================================

struct EvidenceEntry {
    std::string key;
    std::string value;
    std::string hash;  // SHA-256 of value
    std::string timestamp;
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
    std::string H_MSF;
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
    std::string manifest_hash;  // SHA-256 of entire manifest
    std::string previous_manifest_hash;  // For chaining
};

class EvidenceManager {
public:
    EvidenceManager(const std::string& evidence_dir);
    
    // Create new evidence manifest for a task
    EvidenceManifest CreateManifest(
        const std::string& task_id,
        const std::string& authority_envelope_hash,
        const std::string& H_MSF
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
    
    // Save manifest to file
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

private:
    std::string evidence_dir_;
    
    // Execute shell command
    std::string ExecuteCommand(const std::string& command) const;
    
    // Compute SHA-256 hash
    std::string ComputeSHA256(const std::string& data) const;
    
    // Serialize manifest to string
    std::string SerializeManifest(const EvidenceManifest& manifest) const;
    
    // Deserialize manifest from string
    EvidenceManifest DeserializeManifest(const std::string& data) const;
    
    // Get current timestamp
    std::string GetCurrentTimestamp() const;
};

// ============================================================================
// Evidence Schema Validator
//
// Validates that evidence manifests conform to the expected schema.
// ============================================================================

class EvidenceSchemaValidator {
public:
    static bool ValidateManifest(
        const EvidenceManifest& manifest,
        const std::string& schema_name
    );
    
    static bool ValidateEntry(const EvidenceEntry& entry);
    static bool ValidateTestResult(const TestResult& result);
    static bool ValidateCommandRecord(const CommandRecord& record);
    
    // Get required fields for each schema
    static std::vector<std::string> GetRequiredFields(
        const std::string& schema_name
    );
    
    // Get optional fields for each schema
    static std::vector<std::string> GetOptionalFields(
        const std::string& schema_name
    );
};

} // namespace harness

#endif // HARNESS_EVIDENCE_H
