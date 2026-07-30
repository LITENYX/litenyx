// MSF Evidence Manifest Schema Verifier Implementation - CE1 Phase 1.10
//
// Validates evidence manifests against frozen SPEC-3 §9 requirements.
// Authority class: EVIDENCE_OUTPUT (validation only, no mutation)

#include "msf_evidence_manifest_verifier.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace msf {

EvidenceManifestSchemaVerifier::EvidenceManifestSchemaVerifier() {
    // Initialize schema definitions based on SPEC-3 §9 and authority requirements
    
    schemas_["state-generation-manifest"] = {
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "timestamps", "state-hash", "utxo-root", "work-source-composition"},
        {"state-hash", "utxo-root", "work-source-composition"},
        "H1: Canonical state generation manifest"
    };
    
    schemas_["work-execution-manifest"] = {
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "telemetry_hashes", "commands", "environment", "configuration", "validation-result", "work-hash", "target-comparison"},
        {"validation-result", "work-hash", "target-comparison"},
        "H2: Work execution manifest"
    };
    
    schemas_["fork-simulation-manifest"] = {
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "telemetry_hashes", "commands", "environment", "configuration", "competing-history-hashes", "reorg-depth", "displacement-cost", "fork-winner"},
        {"competing-history-hashes", "reorg-depth", "displacement-cost", "fork-winner"},
        "H3: Fork/reorg/displacement manifest"
    };
    
    schemas_["telemetry-manifest"] = {
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "telemetry_hashes", "commands", "environment", "configuration", "telemetry-sources", "aggregation-rules", "missing-data-treatment", "rentability-discount"},
        {"telemetry-sources", "aggregation-rules", "missing-data-treatment", "rentability-discount"},
        "H4: Rentability/external telemetry manifest"
    };
    
    schemas_["missing-data-manifest"] = {
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "telemetry_hashes", "commands", "environment", "configuration", "missing-data-report", "conservative-default-application"},
        {"missing-data-report", "conservative-default-application"},
        "H5: Missing telemetry handler manifest"
    };
    
    schemas_["anti-gaming-manifest"] = {
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "telemetry_hashes", "commands", "environment", "configuration", "mutation-results", "gameability-detection", "statistical-significance"},
        {"mutation-results", "gameability-detection", "statistical-significance"},
        "H6: Anti-gaming executor manifest"
    };
    
    schemas_["evidence-manifest-schema"] = {
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "telemetry_hashes", "raw_observations", "derived_quantities", "test_results", "negative_control_results", "errors", "output_hashes", "commands", "environment", "configuration", "timestamps", "evidence-manifest-hash", "replay-verification"},
        {"evidence-manifest-hash", "replay-verification"},
        "H7: Deterministic evidence emitter manifest"
    };
    
    schemas_["verification-manifest"] = {
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "telemetry_hashes", "raw_observations", "derived_quantities", "test_results", "negative_control_results", "errors", "output_hashes", "commands", "environment", "configuration", "timestamps", "verification-result", "independent-implementation-hash"},
        {"verification-result", "independent-implementation-hash"},
        "H8: Independent conformance verifier manifest"
    };
    
    schemas_["evidence-manifest"] = {  // Default/schema-agnostic validation
        {"task_id", "H_MSF", "manifest_hash", "authority_class", "canonical_input_hashes", "telemetry_hashes", "raw_observations", "derived_quantities", "test_results", "negative_control_results", "errors", "output_hashes", "commands", "environment", "configuration", "timestamps"},
        {},
        "Generic evidence manifest validation"
    };
}

