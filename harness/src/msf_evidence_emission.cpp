// MSF Evidence Emission Implementation (H7) - CE1 Phase 1.3
//
// Deterministic evidence manifest emission with hashing, serialization, and replay.
// Authority class: EVIDENCE_OUTPUT

#include "msf_evidence_emission.h"
#include <filesystem>
#include <algorithm>

namespace msf {

EvidenceManager::EvidenceManager(const std::string& evidence_dir) 
    : evidence_dir_(evidence_dir) {
    std::filesystem::create_directories(evidence_dir_);
}

EvidenceManifest EvidenceManager::CreateManifest(
    const std::string& task_id,
    const std::string& authority_envelope_hash,
    const std::string& authority_class) {
    
    EvidenceManifest manifest;
    manifest.task_id = task_id;
    manifest.authority_envelope_hash = authority_envelope_hash;
    manifest.H_MSF = "a855e7f17cc73f5b31979ee0b9b8f1ee532e3315a55edcf6a30cb702ee14ffd7";
    manifest.authority_class = authority_class;
    manifest.timestamps.push_back(GetCurrentTimestamp());
    manifest.repo_provenance = "litenyx@main";
    manifest.executable_revision = "CE1";
    manifest.environment = "CE1-build";
    manifest.configuration = "MSF-CONTRACT-SPEC-3";
    return manifest;
}

void EvidenceManager::AddEntry(
    EvidenceManifest& manifest,
    const std::string& key,
    const std::string& value) {
    
    EvidenceEntry entry;
    entry.key = key;
    entry.value = value;
    entry.hash = ComputeSHA256(value);
    entry.timestamp = GetCurrentTimestamp();
    manifest.raw_observations.push_back(std::move(entry));
    manifest.timestamps.push_back(GetCurrentTimestamp());
}

void EvidenceManager::AddTestResult(
    EvidenceManifest& manifest,
    const TestResult& result) {
    
    TestResult r = result;
    r.timestamp = GetCurrentTimestamp();
    manifest.test_results.push_back(std::move(r));
    manifest.timestamps.push_back(GetCurrentTimestamp());
}

void EvidenceManager::AddNegativeControlResult(
    EvidenceManifest& manifest,
    const TestResult& result) {
    
    TestResult r = result;
    r.timestamp = GetCurrentTimestamp();
    manifest.negative_control_results.push_back(std::move(r));
    manifest.timestamps.push_back(GetCurrentTimestamp());
}

void EvidenceManager::AddCommandRecord(
    EvidenceManifest& manifest,
    const CommandRecord& record) {
    
    CommandRecord r = record;
    r.timestamp = GetCurrentTimestamp();
    manifest.commands.push_back(std::move(r));
    manifest.timestamps.push_back(GetCurrentTimestamp());
}

void EvidenceManager::AddError(
    EvidenceManifest& manifest,
    const std::string& error) {
    
    manifest.errors.push_back(error);
    manifest.timestamps.push_back(GetCurrentTimestamp());
}

std::string EvidenceManager::FinalizeManifest(EvidenceManifest& manifest) {
    manifest.manifest_hash = ComputeManifestHash(manifest);
    manifest.timestamps.push_back(GetCurrentTimestamp());
    return manifest.manifest_hash;
}

bool EvidenceManager::SaveManifest(
    const EvidenceManifest& manifest,
    const std::string& filename) {
    
    std::filesystem::path path = evidence_dir_ / filename;
    std::ofstream ofs(path);
    if (!ofs) return false;
    
    // Write as JSON (simplified)
    ofs << SerializeManifest(manifest);
    return ofs.good();
}

EvidenceManifest EvidenceManager::LoadManifest(const std::string& filename) {
    std::filesystem::path path = evidence_dir_ / filename;
    std::ifstream ifs(path);
    if (!ifs) return EvidenceManifest{};
    
    std::string content((std::istreambuf_iterator<char>(ifs)), 
                        std::istreambuf_iterator<char>());
    return DeserializeManifest(content);
}

bool EvidenceManager::VerifyManifestIntegrity(const EvidenceManifest& manifest) const {
    std::string computed = ComputeManifestHash(manifest);
    return computed == manifest.manifest_hash;
}

bool EvidenceManager::VerifyManifestChain(
    const EvidenceManifest& current,
    const EvidenceManifest& previous) const {
    
    if (current.previous_manifest_hash.empty()) return true; // Genesis
    return current.previous_manifest_hash == previous.manifest_hash;
}

std::string EvidenceManager::ComputeManifestHash(const EvidenceManifest& manifest) const {
    std::string serialized = SerializeManifest(manifest);
    return ComputeSHA256(serialized);
}

std::vector<EvidenceManifest> EvidenceManager::GetManifestsForTask(
    const std::string& task_id) const {
    
    std::vector<EvidenceManifest> manifests;
    for (const auto& entry : std::filesystem::directory_iterator(evidence_dir_)) {
        if (entry.path().extension() == ".json") {
            std::string filename = entry.path().filename().string();
            if (filename.find(task_id) != std::string::npos) {
                manifests.push_back(LoadManifest(filename));
            }
        }
    }
    return manifests;
}

EvidenceManifest EvidenceManager::GetLatestManifest(const std::string& task_id) {
    auto manifests = GetManifestsForTask(task_id);
    if (manifests.empty()) return EvidenceManifest{};
    
    // Sort by timestamp (last entry)
    std::sort(manifests.begin(), manifests.end(), 
        [](const EvidenceManifest& a, const EvidenceManifest& b) {
            if (a.timestamps.empty() || b.timestamps.empty()) return false;
            return a.timestamps.back() > b.timestamps.back();
        });
    return manifests[0];
}

bool EvidenceManager::CompareManifests(
    const EvidenceManifest& a,
    const EvidenceManifest& b) const {
    
    return a.manifest_hash == b.manifest_hash;
}

std::string EvidenceManager::GenerateReport(const EvidenceManifest& manifest) {
    std::ostringstream oss;
    oss << "=== EVIDENCE MANIFEST REPORT ===\n";
    oss << "Task ID: " << manifest.task_id << "\n";
    oss << "Authority Envelope Hash: " << manifest.authority_envelope_hash << "\n";
    oss << "H_MSF: " << manifest.H_MSF << "\n";
    oss << "Authority Class: " << manifest.authority_class << "\n";
    oss << "Commit Hash: " << manifest.commit_hash << "\n";
    oss << "Repo Provenance: " << manifest.repo_provenance << "\n";
    oss << "Executable Revision: " << manifest.executable_revision << "\n";
    oss << "Manifest Hash: " << manifest.manifest_hash << "\n";
    oss << "Previous Manifest Hash: " << manifest.previous_manifest_hash << "\n";
    oss << "Timestamps: " << manifest.timestamps.size() << "\n";
    oss << "Raw Observations: " << manifest.raw_observations.size() << "\n";
    oss << "Derived Quantities: " << manifest.derived_quantities.size() << "\n";
    oss << "Test Results: " << manifest.test_results.size() << " (passed: " 
        << std::count_if(manifest.test_results.begin(), manifest.test_results.end(),
                         [](const TestResult& t) { return t.passed; }) << ")\n";
    oss << "Negative Controls: " << manifest.negative_control_results.size() << "\n";
    oss << "Commands: " << manifest.commands.size() << "\n";
    oss << "Errors: " << manifest.errors.size() << "\n";
    oss << "Output Hashes: " << manifest.output_hashes.size() << "\n";
    oss << "Authority Class: " << manifest.authority_class << "\n";
    return oss.str();
}

bool EvidenceManager::ReplayVerify(
    const EvidenceManifest& manifest,
    const std::function<std::string(const std::string&)>& command_executor) {
    
    for (const auto& cmd : manifest.commands) {
        std::string output = command_executor(cmd.command);
        if (output != cmd.stdout_output) {
            return false;
        }
    }
    return true;
}

std::string EvidenceManager::ExecuteCommand(const std::string& command) const {
    // Placeholder - in CE1 this is not used for execution
    return "CE1_BUILD_ONLY";
}

std::string EvidenceManager::ComputeSHA256(const std::string& data) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), 
           data.length(), reinterpret_cast<unsigned char*>(&data));
    
    std::ostringstream oss;
    unsigned char hash_bytes[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), 
           data.length(), hash_bytes);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash_bytes[i]);
    }
    return oss.str();
}

