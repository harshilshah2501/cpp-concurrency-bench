#include "variance_tracker.hpp"

// =============================================================================
// Thread Fairness and Variance Tracking Implementation
// =============================================================================
// This module implements statistical analysis for measuring fairness in
// concurrent algorithms. Fairness is a crucial but often overlooked aspect
// of concurrency primitives that becomes critical in production systems.
//
// Key concepts:
// - Perfect fairness: All threads get equal access over time
// - Starvation: Some threads never get access
// - Variance: Statistical measure of how work is distributed
//
// Coefficient of Variation (CV) interpretation:
// - CV = 0.0: Perfect fairness (all threads did equal work)
// - CV < 0.1: Very good fairness
// - CV 0.1-0.3: Acceptable fairness for most applications
// - CV 0.3-0.5: Poor fairness, some threads may be struggling
// - CV > 0.5: Very poor fairness, likely starvation occurring
//
// Common causes of unfairness:
// - Spinlocks under preemption (some threads spin while preempted)
// - Reader-writer locks with writer starvation
// - Priority inversion in priority-based schedulers
// - NUMA effects with non-uniform memory access
// =============================================================================

variance_tracker::variance_tracker(size_t num_threads) 
    : thread_counts(num_threads) {
    // Initialize all atomic counters to zero
    // atomic<uint64_t> has no default constructor that zeros the value
    // on all platforms, so we explicitly initialize each counter
    for (auto& counter : thread_counts) {
        counter.store(0, std::memory_order_relaxed);
    }
}

void variance_tracker::record_operation(size_t thread_id) {
    // Record that thread_id completed one operation
    // Uses relaxed memory ordering since we only care about the count,
    // not synchronization between threads
    if (thread_id < thread_counts.size()) {
        thread_counts[thread_id].fetch_add(1, std::memory_order_relaxed);
    }
    // Note: We silently ignore invalid thread_ids to avoid performance
    // overhead of throwing exceptions in hot benchmark paths
}

double variance_tracker::coefficient_of_variation() const {
    // Collect current counts from all threads
    // We take a snapshot at a single point in time, which may have
    // slight inconsistencies if called while threads are still running,
    // but this is acceptable for fairness analysis
    std::vector<uint64_t> counts;
    counts.reserve(thread_counts.size());
    for (const auto& counter : thread_counts) {
        counts.push_back(counter.load(std::memory_order_relaxed));
    }
    
    if (counts.empty()) return 0.0;
    
    // Calculate mean (average operations per thread)
    double mean = 0.0;
    for (auto count : counts) {
        mean += static_cast<double>(count);
    }
    mean /= counts.size();
    
    // Handle edge case where no operations have been recorded
    if (mean == 0.0) return 0.0;
    
    // Calculate variance (average squared deviation from mean)
    double variance = 0.0;
    for (auto count : counts) {
        double deviation = static_cast<double>(count) - mean;
        variance += deviation * deviation;
    }
    variance /= counts.size();
    
    // Coefficient of variation = standard deviation / mean
    // This normalizes the variance to be independent of the absolute
    // number of operations, making it easy to compare across different
    // benchmark runs and thread counts
    return std::sqrt(variance) / mean;
}

void variance_tracker::reset() {
    // Reset all counters for the next benchmark iteration
    // This allows reusing the same tracker object across multiple
    // benchmark runs without reallocating memory
    for (auto& counter : thread_counts) {
        counter.store(0, std::memory_order_relaxed);
    }
}

std::vector<uint64_t> variance_tracker::get_counts() const {
    // Return raw per-thread counts for detailed analysis
    // Useful for generating histograms or identifying specific
    // threads that are being starved
    std::vector<uint64_t> counts;
    counts.reserve(thread_counts.size());
    for (const auto& counter : thread_counts) {
        counts.push_back(counter.load(std::memory_order_relaxed));
    }
    return counts;
}