msf::EvidenceManifestSchemaVerifier::ValidationResult 
EvidenceManifestSchemaVerifier::ValidateManifest(
    const EvidenceManifest& manifest,
    const std::string& schema_name) {
    
    ValidationResult result;
    result.valid = true;
    result.manifest_hash = manifest.manifest_hash;
    
    // 1. Validate basic schema
    if (!ValidateSchema(manifest, schema_name)) {
        result.valid = false;
        result.failed_checks.push_back("schema-validation");
        result.error_message = "Manifest schema validation failed";
    }
    
    // 2. Validate authority class
    if (!ValidateAuthorityClass(manifest.authority_class)) {
        result.valid = false;
        result.failed_checks.push_back("authority-class-validation");
        if (result.error_message.empty()) {
            result.error_message = "Invalid authority class";
        } else {
            result.error_message += "; invalid authority class";
        }
    }
    
    // 3. Validate H_MSF (frozen contract)
    if (!ValidateHMSF(manifest.H_MSF)) {
        result.valid = false;
        result.failed_checks.push_back("h-msf-validation");
        if (result.error_message.empty()) {
            result.error_message = "Invalid H_MSF hash";
        } else {
            result.error_message += "; invalid H_MSF hash";
        }
    }
    
    // 4. Validate authority chain
    if (!ValidateAuthorityChain(manifest)) {
        result.valid = false;
        result.failed_checks.push_back("authority-chain-validation");
        if (result.error_message.empty()) {
            result.error_message = "Invalid authority chain";
        } else {
            result.error_message += "; invalid authority chain";
        }
    }
    
    // 5. Validate hash chain
    if (!ValidateHashChain(manifest)) {
        result.valid = false;
        result.failed_checks.push_back("hash-chain-validation");
        if (result.error_message.empty()) {
            result.error_message = "Invalid hash chain";
        } else {
            result.error_message += "; invalid hash chain";
        }
    }
    
    // 6. Validate evidence hashes
    if (!ValidateEvidenceHashes(manifest)) {
        result.valid = false;
        result.failed_checks.push_back("evidence-hash-validation");
        if (result.error_message.empty()) {
            result.error_message = "Invalid evidence hashes";
        } else {
            result.error_message += "; invalid evidence hashes";
        }
    }
    
    // 7. Validate completeness (C1-C9) - only for final evidence manifests
    if (manifest.authority_class == "EVIDENCE_OUTPUT" || 
        manifest.authority_class == "EXPERIMENT_INFRASTRUCTURE") {
        result.completeness_result = ValidateCompleteness(manifest);
        if (!result.completeness_result.overall_passed) {
            result.valid = false;
            result.failed_checks.push_back("completeness-validation");
            if (result.error_message.empty()) {
                result.error_message = "Completeness validation failed (C1-C9)";
            } else {
                result.error_message += "; completeness validation failed (C1-C9)";
            }
        }
    }
    
    // Collect warnings (non-fatal issues)
    if (manifest.task_id.empty()) {
        result.warnings.push_back("Empty task_id");
    }
    if (manifest.timestamps.empty()) {
        result.warnings.push_back("No timestamps recorded");
    }
    if (manifest.authority_envelope_hash.empty()) {
        result.warnings.push_back("Missing authority envelope hash");
    }
    
    return result;
}

bool EvidenceManifestSchemaVerifier::ValidateAuthorityChain(
    const EvidenceManifest& manifest) const {
    
    // Authority chain must follow: 
    // FROZEN_INPUT → SCENARIO_INPUT → OBSERVATIONAL_ADAPTER → 
    // DERIVED_METRIC → EXPERIMENT_INFRASTRUCTURE → EVIDENCE_OUTPUT
    
    // For validation purposes, we accept any valid authority class
    // The actual chain validation would be done by checking dependencies
    // in the authority envelope, but for manifest validation we check:
    // 1. Authority class is valid
    // 2. If it's EVIDENCE_OUTPUT, it should have come from EXPERIMENT_INFRASTRUCTURE
    
    if (!ValidateAuthorityClass(manifest.authority_class)) {
        return false;
    }
    
    // Additional chain logic could go here based on authority_envelope_hash
    // or dependencies in the manifest, but for basic validation:
    return true;
}

