#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include "padded.hpp"
#include "spinlock.hpp"
#include "variance_tracker.hpp"

// =============================================================================
// Spinlock-Protected Counter Benchmark
// =============================================================================
// This benchmark measures the performance of custom spinlock under contention.
// Spinlocks are useful for very short critical sections but can waste CPU cycles.
//
// Key insights for interviews:
// - Spinlocks excel when critical sections are extremely short
// - CPU usage is 100% even when waiting (busy-waiting)
// - Performance degrades severely under high contention
// - Can cause priority inversion if lower-priority thread holds lock
// - Memory ordering is crucial for correctness
//
// Expected behavior:
// - Single thread: Should be fastest (minimal lock overhead)
// - Low contention: May outperform mutex for short critical sections
// - High contention: Wastes CPU cycles, may perform worse than mutex
// - Thread count > CPU cores: Severe degradation due to context switching
// =============================================================================

static void Counter_Spin_Hot(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    // Shared resources protected by spinlock
    spinlock lock;
    padded<uint64_t> counter{0};  // Prevent false sharing with padding
    
    // Track per-thread fairness
    variance_tracker tracker(num_threads);
    
    // Synchronization for coordinated start/stop
    std::vector<std::thread> threads;
    std::atomic<bool> start{false};
    std::atomic<bool> stop{false};
    
    // Launch worker threads that will contend for the spinlock
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            // Wait for benchmark to signal start
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            // Main work loop: increment counter under spinlock protection
            while (!stop.load(std::memory_order_acquire)) {
                // Spinlock critical section
                lock.lock();
                counter.value++;  // Critical section (very short)
                lock.unlock();
                
                // Record operation for fairness tracking
                tracker.record_operation(thread_id);
            }
        });
    }
    
    // Benchmark measurement loop
    for (auto _ : state) {
        tracker.reset();
        
        // Start all threads simultaneously
        start.store(true, std::memory_order_release);
        
        // Let threads run for measurement period
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Stop all threads
        stop.store(true, std::memory_order_release);
        start.store(false, std::memory_order_release);
        
        // Brief pause between iterations
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
    
    state.SetItemsProcessed(counter.value);
}

// Spinlock with simulated work in critical section
// This shows how spinlock performance changes with critical section length
static void Counter_Spin_WithWork(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    spinlock lock;
    padded<uint64_t> counter{0};
    variance_tracker tracker(num_threads);
    
    std::vector<std::thread> threads;
    std::atomic<bool> start{false};
    std::atomic<bool> stop{false};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            while (!stop.load(std::memory_order_acquire)) {
                lock.lock();
                
                // Simulate slightly longer critical section
                counter.value++;
                // Small amount of work to simulate real critical section
                volatile int dummy = 0;
                for (int j = 0; j < 10; ++j) {
                    dummy += j;
                }
                
                lock.unlock();
                tracker.record_operation(thread_id);
            }
        });
    }
    
    for (auto _ : state) {
        tracker.reset();
        start.store(true, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stop.store(true, std::memory_order_release);
        start.store(false, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        stop.store(false, std::memory_order_release);
    }
    
    stop.store(true, std::memory_order_release);
    for (auto& t : threads) {
        t.join();
    }
    
    state.counters["ops_total"] = counter.value;
    state.counters["fairness_cv"] = tracker.coefficient_of_variation();
    state.counters["ops_per_thread"] = static_cast<double>(counter.value) / num_threads;
    state.SetItemsProcessed(counter.value);
}

// Spinlock vs mutex fairness comparison
// This benchmark specifically tests fairness under high contention
static void Counter_Spin_Fairness(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    spinlock lock;
    padded<uint64_t> counter{0};
    variance_tracker tracker(num_threads);
    
    std::vector<std::thread> threads;
    std::atomic<bool> start{false};
    std::atomic<bool> stop{false};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            // Run for longer to see fairness effects
            while (!stop.load(std::memory_order_acquire)) {
                if (lock.try_lock()) {
                    counter.value++;
                    lock.unlock();
                    tracker.record_operation(thread_id);
                } else {
                    // Brief yield when lock is busy
                    std::this_thread::yield();
                }
            }
        });
    }
    
    for (auto _ : state) {
        tracker.reset();
        start.store(true, std::memory_order_release);
        
        // Longer measurement period for fairness analysis
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        stop.store(true, std::memory_order_release);
        start.store(false, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        stop.store(false, std::memory_order_release);
    }
    
    stop.store(true, std::memory_order_release);
    for (auto& t : threads) {
        t.join();
    }
    
    state.counters["ops_total"] = counter.value;
    state.counters["fairness_cv"] = tracker.coefficient_of_variation();
    state.counters["ops_per_thread"] = static_cast<double>(counter.value) / num_threads;
    state.SetItemsProcessed(counter.value);
}

// Benchmark configurations
BENCHMARK(Counter_Spin_Hot)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(Counter_Spin_WithWork)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(Counter_Spin_Fairness)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();