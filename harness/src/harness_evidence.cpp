#include "harness_evidence.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>

namespace fs = std::filesystem;

namespace harness {

// ============================================================================
// EvidenceManager Implementation
// ============================================================================

EvidenceManager::EvidenceManager(const std::string& evidence_dir)
    : evidence_dir_(evidence_dir) {
    // Create evidence directory if it doesn't exist
    if (!fs::exists(evidence_dir_)) {
        fs::create_directories(evidence_dir_);
    }
}

EvidenceManifest EvidenceManager::CreateManifest(
    const std::string& task_id,
    const std::string& authority_envelope_hash,
    const std::string& H_MSF
) {
    EvidenceManifest manifest;
    manifest.task_id = task_id;
    manifest.authority_envelope_hash = authority_envelope_hash;
    manifest.H_MSF = H_MSF;
    manifest.timestamps.push_back(GetCurrentTimestamp());
    
    return manifest;
}

void EvidenceManager::AddEntry(
    EvidenceManifest& manifest,
    const std::string& key,
    const std::string& value
) {
    EvidenceEntry entry;
    entry.key = key;
    entry.value = value;
    entry.hash = ComputeSHA256(value);
    entry.timestamp = GetCurrentTimestamp();
    
    manifest.canonical_input_hashes.push_back(entry);
}

void EvidenceManager::AddTestResult(
    EvidenceManifest& manifest,
    const TestResult& result
) {
    manifest.test_results.push_back(result);
}

void EvidenceManager::AddNegativeControlResult(
    EvidenceManifest& manifest,
    const TestResult& result
) {
    manifest.negative_control_results.push_back(result);
}

void EvidenceManager::AddCommandRecord(
    EvidenceManifest& manifest,
    const CommandRecord& record
) {
    manifest.commands.push_back(record);
}

void EvidenceManager::AddError(
    EvidenceManifest& manifest,
    const std::string& error
) {
    manifest.errors.push_back(error);
}

std::string EvidenceManager::FinalizeManifest(EvidenceManifest& manifest) {
    // Compute manifest hash
    std::string hash = ComputeManifestHash(manifest);
    manifest.manifest_hash = hash;
    
    // Save manifest
    std::string filename = evidence_dir_ + "/" + manifest.task_id + "_" + 
                          manifest.manifest_hash.substr(0, 8) + ".json";
    SaveManifest(manifest, filename);
    
    return manifest.manifest_hash;
}

bool EvidenceManager::SaveManifest(
    const EvidenceManifest& manifest,
    const std::string& filename
) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Serialize manifest to JSON
    std::string json = SerializeManifest(manifest);
    file << json;
    file.close();
    
    return true;
}

EvidenceManifest EvidenceManager::LoadManifest(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return EvidenceManifest();
    }
    
    std::string json((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    file.close();
    
    return DeserializeManifest(json);
}

bool EvidenceManager::VerifyManifestIntegrity(const EvidenceManifest& manifest) const {
    std::string computed_hash = ComputeManifestHash(manifest);
    return computed_hash == manifest.manifest_hash;
}

bool EvidenceManager::VerifyManifestChain(
    const EvidenceManifest& current,
    const EvidenceManifest& previous
) {
    return current.previous_manifest_hash == previous.manifest_hash;
}

std::string EvidenceManager::ComputeManifestHash(const EvidenceManifest& manifest) const {
    std::string serialized = SerializeManifest(manifest);
    return ComputeSHA256(serialized);
}

std::vector<EvidenceManifest> EvidenceManager::GetManifestsForTask(
    const std::string& task_id
) {
    std::vector<EvidenceManifest> manifests;
    
    for (const auto& entry : fs::directory_iterator(evidence_dir_)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find(task_id) == 0) {
                EvidenceManifest manifest = LoadManifest(entry.path().string());
                if (!manifest.task_id.empty()) {
                    manifests.push_back(manifest);
                }
            }
        }
    }
    
    // Sort by timestamp
    std::sort(manifests.begin(), manifests.end(),
        [](const EvidenceManifest& a, const EvidenceManifest& b) {
            if (a.timestamps.empty()) return true;
            if (b.timestamps.empty()) return false;
            return a.timestamps.front() < b.timestamps.front();
        });
    
    return manifests;
}