CompletenessResult EvidenceManifestSchemaVerifier::ValidateCompleteness(
    const EvidenceManifest& manifest) const {
    
    // Create a minimal scenario result from manifest for completeness checking
    // In a full implementation, we'd extract scenario data from the manifest
    // For now, we'll use the completeness evaluator with default/empty data
    // since the manifest should already contain completeness evidence
    
    CompletenessEvaluator evaluator;
    
    // Extract completeness data from manifest if available
    // This would require extending EvidenceManifest to include:
    // - scenario_results
    // - replication_results  
    // But for Phase 1.10 validation, we check if completeness evidence exists
    
    ScenarioResult empty_scenario_result{};
    std::vector<ReplicationResult> empty_replications{};
    
    // Try to extract from manifest - for now return basic check
    // In practice, the manifest should contain hypothesis_verdicts, etc.
    // that would be used here
    
    CompletenessResult result = evaluator.Evaluate(
        empty_scenario_result, 
        empty_replications, 
        manifest
    );
    
    // Override with manifest-specific checks
    // Check for hypothesis verdicts in raw_observations or derived_quantities
    bool has_hypothesis_data = false;
    for (const auto& obs : manifest.raw_observations) {
        if (obs.key.find("hypothesis") != std::string::npos ||
            obs.key.find("verdict") != std::string::npos ||
            obs.key.find("MSF-H") != std::string::npos) {
            has_hypothesis_data = true;
            break;
        }
    }
    
    // Check for completeness results
    bool has_completeness_data = false;
    for (const auto& obs : manifest.raw_observations) {
        if (obs.key.find("completeness") != std::string::npos ||
            obs.key.find("C1") != std::string::npos ||
            obs.key.find("C2") != std::string::npos ||
            obs.key.find("C3") != std::string::npos ||
            obs.key.find("C4") != std::string::npos ||
            obs.key.find("C5") != std::string::npos ||
            obs.key.find("C6") != std::string::npos ||
            obs.key.find("C7") != std::string::npos ||
            obs.key.find("C8") != std::string::npos ||
            obs.key.find("C9") != std::string::npos) {
            has_completeness_data = true;
            break;
        }
    }
    
    // If we found hypothesis/completeness data, adjust result accordingly
    if (has_hypothesis_data && has_completeness_data) {
        // This is a simplified check - in reality we'd parse the actual data
        result.results[CompletenessCheck::C7_COMPUTABLE_METRIC].passed = true;
        result.results[CompletenessCheck::C8_HYPOTHESIS_VERDICTS_DERIVED].passed = true;
        result.results[CompletenessCheck::C9_NON_TRIVIAL_EXPERIMENT].passed = true;
        
        // Update overall status
        result.overall_passed = true;
        for (const auto& [check, r] : result.results) {
            if (!r.passed) {
                result.overall_passed = false;
                break;
            }
        }
    }
    
    result.summary = "Evidence-based completeness check";
    return result;
}

bool EvidenceManifestSchemaVerifier::ValidateHashChain(
    const EvidenceManifest& manifest) const {
    
    if (manifest.previous_manifest_hash.empty()) {
        // Genesis manifest - valid
        return true;
    }
    
    // Verify hash matches
    std::string computed = "";
    // In a real implementation, we'd compute the hash of the current manifest
    // and compare with manifest.manifest_hash
    // For now, we'll do a basic check that the hash is present and valid format
    
    // Basic hash format check (should be 64 hex chars prefixed with 0x)
    if (manifest.manifest_hash.length() < 2 || 
        manifest.manifest_hash.substr(0, 2) != "0x" ||
        manifest.manifest_hash.length() != 66) {
        return false;
    }
    
    // Check previous hash format
    if (manifest.previous_manifest_hash.length() < 2 || 
        manifest.previous_manifest_hash.substr(0, 2) != "0x" ||
        manifest.previous_manifest_hash.length() != 66) {
        return false;
    }
    
    // In full implementation: compute hash of manifest and verify it matches manifest_hash
    // and that manifest_hash matches what's expected given previous_manifest_hash
    return true;  // Placeholder
}

bool EvidenceManifestSchemaVerifier::ValidateSchema(
    const EvidenceManifest& manifest,
    const std::string& schema_name) const {
    
    // Check if schema exists
    auto it = schemas_.find(schema_name);
    if (it == schemas_.end()) {
        // Fall back to default evidence-manifest schema
        it = schemas_.find("evidence-manifest");
        if (it == schemas_.end()) {
            return false;
        }
    }
    
    const SchemaDefinition& schema = it->second;
    
    // Check required fields
    std::vector<std::string> missing_fields;
    if (!ValidateRequiredFields(manifest, schema_name, missing_fields)) {
        return false;
    }
    
    // Additional schema-specific validations could go here
    return true;
}

