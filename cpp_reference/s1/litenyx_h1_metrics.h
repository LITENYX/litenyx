// Litenyx H1-SHADOW-ENG — Deterministic Metric Collectors (H1-M1..M4).
//
// H1 ONLY. All collectors are pure accumulators over integer/fixed-point inputs.
// No consensus state, no topology handle. Each metric version is explicit so
// results are comparable across runs.

#ifndef LITENYX_H1_METRICS_H
#define LITENYX_H1_METRICS_H

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>

// Fixed-point scale for metrics that need fractional parts (e.g. HHI in permille).
static const int64_t H1_METRIC_Q = 1000000;

// Explicit deterministic metric version string (change when definitions change).
static const char* H1_METRIC_VERSION = "H1-METRICS-v1";

// ---------------------------------------------------------------------------
// H1-M1 — Temporal Stability
// Inputs: observed block intervals Δt (integer ticks). Cadence: per-block.
// Aggregation: per-window mean/var/std, percentiles, stall, recovery.
// ---------------------------------------------------------------------------
struct H1MetricM1 {
    std::vector<int64_t> intervals; // collected Δt per block

    void Add(int64_t dt) { if (dt > 0) intervals.push_back(dt); }

    void Reset() { intervals.clear(); }

    int64_t Count() const { return (int64_t)intervals.size(); }

    int64_t Mean() const {
        if (intervals.empty()) return 0;
        int64_t s = 0; for (int64_t v : intervals) s += v;
        return s / (int64_t)intervals.size();
    }

    // Population variance (fixed-point denominator = count).
    int64_t Variance() const {
        if (intervals.size() < 2) return 0;
        int64_t m = Mean();
        int64_t s = 0;
        for (int64_t v : intervals) { int64_t d = v - m; s += d * d; }
        return s / (int64_t)intervals.size();
    }

    int64_t StdDev() const {
        int64_t v = Variance();
        // integer sqrt (Newton) for fixed-point std dev.
        if (v <= 0) return 0;
        int64_t x = v;
        for (int i = 0; i < 24; ++i) {
            int64_t y = (x + v / x) / 2;
            if (y >= x) break;
            x = y;
        }
        return x;
    }

    int64_t Percentile(double p) const {
        if (intervals.empty()) return 0;
        std::vector<int64_t> s = intervals;
        std::sort(s.begin(), s.end());
        double idx = p * (s.size() - 1);
        size_t lo = (size_t)idx;
        size_t hi = (lo + 1 < s.size()) ? lo + 1 : lo;
        double frac = idx - lo;
        return (int64_t)(s[lo] + frac * (s[hi] - s[lo]));
    }

    int64_t P5()  const { return Percentile(0.05); }
    int64_t P50() const { return Percentile(0.50); }
    int64_t P95() const { return Percentile(0.95); }
    int64_t P99() const { return Percentile(0.99); }

    // Maximum stall = largest observed interval.
    int64_t MaxStall() const {
        if (intervals.empty()) return 0;
        return *std::max_element(intervals.begin(), intervals.end());
    }

    // Deviation from target = |Mean - target|.
    int64_t DevFromTarget(int64_t target) const {
        int64_t m = Mean();
        return (m > target) ? (m - target) : (target - m);
    }

    // Recovery time = blocks after a shock index until |interval-mean|<tol sustained.
    int64_t Recovery(int64_t shockIdx, int64_t tol) const {
        if (shockIdx >= (int64_t)intervals.size()) return 0;
        int64_t m = Mean();
        for (int64_t i = shockIdx; i < (int64_t)intervals.size(); ++i) {
            int64_t d = (intervals[i] > m) ? (intervals[i] - m) : (m - intervals[i]);
            if (d <= tol) return i - shockIdx;
        }
        return (int64_t)intervals.size() - shockIdx; // never recovered within window
    }
};

// ---------------------------------------------------------------------------
// H1-M2 — Controller Hunting Ω
// Inputs: per-block intervals; normalized error e_T = (Δt - target)/target.
// ---------------------------------------------------------------------------
struct H1MetricM2 {
    std::vector<int64_t> errors; // normalized error * H1_METRIC_Q (fixed-point)

    void AddNormalized(int64_t dt, int64_t target) {
        if (target <= 0) return;
        int64_t e = ((dt - target) * H1_METRIC_Q) / target; // fixed-point
        errors.push_back(e);
    }

    void Reset() { errors.clear(); }

    int64_t Count() const { return (int64_t)errors.size(); }

    // Direction reversals of the normalized error sign (excluding zeros).
    int64_t Reversals() const {
        int64_t rev = 0;
        int64_t prev = 0;
        for (int64_t e : errors) {
            int64_t sgn = (e > 0) ? 1 : (e < 0 ? -1 : 0);
            if (sgn != 0) {
                if (prev != 0 && sgn != prev) ++rev;
                prev = sgn;
            }
        }
        return rev;
    }

