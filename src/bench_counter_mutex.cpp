#include <benchmark/benchmark.h>
#include <mutex>
#include <thread>
#include <vector>
#include "padded.hpp"
#include "variance_tracker.hpp"

// =============================================================================
// Mutex-Protected Counter Benchmark
// =============================================================================
// This benchmark measures the performance of std::mutex under high contention
// by having multiple threads increment a shared counter. This represents the
// "worst case" for mutex performance - maximum contention on a single resource.
//
// Key measurements:
// 1. Throughput: Total operations per second across all threads
// 2. Scalability: How performance changes with thread count
// 3. Fairness: Whether all threads get equal access (coefficient of variation)
// 4. Latency distribution: Individual operation timing
//
// Expected behavior:
// - Single thread: Baseline performance (no contention)
// - Multiple threads: Performance degrades due to serialization
// - Beyond core count: Further degradation due to context switching
//
// This provides the baseline against which to compare:
// - Atomic operations (bench_counter_atomic)
// - Spinlocks (bench_counter_spin)
// - Lock-free algorithms
// =============================================================================

static void Counter_Mutex_Hot(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    // Shared resources protected by mutex
    std::mutex mtx;
    padded<uint64_t> counter{0};  // Prevent false sharing with padding
    
    // Track per-thread fairness
    variance_tracker tracker(num_threads);
    
    // Synchronization for coordinated start/stop
    std::vector<std::thread> threads;
    std::atomic<bool> start{false};
    std::atomic<bool> stop{false};
    
    // Launch worker threads that will contend for the mutex
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            // Wait for benchmark to signal start
            // Using memory_order_acquire to ensure proper synchronization
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();  // Be nice to other threads
            }
            
            // Main work loop: increment counter under mutex protection
            while (!stop.load(std::memory_order_acquire)) {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    counter.value++;  // Critical section
                }
                // Record operation for fairness tracking
                tracker.record_operation(thread_id);
            }
        });
    }
    
    // Benchmark measurement loop
    // Each iteration measures a fixed time window
    for (auto _ : state) {
        tracker.reset();
        
        // Start all threads simultaneously
        start.store(true, std::memory_order_release);
        
        // Let threads run for measurement period
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Stop all threads
        stop.store(true, std::memory_order_release);
        start.store(false, std::memory_order_release);
        
        // Brief pause between iterations to let threads settle
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        stop.store(false, std::memory_order_release);
    }
    
    // Clean shutdown
    stop.store(true, std::memory_order_release);
    for (auto& t : threads) {
        t.join();
    }
    
    // Report key metrics for analysis
    state.counters["ops_total"] = counter.value;
    state.counters["fairness_cv"] = tracker.coefficient_of_variation();
    state.counters["ops_per_thread"] = static_cast<double>(counter.value) / num_threads;
    
    // This tells Google Benchmark how many "items" were processed
    // Used to calculate throughput (items/second)
    state.SetItemsProcessed(counter.value);
}

// Benchmark configuration
// Tests thread counts from 1 to hardware_concurrency (typically 2x core count)
// RangeMultiplier(2) tests powers of 2: 1, 2, 4, 8, etc.
BENCHMARK(Counter_Mutex_Hot)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()  // Use wall clock time, not CPU time
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();