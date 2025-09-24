#include <benchmark/benchmark.h>
#include <atomic>
#include <thread>
#include <vector>
#include "padded.hpp"
#include "variance_tracker.hpp"

// =============================================================================
// Atomic Counter Benchmark
// =============================================================================
// This benchmark measures the performance of atomic operations under contention.
// Compares different memory orderings: relaxed, acquire_release, seq_cst.
//
// Key insights for interviews:
// - Atomic relaxed: Fastest but requires careful reasoning about ordering
// - Atomic seq_cst: Slowest but provides strongest guarantees
// - Cache line ping-ponging: Performance degrades with high contention
// - Scalability: Usually better than mutex for simple operations
// =============================================================================

// Sequential consistency (strongest ordering)
static void Counter_Atomic_SeqCst(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    padded<std::atomic<uint64_t>> counter{0};
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
                // Default memory ordering is seq_cst
                counter.fetch_add(1);
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
    
    state.counters["ops_total"] = counter.load();
    state.counters["fairness_cv"] = tracker.coefficient_of_variation();
    state.SetItemsProcessed(counter.load());
}

// Relaxed ordering (weakest, fastest)
static void Counter_Atomic_Relaxed(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    padded<std::atomic<uint64_t>> counter{0};
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
                // Relaxed ordering - no synchronization guarantees
                counter.fetch_add(1, std::memory_order_relaxed);
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
    
    state.counters["ops_total"] = counter.load();
    state.counters["fairness_cv"] = tracker.coefficient_of_variation();
    state.SetItemsProcessed(counter.load());
}

// Acquire-release ordering (middle ground)
static void Counter_Atomic_AcqRel(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    padded<std::atomic<uint64_t>> counter{0};
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
                // Acquire-release ordering
                counter.fetch_add(1, std::memory_order_acq_rel);
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
    
    state.counters["ops_total"] = counter.load();
    state.counters["fairness_cv"] = tracker.coefficient_of_variation();
    state.SetItemsProcessed(counter.load());
}

BENCHMARK(Counter_Atomic_SeqCst)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(Counter_Atomic_Relaxed)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(Counter_Atomic_AcqRel)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();