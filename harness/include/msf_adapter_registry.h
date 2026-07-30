// MSF Adapter Registry - CE1 Phase 2
//
// Unified interface for all 10 observational adapters (M01-M10).
// Authority class: EXPERIMENT_INFRASTRUCTURE (coordinates adapters)

#ifndef MSF_ADAPTER_REGISTRY_H
#define MSF_ADAPTER_REGISTRY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include "msf_scenario_engine.h"
#include "msf_evidence_emission.h"

namespace msf {

// ============================================================================
// Adapter Interface
// ============================================================================

struct AdapterResult {
    std::string metric_id;
    std::string metric_name;
    std::string value;           // Q32.32 hex string
    std::string authority_class;
    bool valid;
    std::string error_message;
};

using AdapterFn = std::function<AdapterResult(const BlockObservation&)>;

// ============================================================================
// Adapter Registry
// ============================================================================

class AdapterRegistry {
public:
    AdapterRegistry();
    
    // Register all 10 adapters
    void RegisterAll();
    
    // Execute all adapters for a block observation
    std::vector<AdapterResult> ExecuteAll(const BlockObservation& obs) const;
    
    // Execute single adapter by metric ID
    std::optional<AdapterResult> Execute(const std::string& metric_id, const BlockObservation& obs) const;
    
    // Get list of all registered metric IDs
    std::vector<std::string> GetRegisteredMetrics() const;
    
    // Validate all adapters against golden vectors
    struct ValidationReport {
        std::string metric_id;
        bool passed;
        std::string computed;
        std::string expected;
    };
    std::vector<ValidationReport> ValidateAllGolden() const;

private:
    std::unordered_map<std::string, AdapterFn> adapters_;
    std::unordered_map<std::string, std::string> metric_names_;
    std::unordered_map<std::string, std::string> authority_classes_;
    
    // Golden vectors from SPEC-3 (synthetic for CE1 build-time tests)
    struct GoldenVector {
        std::string metric_id;
        BlockObservation input;
        std::string expected_output;
    };
    std::vector<GoldenVector> golden_vectors_;
    
    // Built-in golden vectors
    void InitializeGoldenVectors();
};

// ============================================================================
// Convenience: Emit evidence entries for all adapter results
// ============================================================================

void EmitAdapterEvidence(
    EvidenceManifest& manifest,
    const std::vector<AdapterResult>& results,
    const std::string& authority_envelope_hash);

} // namespace msf

#endif // MSF_ADAPTER_REGISTRY_H