EvidenceManifest EvidenceManager::GetLatestManifest(const std::string& task_id) {
    auto manifests = GetManifestsForTask(task_id);
    if (manifests.empty()) {
        return EvidenceManifest();
    }
    return manifests.back();
}

bool EvidenceManager::CompareManifests(
    const EvidenceManifest& a,
    const EvidenceManifest& b
) {
    return a.manifest_hash == b.manifest_hash;
}

std::string EvidenceManager::GenerateReport(const EvidenceManifest& manifest) {
    std::ostringstream report;
    
    report << "=== Evidence Report ===" << std::endl;
    report << "Task ID: " << manifest.task_id << std::endl;
    report << "H_MSF: " << manifest.H_MSF << std::endl;
    report << "Manifest Hash: " << manifest.manifest_hash << std::endl;
    report << std::endl;
    
    report << "=== Tests ===" << std::endl;
    for (const auto& test : manifest.test_results) {
        report << "  " << test.test_name << ": " 
               << (test.passed ? "PASS" : "FAIL") << std::endl;
        if (!test.passed) {
            report << "    Expected: " << test.expected << std::endl;
            report << "    Actual: " << test.actual << std::endl;
        }
    }
    report << std::endl;
    
    report << "=== Negative Controls ===" << std::endl;
    for (const auto& control : manifest.negative_control_results) {
        report << "  " << control.test_name << ": "
               << (control.passed ? "PASS" : "FAIL") << std::endl;
    }
    report << std::endl;
    
    report << "=== Errors ===" << std::endl;
    for (const auto& error : manifest.errors) {
        report << "  " << error << std::endl;
    }
    
    return report.str();
}

// ============================================================================
// Private Methods
// ============================================================================

std::string EvidenceManager::ExecuteCommand(const std::string& command) const {
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return result;
}

std::string EvidenceManager::ComputeSHA256(const std::string& data) const {
    // Use certutil on Windows
    std::string command = "echo " + data + " | certutil -hashfile - SHA256";
    std::string output = ExecuteCommand(command);
    
    // Extract hash from output
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.length() == 64) {  // SHA-256 hash is 64 hex characters
            return line;
        }
    }
    
    return "";
}

std::string EvidenceManager::SerializeManifest(const EvidenceManifest& manifest) const {
    std::ostringstream json;
    
    json << "{" << std::endl;
    json << "  \"task_id\": \"" << manifest.task_id << "\"," << std::endl;
    json << "  \"authority_envelope_hash\": \"" << manifest.authority_envelope_hash << "\"," << std::endl;
    json << "  \"H_MSF\": \"" << manifest.H_MSF << "\"," << std::endl;
    json << "  \"manifest_hash\": \"" << manifest.manifest_hash << "\"," << std::endl;
    json << "  \"timestamp\": \"" << (manifest.timestamps.empty() ? std::string("unknown") : manifest.timestamps.front()) << "\"," << std::endl;
    
    json << "  \"test_results\": [" << std::endl;
    for (size_t i = 0; i < manifest.test_results.size(); ++i) {
        const auto& test = manifest.test_results[i];
        json << "    {" << std::endl;
        json << "      \"test_name\": \"" << test.test_name << "\"," << std::endl;
        json << "      \"passed\": " << (test.passed ? "true" : "false") << "," << std::endl;
        json << "      \"details\": \"" << test.details << "\"" << std::endl;
        json << "    }" << (i < manifest.test_results.size() - 1 ? "," : "") << std::endl;
    }
    json << "  ]," << std::endl;
    
    json << "  \"negative_control_results\": [" << std::endl;
    for (size_t i = 0; i < manifest.negative_control_results.size(); ++i) {
        const auto& control = manifest.negative_control_results[i];
        json << "    {" << std::endl;
        json << "      \"test_name\": \"" << control.test_name << "\"," << std::endl;
        json << "      \"passed\": " << (control.passed ? "true" : "false") << "," << std::endl;
        json << "      \"details\": \"" << control.details << "\"" << std::endl;
        json << "    }" << (i < manifest.negative_control_results.size() - 1 ? "," : "") << std::endl;
    }
    json << "  ]," << std::endl;
    
    json << "  \"errors\": [" << std::endl;
    for (size_t i = 0; i < manifest.errors.size(); ++i) {
        json << "    \"" << manifest.errors[i] << "\"" 
             << (i < manifest.errors.size() - 1 ? "," : "") << std::endl;
    }
    json << "  ]" << std::endl;
    
    json << "}" << std::endl;
    
    return json.str();
}