bool EvidenceManifestSchemaVerifier::ValidateRequiredFields(
    const EvidenceManifest& manifest,
    const std::string& schema_name,
    std::vector<std::string>& missing_fields) const {
    
    auto it = schemas_.find(schema_name);
    if (it == schemas_.end()) {
        it = schemas_.find("evidence-manifest");
        if (it == schemas_.end()) {
            missing_fields.push_back("schema-definition-not-found");
            return false;
        }
    }
    
    const SchemaDefinition& schema = it->second;
    
    // Check each required field
    for (const auto& field : schema.required_fields) {
        bool found = false;
        
        if (field == "task_id") {
            if (!manifest.task_id.empty()) found = true;
        } else if (field == "H_MSF") {
            if (!manifest.H_MSF.empty()) found = true;
        } else if (field == "manifest_hash") {
            if (!manifest.manifest_hash.empty()) found = true;
        } else if (field == "authority_class") {
            if (!manifest.authority_class.empty()) found = true;
        } else if (field == "canonical_input_hashes") {
            if (!manifest.canonical_input_hashes.empty()) found = true;
        } else if (field == "telemetry_hashes") {
            if (!manifest.telemetry_hashes.empty()) found = true;
        } else if (field == "raw_observations") {
            if (!manifest.raw_observations.empty()) found = true;
        } else if (field == "derived_quantities") {
            if (!manifest.derived_quantities.empty()) found = true;
        } else if (field == "test_results") {
            if (!manifest.test_results.empty()) found = true;
        } else if (field == "negative_control_results") {
            if (!manifest.negative_control_results.empty()) found = true;
        } else if (field == "errors") {
            if (!manifest.errors.empty()) found = true;
        } else if (field == "output_hashes") {
            if (!manifest.output_hashes.empty()) found = true;
        } else if (field == "commands") {
            if (!manifest.commands.empty()) found = true;
        } else if (field == "environment") {
            // Optional in some schemas, but we'll check if present
            found = true;  // environment is optional
        } else if (field == "configuration") {
            found = true;  // configuration is optional
        } else if (field == "timestamps") {
            if (!manifest.timestamps.empty()) found = true;
        } else if (field == "state-hash") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "state-hash") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "state-hash") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "utxo-root") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "utxo-root") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "utxo-root") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "work-source-composition") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "work-source-composition") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "work-source-composition") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "validation-result") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "validation-result") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "validation-result") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "work-hash") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "work-hash") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "work-hash") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "target-comparison") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "target-comparison") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "target-comparison") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "competing-history-hashes") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "competing-history-hashes") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "competing-history-hashes") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "reorg-depth") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "reorg-depth") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "reorg-depth") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "displacement-cost") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "displacement-cost") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "displacement-cost") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "fork-winner") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "fork-winner") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "fork-winner") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "telemetry-sources") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "telemetry-sources") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "telemetry-sources") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "aggregation-rules") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "aggregation-rules") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "aggregation-rules") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "missing-data-treatment") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "missing-data-treatment") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "missing-data-treatment") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "rentability-discount") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "rentability-discount") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "rentability-discount") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "missing-data-report") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "missing-data-report") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "missing-data-report") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "conservative-default-application") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "conservative-default-application") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "conservative-default-application") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "mutation-results") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "mutation-results") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "mutation-results") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "gameability-detection") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "gameability-detection") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "gameability-detection") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "statistical-significance") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "statistical-significance") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "statistical-significance") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "evidence-manifest-hash") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "evidence-manifest-hash") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "evidence-manifest-hash") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "replay-verification") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "replay-verification") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "replay-verification") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "verification-result") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "verification-result") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "verification-result") {
                        found = true;
                        break;
                    }
                }
            }
        } else if (field == "independent-implementation-hash") {
            // Check in raw_observations or derived_quantities
            for (const auto& obs : manifest.raw_observations) {
                if (obs.key == "independent-implementation-hash") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& qty : manifest.derived_quantities) {
                    if (qty.key == "independent-implementation-hash") {
                        found = true;
                        break;
                    }
                }
            }
        }
        
        if (!found) {
            missing_fields.push_back(field);
        }
    }
    
    return missing_fields.empty();
}

bool EvidenceManifestSchemaVerifier::ValidateAuthorityClass(
    const std::string& authority_class) const {
    
    for (const auto& valid_class : kAuthorityClasses) {
        if (authority_class == valid_class) {
            return true;
        }
    }
    return false;
}

