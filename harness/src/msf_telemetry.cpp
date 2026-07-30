// MSF Telemetry Collection Implementation (H4/H5) - CE1 Phase 1.4/1.5
//
// External telemetry collection and missing data handling.
// Authority class: OBSERVATIONAL_TELEMETRY (PREPARATION_ONLY)

#include "msf_telemetry.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace msf {

// Static conservative defaults
const std::unordered_map<std::string, double> MissingTelemetryHandler::kConservativeDefaults = {
    {"hash_rental_rate_usd_per_th", 1.0},      // $1/TH/day - very conservative
    {"exchange_rate_usd_per_ltc", 50.0},       // $50/LTC - conservative
    {"bandwidth_cost_usd_per_gb", 0.01},       // $0.01/GB - conservative
    {"capital_cost_annual_rate", 0.10},        // 10% - conservative
    {"hash_price_usd_per_th_day", 1.0},
    {"ltc_usd", 50.0},
    {"bandwidth_usd_per_gb", 0.01},
    {"capital_rate_annual", 0.10}
};

// ============================================================================
// TelemetryCollector Implementation
// ============================================================================

TelemetryCollector::TelemetryCollector() : cache_expiry_(std::chrono::system_clock::now()) {
    // Register default sources (would be configured in production)
    AddSource({"hash_rental_api", "https://api.nicehash.com/api", "", "json", std::chrono::hours(1), true});
    AddSource({"exchange_api", "https://api.coingecko.com/api/v3", "", "json", std::chrono::hours(1), false});
    AddSource({"bandwidth_provider", "internal", "", "json", std::chrono::hours(1), false});
}

void TelemetryCollector::AddSource(const TelemetrySource& source) {
    sources_[source.name] = source;
}

std::vector<TelemetryRecord> TelemetryCollector::CollectForScenario(
    const std::string& scenario_id,
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) {
    
    std::vector<TelemetryRecord> all_records;
    
    for (const auto& [name, source] : sources_) {
        auto records = FetchFromSource(source, start, end);
        all_records.insert(all_records.end(), records.begin(), records.end());
    }
    
    return all_records;
}

std::vector<TelemetryAggregate> TelemetryCollector::GetAggregates(
    const std::string& metric,
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) {
    
    std::vector<TelemetryAggregate> aggregates;
    // Implementation would aggregate cached records
    return aggregates;
}

bool TelemetryCollector::IsSourceAvailable(const std::string& source_name) const {
    auto it = sources_.find(source_name);
    if (it == sources_.end()) return false;
    
    // Check cache freshness
    auto now = std::chrono::system_clock::now();
    return now < cache_expiry_;
}

std::vector<TelemetryCollector::MissingTelemetryReport> TelemetryCollector::CheckAvailability(
    const std::vector<std::string>& required_metrics) {
    
    std::vector<MissingTelemetryReport> reports;
    
    for (const auto& metric : required_metrics) {
        MissingTelemetryReport report;
        report.metric = metric;
        
        bool found = false;
        for (const auto& [name, source] : sources_) {
            if (source.format == "json") {
                // Check if this source could provide the metric
                // Simplified: assume all sources can provide all metrics
                found = true;
                break;
            }
        }
        
        if (!found) {
            report.reason = "UNAVAILABLE";
            report.conservative_default = std::to_string(GetConservativeDefault(metric));
            report.notes = "No configured source for metric";
        } else {
            report.reason = "AVAILABLE";
            report.conservative_default = std::to_string(GetConservativeDefault(metric));
            report.notes = "Source available";
        }
        reports.push_back(report);
    }
    
    return reports;
}

std::unordered_map<std::string, double> TelemetryCollector::ApplyConservativeDefaults(
    const std::vector<std::string>& required_metrics) {
    
    std::unordered_map<std::string, double> defaults;
    for (const auto& metric : required_metrics) {
        defaults[metric] = GetConservativeDefault(metric);
    }
    return defaults;
}

std::vector<TelemetryRecord> TelemetryCollector::FetchFromSource(
    const TelemetrySource& source,
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) {
    
    // CE1: This is a stub. In production, this would make HTTP requests.
    // For CE1 build, we return synthetic data with clear markers.
    
    std::vector<TelemetryRecord> records;
    TelemetryRecord record;
    record.source = source.name;
    record.metric = "hash_rental_rate_usd_per_th";
    record.value = 1.0;  // Conservative default
    record.timestamp = std::chrono::system_clock::now();
    record.units = "USD/TH/day";
    record.metadata["note"] = "CE1_SYNTHETIC_DATA";
    
    records.push_back(record);
    return records;
}

std::optional<TelemetryRecord> TelemetryCollector::ValidateRecord(
    const TelemetryRecord& record) {
    
    if (record.source.empty() || record.metric.empty()) return std::nullopt;
    if (std::isnan(record.value) || std::isinf(record.value)) return std::nullopt;
    if (record.value < 0) return std::nullopt;
    
    return record;
}

double TelemetryCollector::GetConservativeDefault(const std::string& metric) const {
    static const std::unordered_map<std::string, double> defaults = {
        {"hash_rental_rate_usd_per_th", 1.0},
        {"exchange_rate_usd_per_ltc", 50.0},
        {"bandwidth_cost_usd_per_gb", 0.01},
        {"capital_cost_annual_rate", 0.10}
    };
    
    auto it = defaults.find(metric);
    if (it != defaults.end()) return it->second;
    return 0.0;
}

// ============================================================================
// MissingTelemetryHandler Implementation
// ============================================================================

MissingTelemetryHandler::MissingTelemetryHandler() {}

void MissingTelemetryHandler::ReportMissing(const MissingTelemetryEntry& entry) {
    missing_.push_back(entry);
}

const std::vector<MissingTelemetryEntry>& MissingTelemetryHandler::GetMissing() const {
    return missing_;
}

std::unordered_map<std::string, double> MissingTelemetryHandler::ApplyDefaults(
    const std::unordered_map<std::string, double>& metrics) {
    
    std::unordered_map<std::string, double> result = metrics;
    
    for (const auto& [metric, default_val] : kConservativeDefaults) {
        if (result.find(metric) == result.end()) {
            result[metric] = default_val;
        }
    }
    return result;
}

std::string MissingTelemetryHandler::GenerateReport() const {
    std::ostringstream oss;
    oss << "=== MISSING TELEMETRY REPORT ===\n";
    oss << "Total missing entries: " << missing_.size() << "\n\n";
    
    for (const auto& entry : missing_) {
        oss << "Metric: " << entry.metric << "\n";
        oss << "  Reason: " << entry.reason << "\n";
        oss << "  Conservative Default: " << entry.conservative_default << "\n";
        oss << "  Documentation: " << entry.documentation << "\n";
        oss << "  Affects Conformance: " << (entry.affects_conformance ? "YES" : "NO") << "\n\n";
    }
    
    oss << "Conservative Defaults Applied:\n";
    for (const auto& [metric, val] : kConservativeDefaults) {
        oss << "  " << metric << " = " << val << "\n";
    }
    
    return oss.str();
}

bool MissingTelemetryHandler::HasCriticalMissing() const {
    for (const auto& entry : missing_) {
        if (entry.affects_conformance) return true;
    }
    return false;
}

} // namespace msf