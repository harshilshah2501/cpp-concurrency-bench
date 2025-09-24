#include "timing.hpp"
#include <chrono>

// =============================================================================
// High-Resolution Timing Implementation
// =============================================================================
// This module provides precise timing capabilities for performance measurement.
// Uses std::chrono::high_resolution_clock which typically maps to:
// - Linux: CLOCK_MONOTONIC (not affected by system time changes)
// - macOS: mach_absolute_time() (high-precision monotonic clock)
// - Windows: QueryPerformanceCounter (high-frequency timer)
//
// Key characteristics:
// - Monotonic: Never goes backwards, unaffected by system time adjustments
// - High precision: Nanosecond resolution on most platforms
// - Low overhead: Suitable for micro-benchmarking
//
// Best practices for timing:
// 1. Warm up before measurement to stabilize CPU frequency
// 2. Run multiple iterations and take statistics (median, percentiles)
// 3. Account for timer overhead in very short measurements
// 4. Use consistent CPU frequency (disable turbo boost for repeatability)
// =============================================================================

namespace timing {

high_resolution_timer::high_resolution_timer() {
    // Initialize timer by capturing the current time point
    // This establishes the baseline for all subsequent elapsed time calculations
    reset();
}

void high_resolution_timer::reset() {
    // Capture a new baseline time point
    // This is typically called at the start of each benchmark iteration
    // to measure the duration of a specific operation or code section
    start_time = std::chrono::high_resolution_clock::now();
}

double high_resolution_timer::elapsed_seconds() const {
    // Calculate elapsed time in seconds with nanosecond precision
    // Returns floating-point seconds for easy arithmetic and comparison
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    return duration.count() / 1e9;  // Convert nanoseconds to seconds
}

double high_resolution_timer::elapsed_milliseconds() const {
    // Convenience method for millisecond timing
    // Useful for medium-duration operations (1ms - 1s range)
    return elapsed_seconds() * 1000.0;
}

double high_resolution_timer::elapsed_microseconds() const {
    // Convenience method for microsecond timing
    // Useful for short operations and latency measurements
    // Most system calls and lock operations fall in this range
    return elapsed_seconds() * 1e6;
}

} // namespace timing