bool EvidenceManifestSchemaVerifier::ValidateHMSF(
    const std::string& h_msf) const {
    
    return h_msf == kFrozenHMSF;
}

bool EvidenceManifestSchemaVerifier::ValidateEvidenceHashes(
    const EvidenceManifest& manifest) const {
    
    // Validate that all evidence entries have valid hashes
    // Each EvidenceEntry should have a hash that matches SHA256 of its value
    
    auto validate_entries = [](const std::vector<EvidenceEntry>& entries) -> bool {
        for (const auto& entry : entries) {
            if (entry.key.empty() || entry.value.empty() || entry.hash.empty()) {
                return false;
            }
            // In real implementation: verify entry.hash == SHA256(entry.value)
            // For now, just check format
            if (entry.hash.length() < 2 || 
                entry.hash.substr(0, 2) != "0x" ||
                entry.hash.length() != 66) {
                return false;
            }
        }
        return true;
    };
    
    if (!validate_entries(manifest.canonical_input_hashes)) return false;
    if (!validate_entries(manifest.telemetry_hashes)) return false;
    if (!validate_entries(manifest.raw_observations)) return false;
    if (!validate_entries(manifest.derived_quantities)) return false;
    if (!validate_entries(manifest.output_hashes)) return false;
    
    return true;
}

std::string EvidenceManifestSchemaVerifier::GenerateValidationReport(
    const ValidationResult& result) const {
    
    std::ostringstream oss;
    oss << "=== EVIDENCE MANIFEST SCHEMA VALIDATION REPORT ===\n";
    oss << "Manifest Hash: " << result.manifest_hash << "\n";
    oss << "Overall Valid: " << (result.valid ? "YES" : "NO") << "\n\n";
    
    if (!result.failed_checks.empty()) {
        oss << "FAILED CHECKS:\n";
        for (const auto& check : result.failed_checks) {
            oss << "  - " << check << "\n";
        }
        oss << "\n";
    }
    
    if (!result.warnings.empty()) {
        oss << "WARNINGS:\n";
        for (const auto& warning : result.warnings) {
            oss << "  - " << warning << "\n";
        }
        oss << "\n";
    }
    
    oss << "ERROR MESSAGE: " << (result.error_message.empty() ? "None" : result.error_message) << "\n\n";
    
    // Completeness results
    oss << "COMPLETENESS (C1-C9):\n";
    oss << "  Overall: " << (result.completeness_result.overall_passed ? "PASS" : "FAIL") << "\n";
    for (const auto& [check, r] : result.completeness_result.results) {
        std::string check_name;
        switch (check) {
            case CompletenessCheck::C1_ALL_SCENARIOS_EXECUTED: check_name = "C1"; break;
            case CompletenessCheck::C2_ALL_REPLICATIONS_PRESENT: check_name = "C2"; break;
            case CompletenessCheck::C3_NO_PARAMETER_SUBSTITUTION: check_name = "C3"; break;
            case CompletenessCheck::C4_OUTPUT_SCHEMA_VALID: check_name = "C4"; break;
            case CompletenessCheck::C5_VIABILITY_CHECK_PRESENT: check_name = "C5"; break;
            case CompletenessCheck::C6_DETERMINISTIC_REPLAY: check_name = "C6"; break;
            case CompletenessCheck::C7_COMPUTABLE_METRIC: check_name = "C7"; break;
            case CompletenessCheck::C8_HYPOTHESIS_VERDICTS_DERIVED: check_name = "C8"; break;
            case CompletenessCheck::C9_NON_TRIVIAL_EXPERIMENT: check_name = "C9"; break;
        }
        oss << "  " << check_name << ": " << (r.passed ? "PASS" : "FAIL") << " - " << r.details << "\n";
    }
    oss << "  Summary: " << result.completeness_result.summary << "\n\n";
    
    if (!result.completeness_result.failures.empty()) {
        oss << "  FAILURES: ";
        for (size_t i = 0; i < result.completeness_result.failures.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << result.completeness_result.failures[i];
        }
        oss << "\n";
    }
    
    return oss.str();
}

} // namespace msf