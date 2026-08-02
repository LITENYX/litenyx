// MSF Evidence Manifest Schema Verifier - CE1 Phase 1.10
//
// Validates evidence manifests against frozen SPEC-3 §9 requirements.
// Authority class: EVIDENCE_OUTPUT (validation only, no mutation)

#ifndef MSF_EVIDENCE_MANIFEST_VERIFIER_H
#define MSF_EVIDENCE_MANIFEST_VERIFIER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <filesystem>

#include "msf_evidence_emission.h"
#include "msf_completeness.h"
#include "harness_authority.h"

namespace msf {

// ============================================================================
// Evidence Manifest Schema Verifier
// ============================================================================

class EvidenceManifestSchemaVerifier {
public:
    explicit EvidenceManifestSchemaVerifier();
    
    // Validate evidence manifest against frozen SPEC-3 schema
    struct ValidationResult {
        bool valid;
        std::string error_message;
        std::string manifest_hash;
        std::vector<std::string> failed_checks;
        std::vector<std::string> warnings;
        CompletenessResult completeness_result;
    };
    
    ValidationResult ValidateManifest(
        const EvidenceManifest& manifest,
        const std::string& schema_name = "evidence-manifest"
    );
    
    // Validate authority chain (FROZEN_INPUT → SCENARIO_INPUT → OBSERVATIONAL_ADAPTER → 
    // DERIVED_METRIC → EXPERIMENT_INFRASTRUCTURE → EVIDENCE_OUTPUT)
    bool ValidateAuthorityChain(const EvidenceManifest& manifest) const;
    
    // Validate completeness (C1-C9) per SPEC-3 §8.1
    CompletenessResult ValidateCompleteness(const EvidenceManifest& manifest) const;
    
    // Validate hash chain integrity
    bool ValidateHashChain(const EvidenceManifest& manifest) const;
    
    // Validate against specific schema requirements
    bool ValidateSchema(const EvidenceManifest& manifest, const std::string& schema_name) const;
    
    // Generate validation report
    std::string GenerateValidationReport(const ValidationResult& result) const;

private:
    // Helper methods
    bool ValidateRequiredFields(const EvidenceManifest& manifest, 
                               const std::string& schema_name,
                               std::vector<std::string>& missing_fields) const;
                               
    bool ValidateAuthorityClass(const std::string& authority_class) const;
    
    bool ValidateHMSF(const std::string& h_msf) const;
    
    bool ValidateEvidenceHashes(const EvidenceManifest& manifest) const;
    
    // Schema definitions
    struct SchemaDefinition {
        std::vector<std::string> required_fields;
        std::vector<std::string> required_evidence;
        std::string description;
    };
    
    std::unordered_map<std::string, SchemaDefinition> schemas_;
    
    // Frozen contract constants
    static constexpr const char* kFrozenHMSF = "a855e7f17cc73f5b31979ee0b9b8f1ee532e3315a55edcf6a30cb702ee14ffd7";
    
    static constexpr const char* kAuthorityClasses[] = {
        "FROZEN_INPUT",
        "SCENARIO_INPUT", 
        "OBSERVATIONAL_ADAPTER",
        "DERIVED_METRIC",
        "EXPERIMENT_INFRASTRUCTURE",
        "EVIDENCE_OUTPUT"
    };
};

} // namespace msf

#endif // MSF_EVIDENCE_MANIFEST_VERIFIER_H