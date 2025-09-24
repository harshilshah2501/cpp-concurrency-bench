#pragma once
#include <thread>
#include <vector>

// =============================================================================
// CPU Affinity and Thread Pinning Utilities
// =============================================================================
// Thread affinity control is crucial for consistent benchmarking results.
// Without pinning threads to specific cores:
// - Threads may migrate between cores, causing cache misses
// - OS scheduling can introduce timing variability
// - NUMA effects can skew results on multi-socket systems
//
// These utilities provide cross-platform thread pinning for:
// - Reproducible benchmark results
// - Measuring NUMA effects by controlling placement
// - Isolating performance effects from scheduling noise
//
// Platform support:
// - Linux: Uses pthread_setaffinity_np and cpu_set_t
// - macOS: Uses thread_policy_set (limited compared to Linux)
// - Windows: Could use SetThreadAffinityMask (not implemented)
// =============================================================================

#ifdef __APPLE__
#include <mach/thread_policy.h>
#include <mach/mach.h>
#elif __linux__
#include <sched.h>
#include <pthread.h>
#endif

namespace affinity {
    // Pin the current thread to a specific CPU core
    // Returns true on success, false if unsupported or failed
    // Note: macOS has limited thread affinity support compared to Linux
    bool pin_thread_to_core(int core_id);
    
    // Get list of available CPU cores for thread placement
    // Useful for distributing threads across available hardware
    std::vector<int> get_available_cores();
    
    // Get the number of logical CPU cores (includes hyperthreading)
    // Used to determine maximum useful thread count for benchmarks
    int get_num_logical_cores();
    
    // Check if the current system supports thread affinity
    bool is_affinity_supported();
}