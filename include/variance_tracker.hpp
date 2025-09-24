#pragma once
#include <vector>
#include <atomic>
#include <cmath>

// =============================================================================
// Thread Fairness and Variance Measurement
// =============================================================================
// This class tracks per-thread operation counts to measure fairness in
// concurrent algorithms. High variance indicates some threads are being
// starved or getting disproportionate access to shared resources.
//
// Key metrics:
// - Coefficient of Variation (CV): Standard deviation / mean
//   - CV < 0.1: Very fair distribution
//   - CV 0.1-0.5: Moderate fairness
//   - CV > 0.5: Poor fairness, potential starvation
//
// This is crucial for interview discussions about spinlock vs mutex
// fairness, reader-writer lock starvation, and lock-free algorithm
// progress guarantees.
// =============================================================================

class variance_tracker {
public:
    // Initialize tracking for the specified number of threads
    explicit variance_tracker(size_t num_threads);
    
    // Record that thread_id completed one operation
    // Must be called from the correct thread for accurate tracking
    void record_operation(size_t thread_id);
    
    // Calculate coefficient of variation across all threads
    // Returns 0.0 if no operations recorded or division by zero
    double coefficient_of_variation() const;
    
    // Reset all counters for next benchmark iteration
    void reset();
    
    // Get raw counts for detailed analysis
    std::vector<uint64_t> get_counts() const;

private:
    // Per-thread operation counters
    // Using atomic for thread-safe updates without additional synchronization
    std::vector<std::atomic<uint64_t>> thread_counts;
};