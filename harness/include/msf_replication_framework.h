// MSF Replication Framework (CE1 Phase 1.2)
//
// Implements 3× bit-identical replications per scenario with CRN management.
// Does NOT execute the scientific experiment.
// Authority class: EXPERIMENT_INFRASTRUCTURE

#ifndef MSF_REPLICATION_FRAMEWORK_H
#define MSF_REPLICATION_FRAMEWORK_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>

#include "msf_scenario_engine.h"

namespace msf {

// ============================================================================
// Deterministic RNG (splitmix64 - CRN management)
// ============================================================================

class DeterministicRNG {
public:
    explicit DeterministicRNG(uint64_t seed = 0x9E3779B97F4A7C15ULL) : state_(seed) {}
    
    // Next 64-bit value
    uint64_t Next() {
        uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    
    // Uniform [0,1)
    double Uniform() {
        return static_cast<double>(Next() >> 11) * (1.0 / static_cast<double>(1ULL << 53));
    }
    
    // Uniform integer [0, max)
    uint64_t Uniform(uint64_t max) {
        return Next() % max;
    }
    
    // Get state for checkpointing
    uint64_t GetState() const { return state_; }
    
    // Set state for replay
    void SetState(uint64_t state) { state_ = state; }
    
private:
    uint64_t state_;
};

// ============================================================================
// CRN (Common Random Numbers) Manager
// ============================================================================

class CRNManager {
public:
    CRNManager() = default;
    
    // Initialize CRN streams for a scenario with given seed
    // Each (scenario, replication, stream_id) gets independent stream
    DeterministicRNG GetStream(const std::string& scenario_id, 
                               int replication_id, 
                               const std::string& stream_id) const;
    
    // Initialize all streams for a scenario's replications
    void InitializeScenario(const std::string& scenario_id, 
                            uint64_t base_seed);
    
    // Verify bit-identical replications
    bool VerifyBitIdentical(const std::vector<std::string>& replication_outputs) const;
    
    // Get max divergence between replications
    std::string GetMaxDivergence(const std::vector<std::string>& replication_outputs) const;

private:
    std::unordered_map<std::string, std::unordered_map<int, 
           std::unordered_map<std::string, DeterministicRNG>>> streams_;
    
    uint64_t ComputeStreamSeed(const std::string& scenario_id, 
                               int replication_id, 
                               const std::string& stream_id) const;
};

// ============================================================================
// Replication Engine
// ============================================================================

struct ReplicationConfig {
    int num_replications = 3;
    uint64_t base_seed = 0x9E3779B97F4A7C15ULL;
    bool verify_bit_identical = true;
};

struct ReplicationResult {
    int replication_id;                    // 1, 2, or 3
    std::vector<std::string> block_outputs; // Per-block serialized output
    std::string final_hash;                 // SHA-256 of all block outputs
    bool bit_identical_to_first = false;
    std::string max_divergence;             // Max divergence from replication 1
    bool overflow_detected = false;
    bool div_zero_detected = false;
    std::string anomaly_notes;
};

class ReplicationEngine {
public:
    explicit ReplicationEngine(const ReplicationConfig& config = ReplicationConfig());
    
    // Execute 3 replications of a scenario
    // Note: In CE1, this builds the execution plan; actual execution is Phase 4
    std::vector<ReplicationResult> ExecuteReplications(
        const ScenarioParams& scenario,
        const std::function<void(const ScenarioParams&, int, DeterministicRNG&, 
                                std::vector<std::string>&)>& block_fn);
    
    // Verify all replications are bit-identical
    bool VerifyBitIdentical(const std::vector<ReplicationResult>& results) const;
    
    // Get max divergence between replications
    std::string GetMaxDivergence(const std::vector<ReplicationResult>& results) const;
    
    // Generate replication report for evidence manifest
    std::string GenerateReplicationReport(const std::vector<ReplicationResult>& results) const;

private:
    ReplicationConfig config_;
    CRNManager crn_manager_;
    
    // Execute single replication
    ReplicationResult ExecuteSingleReplication(
        const ScenarioParams& scenario,
        int replication_id,
        const std::function<void(const ScenarioParams&, int, DeterministicRNG&, 
                                std::vector<std::string>&)>& block_fn);
    
    // Generate deterministic seed for (scenario, replication)
    uint64_t ComputeReplicationSeed(const std::string& scenario_id, int replication_id) const;
};

// ============================================================================
// Q32.32 Fixed-Point Helpers (matching LITENYX_fixed_point.h)
// ============================================================================

namespace q32_32 {

static constexpr uint64_t FRAC_BITS = 32;
static constexpr uint64_t ONE = 1ULL << FRAC_BITS;
static constexpr uint64_t MAX = 0xFFFFFFFFFFFFFFFFULL;

inline uint64_t FromDouble(double d) {
    if (d <= 0.0) return 0;
    if (d >= 4294967296.0) return MAX;
    return static_cast<uint64_t>(d * ONE);
}

inline double ToDouble(uint64_t bits) {
    return static_cast<double>(bits) / static_cast<double>(ONE);
}

inline uint64_t Add(uint64_t a, uint64_t b) {
    uint64_t result = a + b;
    if (result < a) return MAX;  // Overflow clamp
    return result;
}

inline uint64_t Sub(uint64_t a, uint64_t b) {
    if (a < b) return 0;  // Underflow clamp
    return a - b;
}

inline uint64_t Mul(uint64_t a, uint64_t b) {
    __uint128_t product = static_cast<__uint128_t>(a) * static_cast<__uint128_t>(b);
    uint64_t result = static_cast<uint64_t>(product >> FRAC_BITS);
    return result;
}

inline uint64_t Div(uint64_t a, uint64_t b) {
    if (b == 0) return MAX;  // Division by zero -> MAX (pessimistic)
    __uint128_t dividend = (static_cast<__uint128_t>(a) << FRAC_BITS);
    uint64_t result = static_cast<uint64_t>(dividend / b);
    return result;
}

inline uint64_t FromInt64(int64_t i) {
    return static_cast<uint64_t>(static_cast<uint64_t>(i) << FRAC_BITS);
}

inline int64_t ToInt64(uint64_t bits) {
    return static_cast<int64_t>(bits >> FRAC_BITS);
}

inline std::string ToHex(uint64_t bits) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << bits;
    return oss.str();
}

} // namespace q32_32

#endif // MSF_REPLICATION_FRAMEWORK_H