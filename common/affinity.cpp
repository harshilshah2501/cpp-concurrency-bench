#include "affinity.hpp"

// =============================================================================
// Thread Affinity Implementation
// =============================================================================
// This file implements cross-platform CPU affinity control for consistent
// benchmarking. Thread migration between cores can introduce significant
// measurement noise due to:
// - Cold cache effects when threads move to new cores
// - NUMA topology effects on multi-socket systems
// - Variable memory access latencies
// - OS scheduling interference
//
// Platform-specific implementations:
// - macOS: Limited affinity support via thread policies
// - Linux: Full control via pthread_setaffinity_np
// - Windows: SetThreadAffinityMask (not implemented)
//
// For benchmarking best practices:
// 1. Pin benchmark threads to specific cores
// 2. Leave at least one core free for OS and other processes
// 3. Consider NUMA topology for multi-socket systems
// 4. Use numactl on Linux for memory binding
// =============================================================================

namespace affinity {

bool pin_thread_to_core(int core_id) {
#ifdef __APPLE__
    // macOS has limited thread affinity support compared to Linux
    // The thread affinity policy provides hints to the scheduler but
    // doesn't guarantee exclusive core assignment
    thread_affinity_policy_data_t policy = { core_id };
    thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
    return thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                           (thread_policy_t)&policy, 1) == KERN_SUCCESS;
#elif __linux__
    // Linux provides excellent thread affinity control
    // cpu_set_t allows fine-grained control over which cores a thread can run on
    // This is essential for NUMA-aware benchmarking and eliminating scheduler noise
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);           // Clear the CPU set
    CPU_SET(core_id, &cpuset);   // Add the target core to the set
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#else
    // Unsupported platform - return false to indicate failure
    // Benchmarks should handle this gracefully and warn about potential noise
    (void)core_id;  // Suppress unused parameter warning
    return false;
#endif
}

std::vector<int> get_available_cores() {
    // Return a simple range of core IDs from 0 to num_cores-1
    // In production, this could be enhanced to:
    // - Query actual CPU topology
    // - Exclude isolated or offline cores
    // - Respect NUMA node boundaries
    // - Filter out cores reserved for system processes
    int num_cores = get_num_logical_cores();
    std::vector<int> cores;
    cores.reserve(num_cores);
    for (int i = 0; i < num_cores; ++i) {
        cores.push_back(i);
    }
    return cores;
}

int get_num_logical_cores() {
    // Returns the number of logical cores (includes hyperthreading)
    // This is typically 2x the number of physical cores on modern CPUs
    // For benchmarking:
    // - Use physical core count for CPU-bound workloads
    // - Use logical core count for I/O or mixed workloads
    unsigned int cores = std::thread::hardware_concurrency();
    return cores == 0 ? 1 : static_cast<int>(cores);
}

bool is_affinity_supported() {
#if defined(__linux__) || defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

} // namespace affinity