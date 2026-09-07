#include <benchmark/benchmark.h>
#include <barrier>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <numeric>
#include "timing.hpp"
#include "variance_tracker.hpp"

// =============================================================================
// Barrier Synchronization Benchmark
// =============================================================================
// This benchmark measures the performance of std::barrier (C++20) for thread
// synchronization points vs manual condition variable implementations.
//
// Key insights for interviews:
// - Barrier: N threads wait until all reach synchronization point
// - Completion function execution on last arriving thread
// - Phase-based execution patterns (BSP - Bulk Synchronous Parallel)
// - Scalability of barrier implementations
// - Arrival/wait vs arrive_and_wait patterns
//
// Patterns tested:
// 1. std::barrier vs manual condition variable barrier
// 2. Various thread counts and synchronization phases
// 3. Barrier completion functions
// 4. Thread arrival time variance
// 5. Work simulation between barriers
// =============================================================================

// Manual barrier implementation using condition variable for comparison
class ManualBarrier {
private:
    mutable std::mutex mtx_;
    mutable std::condition_variable cv_;
    std::size_t count_;
    std::size_t waiting_;
    std::size_t generation_;

public:
    explicit ManualBarrier(std::size_t count) 
        : count_(count), waiting_(0), generation_(0) {}
    
    void arrive_and_wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        const auto current_gen = generation_;
        
        if (++waiting_ == count_) {
            // Last thread to arrive
            waiting_ = 0;
            ++generation_;
            cv_.notify_all();
        } else {
            // Wait for all threads to arrive
            cv_.wait(lock, [this, current_gen] { 
                return generation_ > current_gen; 
            });
        }
    }
};

// Simulated work function to create realistic load between barriers
void simulate_work(std::chrono::microseconds duration) {
    auto start = std::chrono::high_resolution_clock::now();
    volatile int dummy = 0;
    while (std::chrono::high_resolution_clock::now() - start < duration) {
        for (int i = 0; i < 100; ++i) {
            dummy += i;  // Prevent optimization
        }
    }
    benchmark::DoNotOptimize(dummy);
}

// std::barrier benchmark with work simulation
static void StdBarrier_WithWork(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int num_phases = 10;
    const auto work_duration = std::chrono::microseconds(100);

    for (auto _ : state) {
        std::barrier sync_point(num_threads);
        // Per-phase start times published by thread 0 after each barrier.
        std::vector<std::chrono::high_resolution_clock::time_point> phase_starts(
            static_cast<size_t>(num_phases));
        std::vector<std::chrono::nanoseconds> last_arrival(
            static_cast<size_t>(num_threads));
        phase_starts[0] = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i] {
                for (int phase = 0; phase < num_phases; ++phase) {
                    simulate_work(work_duration);

                    auto arrival_time =
                        std::chrono::high_resolution_clock::now();
                    // phase_starts[phase] is published before this phase begins
                    // (phase 0 before launch; later phases by thread 0 after
                    // the previous barrier, before the next arrive_and_wait).
                    last_arrival[static_cast<size_t>(thread_id)] =
                        arrival_time - phase_starts[static_cast<size_t>(phase)];

                    sync_point.arrive_and_wait();

                    if (thread_id == 0 && phase + 1 < num_phases) {
                        phase_starts[static_cast<size_t>(phase + 1)] =
                            std::chrono::high_resolution_clock::now();
                    }
                    // Ensure all threads observe the next phase start before
                    // continuing (thread 0 publishes after the barrier).
                    if (phase + 1 < num_phases) {
                        sync_point.arrive_and_wait();
                    }
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        std::vector<double> arrival_ns;
        arrival_ns.reserve(static_cast<size_t>(num_threads));
        for (const auto& time : last_arrival) {
            arrival_ns.push_back(static_cast<double>(time.count()));
        }

        double mean =
            std::accumulate(arrival_ns.begin(), arrival_ns.end(), 0.0) /
            arrival_ns.size();
        double variance = 0.0;
        for (double time : arrival_ns) {
            variance += (time - mean) * (time - mean);
        }
        variance /= arrival_ns.size();
        double cv = mean > 0.0 ? std::sqrt(variance) / mean : 0.0;

        state.counters["arrival_cv"] = cv;
    }

    state.counters["phases"] = num_phases;
    state.counters["work_us"] = work_duration.count();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            num_phases * num_threads);
}

