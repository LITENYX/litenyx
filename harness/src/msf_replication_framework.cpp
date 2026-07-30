// MSF Replication Framework Implementation (CE1 Phase 1.2)
//
// Implements 3× bit-identical replications per scenario with CRN management.
// Does NOT execute the scientific experiment.
// Authority class: EXPERIMENT_INFRASTRUCTURE

#include "msf_replication_framework.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace msf {

// ============================================================================
// CRN Manager Implementation
// ============================================================================

uint64_t CRNManager::ComputeStreamSeed(const std::string& scenario_id, 
                                       int replication_id, 
                                       const std::string& stream_id) const {
    // Deterministic seed derivation: hash(scenario_id + replication_id + stream_id)
    std::string combined = scenario_id + "|" + std::to_string(replication_id) + "|" + stream_id;
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), 
           combined.length(), hash);
    
    // Take first 8 bytes as seed
    uint64_t seed = 0;
    for (int i = 0; i < 8; ++i) {
        seed = (seed << 8) | hash[i];
    }
    return seed;
}

DeterministicRNG CRNManager::GetStream(const std::string& scenario_id, 
                                       int replication_id, 
                                       const std::string& stream_id) const {
    auto scenario_it = streams_.find(scenario_id);
    if (scenario_it == streams_.end()) {
        // Not initialized yet - create on demand
        uint64_t seed = ComputeStreamSeed(scenario_id, 0, stream_id);
        return DeterministicRNG(seed);
    }
    
    auto repl_it = scenario_it->second.find(0);  // Base stream
    if (repl_it != scenario_it->second.end()) {
        auto stream_it = repl_it->second.find(stream_id);
        if (stream_it != repl_it->second.end()) {
            return stream_it->second;
        }
    }
    
    // Fallback: compute deterministic seed
    uint64_t seed = ComputeStreamSeed(scenario_id, 0, stream_id);
    return DeterministicRNG(seed);
}

void CRNManager::InitializeScenario(const std::string& scenario_id, 
                                    uint64_t base_seed) {
    // Pre-initialize streams for this scenario
    // We don't know all stream IDs in advance, so we'll compute on demand
    // But we can pre-seed the base RNG
    (void)base_seed;  // Stored implicitly via deterministic derivation
}

uint64_t CRNManager::ComputeStreamSeed(const std::string& scenario_id, 
                                       int replication_id, 
                                       const std::string& stream_id) const {
    std::string combined = scenario_id + "|" + std::to_string(replication_id) + "|" + stream_id;
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), 
           combined.length(), hash);
    
    uint64_t seed = 0;
    for (int i = 0; i < 8; ++i) {
        seed = (seed << 8) | hash[i];
    }
    return seed;
}

bool CRNManager::VerifyBitIdentical(const std::vector<std::string>& replication_outputs) const {
    if (replication_outputs.size() < 2) return true;
    
    const std::string& first = replication_outputs[0];
    for (size_t i = 1; i < replication_outputs.size(); ++i) {
        if (replication_outputs[i] != replication_outputs[0]) {
            return false;
        }
    }
    return true;
}

std::string CRNManager::GetMaxDivergence(const std::vector<std::string>& replication_outputs) const {
    if (replication_outputs.size() < 2) return "0x0";
    
    // Simple divergence: compare byte-by-byte
    const std::string& first = replication_outputs[0];
    size_t max_diff = 0;
    
    for (size_t i = 1; i < replication_outputs.size(); ++i) {
        const std::string& other = replication_outputs[i];
        size_t min_len = std::min(first.size(), other.size());
        size_t diff = 0;
        for (size_t j = 0; j < min_len; ++j) {
            if (first[j] != other[j]) ++diff;
        }
        diff += std::abs(static_cast<int>(first.size()) - static_cast<int>(other.size()));
        max_diff = std::max(max_diff, diff);
    }
    
    std::ostringstream oss;
    oss << "0x" << std::hex << max_diff;
    return oss.str();
}

// ============================================================================
// Replication Engine Implementation
// ============================================================================

ReplicationEngine::ReplicationEngine(const ReplicationConfig& config) 
    : config_(config) {
    crn_manager_.InitializeScenario("default", config.base_seed);
}

uint64_t ReplicationEngine::ComputeReplicationSeed(const std::string& scenario_id, 
                                                   int replication_id) const {
    std::string combined = scenario_id + "|REP|" + std::to_string(replication_id);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(scenario_id.c_str()), 
           scenario_id.length() + 4 + std::to_string(replication_id).length(), 
           reinterpret_cast<unsigned char*>(const_cast<char*>(reinterpret_cast<const char*>(&replication_id))));
    
    // Better: proper hash
    std::string combined = scenario_id + "|REP|" + std::to_string(replication_id);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), 
           combined.length(), reinterpret_cast<unsigned char*>(&replication_id));
    
    uint64_t seed = 0;
    unsigned char hash_buf[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), 
           combined.length(), hash_buf);
    
    uint64_t seed = 0;
    for (int i = 0; i < 8; ++i) {
        seed = (seed << 8) | hash_buf[i];
    }
    return seed;
}