EvidenceManifest EvidenceManager::DeserializeManifest(const std::string& data) const {
    // Simplified deserialization - in production, use a proper JSON parser
    EvidenceManifest manifest;
    
    // For now, just extract task_id and manifest_hash
    size_t task_pos = data.find("\"task_id\":");
    if (task_pos != std::string::npos) {
        size_t start = data.find("\"", task_pos + 11) + 1;
        size_t end = data.find("\"", start);
        manifest.task_id = data.substr(start, end - start);
    }
    
    size_t hash_pos = data.find("\"manifest_hash\":");
    if (hash_pos != std::string::npos) {
        size_t start = data.find("\"", hash_pos + 17) + 1;
        size_t end = data.find("\"", start);
        manifest.manifest_hash = data.substr(start, end - start);
    }
    
    return manifest;
}

std::string EvidenceManager::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime(&time));
    return std::string(buffer);
}

// ============================================================================
// EvidenceSchemaValidator Implementation
// ============================================================================

bool EvidenceSchemaValidator::ValidateManifest(
    const EvidenceManifest& manifest,
    const std::string& schema_name
) {
    // Get required fields for this schema
    auto required_fields = GetRequiredFields(schema_name);
    
    // Check that all required fields are present
    // This is a simplified check - in production, use a proper schema validator
    if (manifest.task_id.empty()) return false;
    if (manifest.H_MSF.empty()) return false;
    if (manifest.manifest_hash.empty()) return false;
    
    return true;
}

bool EvidenceSchemaValidator::ValidateEntry(const EvidenceEntry& entry) {
    return !entry.key.empty() && !entry.value.empty();
}

bool EvidenceSchemaValidator::ValidateTestResult(const TestResult& result) {
    return !result.test_name.empty();
}

bool EvidenceSchemaValidator::ValidateCommandRecord(const CommandRecord& record) {
    return !record.command.empty();
}

std::vector<std::string> EvidenceSchemaValidator::GetRequiredFields(
    const std::string& schema_name
) {
    if (schema_name == "state-generation-manifest") {
        return {"task_id", "H_MSF", "manifest_hash", "state-hash", "utxo-root"};
    } else if (schema_name == "work-execution-manifest") {
        return {"task_id", "H_MSF", "manifest_hash", "validation-result", "work-hash"};
    } else if (schema_name == "fork-simulation-manifest") {
        return {"task_id", "H_MSF", "manifest_hash", "competing-history-hashes", "reorg-depth"};
    } else if (schema_name == "telemetry-manifest") {
        return {"task_id", "H_MSF", "manifest_hash", "telemetry-sources", "aggregation-rules"};
    } else if (schema_name == "missing-data-manifest") {
        return {"task_id", "H_MSF", "manifest_hash", "missing-data-report"};
    } else if (schema_name == "anti-gaming-manifest") {
        return {"task_id", "H_MSF", "manifest_hash", "mutation-results", "gameability-detection"};
    } else if (schema_name == "evidence-manifest-schema") {
        return {"task_id", "H_MSF", "manifest_hash", "evidence-manifest-hash"};
    } else if (schema_name == "verification-manifest") {
        return {"task_id", "H_MSF", "manifest_hash", "verification-result"};
    }
    
    return {"task_id", "H_MSF", "manifest_hash"};
}

std::vector<std::string> EvidenceSchemaValidator::GetOptionalFields(
    const std::string& schema_name
) {
    return {"environment", "configuration", "timestamps", "errors"};
}

} // namespace harness