std::string EvidenceManager::SerializeManifest(const EvidenceManifest& manifest) const {
    // Deterministic JSON serialization (simplified)
    std::ostringstream oss;
    oss << "{";
    oss << "\"task_id\":\"" << manifest.task_id << "\",";
    oss << "\"authority_envelope_hash\":\"" << manifest.authority_envelope_hash << "\",";
    oss << "\"H_MSF\":\"" << manifest.H_MSF << "\",";
    oss << "\"authority_class\":\"" << manifest.authority_class << "\",";
    oss << "\"commit_hash\":\"" << manifest.commit_hash << "\",";
    oss << "\"repo_provenance\":\"" << manifest.repo_provenance << "\",";
    oss << "\"executable_revision\":\"" << manifest.executable_revision << "\",";
    oss << "\"manifest_hash\":\"" << manifest.manifest_hash << "\",";
    oss << "\"previous_manifest_hash\":\"" << manifest.previous_manifest_hash << "\",";
    oss << "\"timestamp_count\":" << manifest.timestamps.size() << ",";
    oss << "\"raw_observations_count\":" << manifest.raw_observations.size() << ",";
    oss << "\"derived_quantities_count\":" << manifest.derived_quantities.size() << ",";
    oss << "\"test_results_count\":" << manifest.test_results.size() << ",";
    oss << "\"negative_control_results_count\":" << manifest.negative_control_results.size() << ",";
    oss << "\"command_count\":" << manifest.commands.size() << ",";
    oss << "\"error_count\":" << manifest.errors.size() << ",";
    oss << "\"output_hashes_count\":" << manifest.output_hashes.size();
    oss << "}";
    return oss.str();
}

EvidenceManifest EvidenceManager::DeserializeManifest(const std::string& data) const {
    // Simplified deserialization
    EvidenceManifest manifest;
    // Full JSON parsing would go here
    return manifest;
}

std::string EvidenceManager::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::vector<std::string> EvidenceManager::GetManifestFiles(const std::string& task_id) const {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(evidence_dir_)) {
        if (entry.path().extension() == ".json") {
            std::string filename = entry.path().filename().string();
            if (filename.find(task_id) != std::string::npos) {
                files.push_back(entry.path().string());
            }
        }
    }
    return files;
}

std::string EvidenceManager::HashFile(const std::string& filepath) const {
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs) return "";
    
    std::string content((std::istreambuf_iterator<char>(ifs)), 
                        std::istreambuf_iterator<char>());
    return ComputeSHA256(content);
}

} // namespace msf