// Manual barrier benchmark for comparison
static void ManualBarrier_WithWork(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int num_phases = 10;
    const auto work_duration = std::chrono::microseconds(100);
    
    for (auto _ : state) {
        ManualBarrier sync_point(num_threads);
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                for (int phase = 0; phase < num_phases; ++phase) {
                    // Simulate some work
                    simulate_work(work_duration);
                    
                    // Synchronize at barrier
                    sync_point.arrive_and_wait();
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.counters["phases"] = num_phases;
    state.counters["work_us"] = work_duration.count();
    state.SetItemsProcessed(num_phases * num_threads);
}

// Barrier with completion function (only available with std::barrier)
static void StdBarrier_WithCompletion(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int num_phases = 10;
    std::atomic<int> completion_count{0};
    
    for (auto _ : state) {
        completion_count = 0;
        
        // Completion function runs on the last arriving thread
        auto completion_fn = [&completion_count]() noexcept {
            completion_count.fetch_add(1, std::memory_order_relaxed);
        };
        
        std::barrier sync_point(num_threads, completion_fn);
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                for (int phase = 0; phase < num_phases; ++phase) {
                    // Simulate variable work to test barrier effectiveness
                    auto work_time = std::chrono::microseconds(50 + (phase * 10));
                    simulate_work(work_time);
                    
                    // Synchronize at barrier
                    sync_point.arrive_and_wait();
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        state.counters["completions"] = completion_count.load();
    }
    
    state.counters["phases"] = num_phases;
    state.SetItemsProcessed(num_phases * num_threads);
}

// High-frequency barrier test (minimal work between barriers)
static void StdBarrier_HighFrequency(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int num_phases = 1000;  // Many short phases
    
    for (auto _ : state) {
        std::barrier sync_point(num_threads);
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                for (int phase = 0; phase < num_phases; ++phase) {
                    // Minimal work - just increment a counter
                    volatile int dummy = phase;
                    benchmark::DoNotOptimize(dummy);
                    
                    // Synchronize at barrier
                    sync_point.arrive_and_wait();
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.counters["phases"] = num_phases;
    state.SetItemsProcessed(num_phases * num_threads);
}

// Arrival/wait pattern test (some threads arrive early, others wait)
static void StdBarrier_ArrivalPattern(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int num_phases = 50;
    
    for (auto _ : state) {
        std::barrier sync_point(num_threads);
        variance_tracker sync_variance(num_threads);
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i] {
                for (int phase = 0; phase < num_phases; ++phase) {
                    // Variable work based on thread ID to create arrival patterns
                    auto work_multiplier = (thread_id % 3) + 1;  // 1x, 2x, or 3x work
                    auto work_time = std::chrono::microseconds(50 * work_multiplier);
                    simulate_work(work_time);
                    
                    auto before_barrier = std::chrono::high_resolution_clock::now();
                    sync_point.arrive_and_wait();
                    auto after_barrier = std::chrono::high_resolution_clock::now();
                    
                    // Record operation for fairness tracking
                    sync_variance.record_operation(thread_id);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        state.counters["sync_fairness"] = sync_variance.coefficient_of_variation();
    }
    
    state.counters["phases"] = num_phases;
    state.SetItemsProcessed(num_phases * num_threads);
}

// Benchmark configurations
BENCHMARK(StdBarrier_WithWork)
    ->RangeMultiplier(2)
    ->Range(2, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ManualBarrier_WithWork)
    ->RangeMultiplier(2)
    ->Range(2, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdBarrier_WithCompletion)
    ->RangeMultiplier(2)
    ->Range(2, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdBarrier_HighFrequency)
    ->RangeMultiplier(2)
    ->Range(2, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdBarrier_ArrivalPattern)
    ->RangeMultiplier(2)
    ->Range(2, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