    // Zero crossings of normalized error (sign change through zero).
    int64_t ZeroCrossings() const {
        int64_t zc = 0;
        int64_t prev = 0;
        for (int64_t e : errors) {
            int64_t sgn = (e > 0) ? 1 : (e < 0 ? -1 : 0);
            if (sgn == 0) continue;
            if (prev != 0 && sgn != prev) ++zc;
            prev = sgn;
        }
        return zc;
    }

    // Sustained alternating corrections: max run of strictly alternating signs.
    int64_t AlternatingRun() const {
        int64_t best = 0, cur = 0, prev = 0;
        for (int64_t e : errors) {
            int64_t sgn = (e > 0) ? 1 : (e < 0 ? -1 : 0);
            if (sgn == 0) { cur = 0; prev = 0; continue; }
            if (prev != 0 && sgn != prev) { ++cur; best = std::max(best, cur); }
            else cur = 1;
            prev = sgn;
        }
        return best;
    }

    int64_t OscillationAmplitude() const {
        if (errors.empty()) return 0;
        int64_t lo = *std::min_element(errors.begin(), errors.end());
        int64_t hi = *std::max_element(errors.begin(), errors.end());
        return hi - lo;
    }

    // Settling time = first index after which |e| < tol sustained for `window` blocks.
    int64_t Settling(int64_t tol, int64_t window) const {
        for (int64_t i = 0; i + window <= (int64_t)errors.size(); ++i) {
            bool ok = true;
            for (int64_t j = i; j < i + window; ++j) {
                int64_t a = (errors[j] > 0) ? errors[j] : -errors[j];
                if (a > tol) { ok = false; break; }
            }
            if (ok) return i;
        }
        return (int64_t)errors.size(); // not converged within window
    }

    bool NonConverged(int64_t tol, int64_t window) const {
        return Settling(tol, window) >= (int64_t)errors.size();
    }

    // Ω = reversals (the engine compares CF - BASE at the experiment level).
    int64_t Omega() const { return Reversals(); }
};

// ---------------------------------------------------------------------------
// H1-M3 — Hash-Power Concentration H_c (HHI)
// Inputs: per-window cohort solved-block shares s_i (fixed-point fractions).
// ---------------------------------------------------------------------------
struct H1MetricM3 {
    // Accumulate a window's realized solved-block shares (each s_i in fixed-point
    // fraction of H1_METRIC_Q, summing to ~H1_METRIC_Q).
    std::vector<int64_t> shares; // last window's shares

    void SetWindowShares(const std::vector<int64_t>& s) { shares = s; }

    void Reset() { shares.clear(); }

    // HHI = sum_i s_i^2, with s_i normalized so that sum s_i = H1_METRIC_Q.
    // Returns HHI * H1_METRIC_Q (fixed-point) to keep integer math.
    int64_t HHI() const {
        int64_t sum = 0;
        for (int64_t s : shares) sum += s * s; // s in [0, H1_METRIC_Q]
        return sum; // already scaled by H1_METRIC_Q^2 / H1_METRIC_Q effectively
    }

    // Expected vs realized delta for a cohort (normalized sim units).
    int64_t ShareDelta(int64_t expected, int64_t realized) const {
        return realized - expected;
    }
};

// ---------------------------------------------------------------------------
// H1-M4 — Economic Manipulation Cross-Over V_e (highest-priority security metric)
// Inputs: wallet manipulation magnitude, induced target change, work-factor change,
// block-time impact, lane-stall probability, recovery, normalized attacker cost.
// All normalized simulation units; NO real-world cost invented.
// ---------------------------------------------------------------------------
struct H1MetricM4 {
    int64_t W_adv_magnitude = 0;     // |ΔW| under adversarial manipulation (fixed-point)
    int64_t T_econ_change = 0;       // |T_econ_after - T_econ_before| (fixed-point)
    int64_t T_econ_velocity = 0;     // |ΔT_econ| per block (fixed-point)
    int64_t work_factor_change = 0;  // T_time / T_PoW^CF (>=1 means tighter) fixed-point
    int64_t block_time_impact = 0;   // mean Δt change vs baseline (ticks)
    int64_t lane_stall_prob = 0;     // 0..H1_METRIC_Q (fixed-point probability)
    int64_t recovery_blocks = 0;     // blocks to recover
    int64_t attacker_cost_norm = 0;  // normalized sim cost units ONLY

    // Composite V_e (normalized sim units): weighted sum of normalized sub-metrics.
    // Weights are EXPERIMENTAL; documented, not frozen.
    int64_t Composite(int64_t w_target=1, int64_t w_stall=1, int64_t w_time=1) const {
        int64_t v = w_target * T_econ_change
                  + w_stall * lane_stall_prob
                  + w_time  * block_time_impact;
        return v;
    }
};

#endif // LITENYX_H1_METRICS_H
