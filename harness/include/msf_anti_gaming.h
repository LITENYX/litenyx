// MSF Anti-Gaming Framework (H6) - CE1 Phase 1.6
//
// Metric mutation testing and gameability detection.
// Authority class: EXPERIMENT_INFRASTRUCTURE (EXECUTABLE)

#ifndef MSF_ANTI_GAMING_H
#define MSF_ANTI_GAMING_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>
#include <random>

namespace msf {

// ============================================================================
// Anti-Gaming Mutation Types
// ============================================================================

enum class MutationType {
    SCALAR_MULTIPLY,      // Multiply metric by factor
    SCALAR_ADD,           // Add constant to metric
    SIGN_FLIP,            // Negate metric
    NOISE_INJECTION,      // Add Gaussian noise
    ZERO_OUT,             // Set to zero
    MAX_CLAMP,            // Clamp to maximum
    SWAP_METRICS,         // Swap two metric values
    TIME_SHIFT,           // Shift time series
    REPLICATION_SWAP      // Swap replication outputs
};

struct MutationSpec {
    MutationType type;
    std::string target_metric;
    double factor = 1.0;          // For SCALAR_MULTIPLY
    double addend = 0.0;          // For SCALAR_ADD
    double noise_stddev = 0.0;    // For NOISE_INJECTION
    double clamp_value = 0.0;     // For MAX_CLAMP
    std::string swap_metric;      // For SWAP_METRICS
    int time_shift_blocks = 0;    // For TIME_SHIFT
    int source_replication = 0;   // For REPLICATION_SWAP
    int target_replication = 0;
};

// ============================================================================
// Gameability Test Result
// ============================================================================

struct GameabilityResult {
    std::string metric;
    MutationType mutation;
    double original_value;
    double mutated_value;
    bool verdict_changed;           // Did the hypothesis verdict change?
    std::string original_verdict;
    std::string mutated_verdict;
    double sensitivity;             // |Δverdict| / |Δmetric|
    bool statistically_significant = false;
    double p_value = 1.0;
    int sample_size = 0;
    std::string notes;
};

// ============================================================================
// H6: Anti-Gaming Executor
// ============================================================================

class AntiGamingExecutor {
public:
    AntiGamingExecutor();
    
    // Apply mutation to experimental results
    struct ExperimentResults {
        std::unordered_map<std::string, double> metrics;  // metric -> value
        std::unordered_map<std::string, std::string> verdicts;  // hypothesis -> verdict
    };
    
    ExperimentResults ApplyMutation(
        const ExperimentResults& original,
        const MutationSpec& mutation);
    
    // Run gameability test for a metric
    GameabilityResult TestGameability(
        const ExperimentResults& baseline,
        const std::string& metric,
        MutationType mutation_type,
        double magnitude);
    
    // Run comprehensive gameability suite
    std::vector<GameabilityResult> RunGameabilitySuite(
        const ExperimentResults& baseline,
        const std::vector<std::string>& metrics,
        const std::vector<MutationType>& mutations = {});
    
    // Statistical significance test
    struct SignificanceTest {
        double p_value;
        double effect_size;
        bool significant;
        int degrees_of_freedom;
    };
    SignificanceTest TestSignificance(
        const std::vector<double>& original_values,
        const std::vector<double>& mutated_values);
    
    // Predefined mutation suites
    static std::vector<MutationSpec> GetStandardMutations(const std::string& metric);
    static std::vector<MutationSpec> GetExhaustiveMutations(const std::string& metric);

private:
    // Apply specific mutation type
    double ApplyScalarMultiply(double value, double factor);
    double ApplyScalarAdd(double value, double addend);
    double ApplySignFlip(double value);
    double ApplyNoiseInjection(double value, double stddev);
    double ApplyZeroOut(double value);
    double ApplyMaxClamp(double value, double clamp);
    
    // Verify result integrity
    bool VerifyIntegrity(const ExperimentResults& results) const;
    
    // Compute sensitivity
    double ComputeSensitivity(
        double original_verdict_score,
        double mutated_verdict_score,
        double original_metric,
        double mutated_metric);
    
    // Convert verdict to numeric score for sensitivity
    double VerdictToScore(const std::string& verdict) const;
};

// ============================================================================
// Predefined Mutation Suites
// ============================================================================

inline std::vector<MutationSpec> AntiGamingExecutor::GetStandardMutations(const std::string& metric) {
    return {
        {MutationType::SCALAR_MULTIPLY, metric, 1.1, 0, 0, 0, "", 0, 0, 0},  // +10%
        {MutationType::SCALAR_MULTIPLY, metric, 0.9, 0, 0, 0, "", 0, 0, 0},  // -10%
        {MutationType::SCALAR_MULTIPLY, metric, 2.0, 0, 0, 0, "", 0, 0, 0},  // 2x
        {MutationType::SCALAR_MULTIPLY, metric, 0.5, 0, 0, 0, "", 0, 0, 0},  // 0.5x
        {MutationType::SCALAR_ADD, metric, 1.0, 1.0, 0, 0, "", 0, 0, 0},     // +1
        {MutationType::SCALAR_ADD, metric, 1.0, -1.0, 0, 0, "", 0, 0, 0},    // -1
        {MutationType::NOISE_INJECTION, metric, 1.0, 0, 0.1, 0, "", 0, 0, 0}, // 10% noise
        {MutationType::ZERO_OUT, metric, 0, 0, 0, 0, "", 0, 0, 0},           // Zero
        {MutationType::SIGN_FLIP, metric, 0, 0, 0, 0, "", 0, 0, 0}           // Sign flip
    };
}

inline std::vector<MutationSpec> AntiGamingExecutor::GetExhaustiveMutations(const std::string& metric) {
    auto standard = GetStandardMutations(metric);
    
    // Add extreme mutations
    standard.push_back({MutationType::SCALAR_MULTIPLY, metric, 10.0, 0, 0, 0, "", 0, 0, 0});   // 10x
    standard.push_back({MutationType::SCALAR_MULTIPLY, metric, 0.1, 0, 0, 0, "", 0, 0, 0});   // 0.1x
    standard.push_back({MutationType::SCALAR_ADD, metric, 1.0, 100.0, 0, 0, "", 0, 0, 0});    // +100
    standard.push_back({MutationType::SCALAR_ADD, metric, 1.0, -100.0, 0, 0, "", 0, 0, 0});   // -100
    standard.push_back({MutationType::NOISE_INJECTION, metric, 1.0, 0, 0.5, 0, "", 0, 0, 0}); // 50% noise
    standard.push_back({MutationType::MAX_CLAMP, metric, 1.0, 0, 0, 1.0, "", 0, 0, 0});       // Clamp to 1
    standard.push_back({MutationType::MAX_CLAMP, metric, 1.0, 0, 0, 0.0, "", 0, 0, 0});       // Clamp to 0
    
    return standard;
}

} // namespace msf

#endif // MSF_ANTI_GAMING_H