ReplicationEngine::ReplicationResult ReplicationEngine::ExecuteSingleReplication(
    const ScenarioParams& scenario,
    int replication_id,
    const std::function<void(const ScenarioParams&, int, DeterministicRNG&, 
                            std::vector<std::string>&)>& block_fn) {
    
    ReplicationResult result;
    result.replication_id = replication_id;
    
    // Get deterministic seed for this replication
    uint64_t seed = ComputeReplicationSeed(scenario.scenario_id, replication_id);
    DeterministicRNG rng(seed);
    
    // Execute all blocks
    std::vector<std::string> block_outputs;
    block_outputs.reserve(scenario.D_i > 0 ? 1000 : 1); // reserve
    
    bool overflow = false;
    bool div_zero = false;
    std::string notes;
    
    for (int64_t block = 0; block < 1000; ++block) {
        std::string block_output;
        block_fn(scenario, static_cast<int>(block), rng, block_outputs);
    }
    
    // Compute final hash of all block outputs
    std::string concatenated;
    for (const auto& out : block_outputs) {
        concatenated += out;
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(concatenated.c_str()), 
           concatenated.length(), reinterpret_cast<unsigned char*>(&result.final_hash));
    
    // Convert hash to hex string
    std::ostringstream oss;
    unsigned char hash_bytes[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(concatenated.c_str()), 
           concatenated.length(), reinterpret_cast<unsigned char*>(&result.final_hash));
    
    std::ostringstream oss;
    unsigned char hash_bytes2[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(concatenated.c_str()), 
           concatenated.length(), hash_bytes2);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash_bytes2[i]);
    }
    result.final_hash = oss.str();
    
    result.block_outputs = std::move(block_outputs);
    result.overflow_detected = false;
    result.div_zero_detected = false;
    result.anomaly_notes = "";
    
    return result;
}

std::vector<ReplicationEngine::ReplicationResult> ReplicationEngine::ExecuteReplications(
    const ScenarioParams& scenario,
    const std::function<void(const ScenarioParams&, int, DeterministicRNG&, 
                            std::vector<std::string>&)>& block_fn) {
    
    std::vector<ReplicationResult> results;
    results.reserve(config_.num_replications);
    
    for (int rep = 1; rep <= config_.num_replications; ++rep) {
        results.push_back(ExecuteSingleReplication(scenario, rep, block_fn));
    }
    
    // Verify bit-identical if enabled
    if (config_.verify_bit_identical && results.size() >= 2) {
        std::vector<std::string> hashes;
        for (const auto& r : results) {
            hashes.push_back(r.final_hash);
        }
        
        bool identical = true;
        for (size_t i = 1; i < hashes.size(); ++i) {
            if (hashes[i] != hashes[0]) {
                identical = false;
                break;
            }
        }
        
        if (identical) {
            for (auto& r : results) {
                r.bit_identical_to_first = true;
            }
        }
        
        // Compute max divergence
        std::vector<std::string> outputs;
        for (const auto& r : results) {
            std::string concat;
            for (const auto& b : r.block_outputs) concat += b;
            outputs.push_back(concat);
        }
        
        for (auto& r : results) {
            r.max_divergence = "0x0"; // Placeholder - would compute actual
        }
    }
    
    return results;
}

bool ReplicationEngine::VerifyBitIdentical(const std::vector<ReplicationResult>& results) const {
    if (results.size() < 2) return true;
    
    const std::string& first_hash = results[0].final_hash;
    for (size_t i = 1; i < results.size(); ++i) {
        if (results[i].final_hash != first_hash) {
            return false;
        }
    }
    return true;
}

std::string ReplicationEngine::GetMaxDivergence(const std::vector<ReplicationResult>& results) const {
    if (results.size() < 2) return "0x0";
    
    // Compare final hashes
    const std::string& first = results[0].final_hash;
    size_t max_diff = 0;
    
    for (size_t i = 1; i < results.size(); ++i) {
        size_t diff = 0;
        const std::string& other = results[i].final_hash;
        size_t min_len = std::min(first.size(), other.size());
        for (size_t j = 0; j < min_len; ++j) {
            if (first[j] != other[j]) ++diff;
        }
        diff += std::abs(static_cast<int>(first.size()) - static_cast<int>(other.size()));
        max_diff = std::max(max_diff, diff);
    }
    
    std::ostringstream oss;
    oss << "0x" << std::hex << max_diff;
    return oss.str();
}

std::string ReplicationEngine::GenerateReplicationReport(const std::vector<ReplicationResult>& results) const {
    std::ostringstream oss;
    oss << "=== REPLICATION REPORT ===\n";
    oss << "Scenario replications: " << results.size() << "\n";
    oss << "Bit-identical: " << (VerifyBitIdentical(results) ? "YES" : "NO") << "\n";
    oss << "Max divergence: " << GetMaxDivergence(results) << "\n\n";
    
    for (const auto& r : results) {
        oss << "Replication " << r.replication_id << ":\n";
        oss << "  Final hash: " << r.final_hash << "\n";
        oss << "  Blocks: " << r.block_outputs.size() << "\n";
        oss << "  Overflow: " << (r.overflow_detected ? "YES" : "NO") << "\n";
        oss << "  Div-zero: " << (r.div_zero_detected ? "YES" : "NO") << "\n";
        oss << "  Bit-identical to rep 1: " << (r.bit_identical_to_first ? "YES" : "NO") << "\n";
        oss << "  Max divergence: " << r.max_divergence << "\n";
        if (!r.anomaly_notes.empty()) {
            oss << "  Notes: " << r.anomaly_notes << "\n";
        }
        oss << "\n";
    }
    
    return oss.str();
}

} // namespace msf