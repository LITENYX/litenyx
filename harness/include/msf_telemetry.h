// MSF Telemetry Collection (H4) - CE1 Phase 1.4
//
// External telemetry collection for MSF execution.
// Authority class: OBSERVATIONAL_TELEMETRY (PREPARATION_ONLY)

#ifndef MSF_TELEMETRY_H
#define MSF_TELEMETRY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include <chrono>

namespace msf {

// ============================================================================
// Telemetry Data Structures
// ============================================================================

struct TelemetrySource {
    std::string name;
    std::string endpoint;
    std::string api_key;  // Stored separately in production
    std::string format;   // JSON, CSV, etc.
    std::chrono::seconds poll_interval;
    bool requires_auth = false;
};

struct TelemetryRecord {
    std::string source;
    std::string metric;
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::string units;
    std::unordered_map<std::string, std::string> metadata;
};

struct TelemetryAggregate {
    std::string metric;
    std::vector<TelemetryRecord> samples;
    double mean = 0;
    double min = 0;
    double max = 0;
    double stddev = 0;
    size_t count = 0;
    std::chrono::system_clock::time_point window_start;
    std::chrono::system_clock::time_point window_end;
};

// ============================================================================
// H4: Rentability + External Telemetry Collector
// ============================================================================

class TelemetryCollector {
public:
    TelemetryCollector();
    
    // Add telemetry source
    void AddSource(const TelemetrySource& source);
    
    // Collect telemetry for a scenario
    // Returns collected records (does not mutate production state)
    std::vector<TelemetryRecord> CollectForScenario(
        const std::string& scenario_id,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end);
    
    // Get aggregated telemetry
    std::vector<TelemetryAggregate> GetAggregates(
        const std::string& metric,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end);
    
    // Check availability
    bool IsSourceAvailable(const std::string& source_name) const;
    
    // Get missing telemetry report
    struct MissingTelemetryReport {
        std::string metric;
        std::string reason;  // "UNAVAILABLE", "PARTIAL", "STALE"
        std::string conservative_default;
        std::string notes;
    };
    std::vector<MissingTelemetryReport> CheckAvailability(
        const std::vector<std::string>& required_metrics);
    
    // Apply conservative defaults for missing telemetry
    std::unordered_map<std::string, double> ApplyConservativeDefaults(
        const std::vector<std::string>& required_metrics);

private:
    std::unordered_map<std::string, TelemetrySource> sources_;
    std::vector<TelemetryRecord> cache_;
    std::chrono::system_clock::time_point cache_expiry_;
    
    // Fetch from source (implementation-specific)
    std::vector<TelemetryRecord> FetchFromSource(
        const TelemetrySource& source,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end);
    
    // Validate and normalize record
    std::optional<TelemetryRecord> ValidateRecord(
        const TelemetryRecord& record);
    
    // Conservative defaults per metric (from SPEC-3 §3.5)
    double GetConservativeDefault(const std::string& metric) const;
};

// ============================================================================
// H5: Missing Telemetry Handler
// ============================================================================

struct MissingTelemetryEntry {
    std::string metric;
    std::string reason;  // UNAVAILABLE, PARTIAL, STALE, INVALID
    std::string conservative_default;
    std::string documentation;
    bool affects_conformance = false;
};

class MissingTelemetryHandler {
public:
    MissingTelemetryHandler();
    
    // Report missing telemetry
    void ReportMissing(const MissingTelemetryEntry& entry);
    
    // Get all missing telemetry entries
    const std::vector<MissingTelemetryEntry>& GetMissing() const;
    
    // Apply conservative defaults to a metric map
    std::unordered_map<std::string, double> ApplyDefaults(
        const std::unordered_map<std::string, double>& metrics);
    
    // Generate missing data report for evidence
    std::string GenerateReport() const;
    
    // Check if critical telemetry is missing
    bool HasCriticalMissing() const;

private:
    std::vector<MissingTelemetryEntry> missing_;
    
    // Conservative defaults from SPEC-3 §3.5 + MSF-EXECUTION-1
    static const std::unordered_map<std::string, double> kConservativeDefaults;
};

} // namespace msf

#endif // MSF_TELEMETRY_H