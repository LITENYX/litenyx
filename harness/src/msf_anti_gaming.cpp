// MSF Anti-Gaming Implementation (H6) - CE1 Phase 1.6
//
// Metric mutation testing and gameability detection.
// Authority class: EXPERIMENT_INFRASTRUCTURE (EXECUTABLE)

#include "msf_anti_gaming.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace msf {

AntiGamingExecutor::AntiGamingExecutor() {}

AntiGamingExecutor::ExperimentResults AntiGamingExecutor::ApplyMutation(
    const ExperimentResults& original,
    const MutationSpec& mutation) {
    
    ExperimentResults mutated = original;
    
    auto it = mutated.metrics.find(mutation.target_metric);
    if (it == mutated.metrics.end()) {
        // Metric not found - can't mutate
        return mutated;
    }
    
    double original_value = it->second;
    double mutated_value = original_value;
    
    switch (mutation.type) {
        case MutationType::SCALAR_MULTIPLY:
            mutated_value = ApplyScalarMultiply(original_value, mutation.factor);
            break;
        case MutationType::SCALAR_ADD:
            mutated_value = ApplyScalarAdd(original_value, mutation.addend);
            break;
        case MutationType::SIGN_FLIP:
            mutated_value = ApplySignFlip(original_value);
            break;
        case MutationType::NOISE_INJECTION:
            mutated_value = ApplyNoiseInjection(original_value, mutation.noise_stddev);
            break;
        case MutationType::ZERO_OUT:
            mutated_value = ApplyZeroOut(original_value);
            break;
        case MutationType::MAX_CLAMP:
            mutated_value = ApplyMaxClamp(original_value, mutation.clamp_value);
            break;
        default:
            // Unsupported mutation type
            return mutated;
    }
    
    it->second = mutated_value;
    
    // Re-evaluate verdicts based on mutated metrics
    // This is a simplified version - full implementation would re-run hypothesis evaluation
    mutated.verdicts = original.verdicts;
    
    return mutated;
}

msf::GameabilityResult AntiGamingExecutor::TestGameability(
    const ExperimentResults& baseline,
    const std::string& metric,
    MutationType mutation_type,
    double magnitude) {
    
    GameabilityResult result;
    result.metric = metric;
    result.mutation = mutation_type;
    result.original_value = baseline.metrics.count(metric) ? baseline.metrics.at(metric) : 0.0;
    
    // Create mutation spec
    MutationSpec mutation;
    mutation.type = mutation_type;
    mutation.target_metric = metric;
    
    switch (mutation_type) {
        case MutationType::SCALAR_MULTIPLY:
            mutation.factor = magnitude;
            break;
        case MutationType::SCALAR_ADD:
            mutation.addend = magnitude;
            break;
        case MutationType::NOISE_INJECTION:
            mutation.noise_stddev = magnitude;
            break;
        case MutationType::MAX_CLAMP:
            mutation.clamp_value = magnitude;
            break;
        default:
            mutation.factor = magnitude;
    }
    
    // Apply mutation
    ExperimentResults mutated = ApplyMutation(baseline, mutation);
    result.mutated_value = mutated.metrics.count(metric) ? mutated.metrics.at(metric) : 0.0;
    
    // Check if verdict changed
    std::string original_verdict = "UNKNOWN";
    std::string mutated_verdict = "UNKNOWN";
    
    if (baseline.verdicts.count("MSF-H1")) original_verdict = baseline.verdicts.at("MSF-H1");
    if (mutated.verdicts.count("MSF-H1")) mutated_verdict = mutated.verdicts.at("MSF-H1");
    
    result.original_verdict = original_verdict;
    result.mutated_verdict = mutated_verdict;
    result.verdict_changed = (original_verdict != mutated_verdict);
    
    // Compute sensitivity
    double original_score = VerdictToScore(original_verdict);
    double mutated_score = VerdictToScore(mutated_verdict);
    double metric_delta = std::abs(result.mutated_value - result.original_value);
    double score_delta = std::abs(mutated_score - original_score);
    
    if (metric_delta > 0) {
        result.sensitivity = score_delta / metric_delta;
    } else {
        result.sensitivity = 0.0;
    }
    
    // Statistical significance (would need multiple runs)
    result.statistically_significant = false;
    result.p_value = 1.0;
    result.sample_size = 1;
    result.notes = "Single mutation test; needs replication for statistical significance";
    
    return result;
}

std::vector<GameabilityResult> AntiGamingExecutor::RunGameabilitySuite(
    const ExperimentResults& baseline,
    const std::vector<std::string>& metrics,
    const std::vector<MutationType>& mutations) {
    
    std::vector<GameabilityResult> results;
    
    std::vector<MutationType> default_mutations = {
        MutationType::SCALAR_MULTIPLY,
        MutationType::SCALAR_ADD,
        MutationType::NOISE_INJECTION,
        MutationType::ZERO_OUT,
        MutationType::SIGN_FLIP,
        MutationType::MAX_CLAMP
    };
    
    const auto& test_mutations = mutations.empty() ? default_mutations : mutations;
    
    for (const auto& metric : metrics) {
        if (baseline.metrics.count(metric) == 0) continue;
        
        for (const auto& mutation_type : test_mutations) {
            // Test with standard magnitudes
            std::vector<double> magnitudes = {0.1, 0.5, 1.0, 2.0, 10.0, -1.0, -10.0};
            
            for (double mag : magnitudes) {
                GameabilityResult result = TestGameability(baseline, metric, mutation_type, mag);
                results.push_back(result);
            }
        }
    }
    
    return results;
}

AntiGamingExecutor::SignificanceTest AntiGamingExecutor::TestSignificance(
    const std::vector<double>& original_values,
    const std::vector<double>& mutated_values) {
    
    SignificanceTest result;
    result.p_value = 1.0;
    result.effect_size = 0.0;
    result.significant = false;
    result.degrees_of_freedom = 0;
    
    if (original_values.size() < 2 || mutated_values.size() < 2) {
        return result;
    }
    
    // Compute means
    double mean_orig = std::accumulate(original_values.begin(), original_values.end(), 0.0) / original_values.size();
    double mean_mut = std::accumulate(mutated_values.begin(), mutated_values.end(), 0.0) / mutated_values.size();
    
    // Compute variances
    double var_orig = 0, var_mut = 0;
    for (double v : original_values) var_orig += (v - mean_orig) * (v - mean_orig);
    for (double v : mutated_values) var_mut += (v - mean_mut) * (v - mean_mut);
    var_orig /= (original_values.size() - 1);
    var_mut /= (mutated_values.size() - 1);
    
    // Welch's t-test (unequal variances)
    double t_stat = (mean_orig - mean_mut) / std::sqrt(var_orig / original_values.size() + var_mut / mutated_values.size());
    double df = (var_orig / original_values.size() + var_mut / mutated_values.size());
    df = df * df / (
        (var_orig * var_orig) / (original_values.size() * original_values.size() * (original_values.size() - 1)) +
        (var_mut * var_mut) / (mutated_values.size() * mutated_values.size() * (mutated_values.size() - 1))
    );
    
    // Approximate p-value (two-tailed)
    double p = 2.0 * (1.0 - 0.5 * (1.0 + std::erf(std::abs(t_stat) / std::sqrt(2.0))));
    
    // Effect size (Cohen's d)
    double pooled_sd = std::sqrt((var_orig + var_mut) / 2.0);
    double d = (mean_orig - mean_mut) / pooled_sd;
    
    result.p_value = p;
    result.effect_size = d;
    result.significant = (p < 0.05);
    result.degrees_of_freedom = static_cast<int>(df);
    
    return result;
}

double AntiGamingExecutor::ApplyScalarMultiply(double value, double factor) {
    return value * factor;
}

double AntiGamingExecutor::ApplyScalarAdd(double value, double addend) {
    return value + addend;
}

double AntiGamingExecutor::ApplySignFlip(double value) {
    return -value;
}

double AntiGamingExecutor::ApplyNoiseInjection(double value, double stddev) {
    static std::mt19937 rng(0x9E3779B97F4A7C15ULL);
    std::normal_distribution<double> dist(0.0, stddev);
    return value + dist(rng);
}

double AntiGamingExecutor::ApplyZeroOut(double value) {
    return 0.0;
}

double AntiGamingExecutor::ApplyMaxClamp(double value, double clamp) {
    return std::min(value, clamp);
}

bool AntiGamingExecutor::VerifyIntegrity(const ExperimentResults& results) const {
    // Check that all required metrics exist
    static const std::vector<std::string> required_metrics = {
        "MSF-M01", "MSF-M02", "MSF-M03", "MSF-M04", "MSF-M05",
        "MSF-M06", "MSF-M07", "MSF-M08", "MSF-M09", "MSF-M10"
    };
    
    for (const auto& metric : required_metrics) {
        if (results.metrics.find(metric) == results.metrics.end()) {
            return false;
        }
    }
    return true;
}

double AntiGamingExecutor::ComputeSensitivity(
    double original_verdict_score,
    double mutated_verdict_score,
    double original_metric,
    double mutated_metric) {
    
    double score_delta = std::abs(mutated_verdict_score - original_verdict_score);
    double metric_delta = std::abs(mutated_metric - original_metric);
    
    if (metric_delta > 0) {
        return score_delta / metric_delta;
    }
    return 0.0;
}

double AntiGamingExecutor::VerdictToScore(const std::string& verdict) const {
    if (verdict == "SUPPORTED") return 1.0;
    if (verdict == "FALSIFIED") return 0.0;
    if (verdict == "INCONCLUSIVE") return 0.5;
    return 0.5;  // Default for UNKNOWN/NOT_EVALUABLE
}

} // namespace msf