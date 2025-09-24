#include <benchmark/benchmark.h>
#include <future>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include <numeric>

// =============================================================================
// Async Future vs Promise-Future Benchmark
// =============================================================================
// This benchmark compares std::async with different launch policies against
// manual promise-future patterns for asynchronous task execution.
//
// Key insights for interviews:
// - std::async: High-level, automatic thread management
// - Promise-Future: Manual control, explicit thread management
// - Launch policies: deferred vs async vs auto
// - Exception handling across async boundaries
// - Future chaining and composition patterns
//
// Patterns tested:
// 1. std::async with different launch policies
// 2. Manual promise-future with custom threading
// 3. Exception propagation patterns
// 4. Future chaining and continuation
// 5. Task cancellation simulation
// =============================================================================

// CPU-intensive work function
int cpu_work(int iterations) {
    int result = 0;
    for (int i = 0; i < iterations; ++i) {
        result += i * i;
        if (i % 1000 == 0) {
            result ^= (i >> 4);  // Prevent optimization
        }
    }
    return result;
}

// I/O simulation
void io_work(std::chrono::microseconds duration) {
    std::this_thread::sleep_for(duration);
}

// Exception-throwing work function
int risky_work(int value, bool should_throw = false) {
    if (should_throw && value % 10 == 0) {
        throw std::runtime_error("Simulated error");
    }
    return cpu_work(value * 100);
}

// std::async with launch::async policy
static void StdAsync_LaunchAsync(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int work_size = 10000;
    
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Launch all tasks with async policy
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                std::async(std::launch::async, [i, work_size] {
                    return cpu_work(work_size + i);
                })
            );
        }
        
        // Collect all results
        int total_result = 0;
        for (auto& future : futures) {
            total_result += future.get();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        benchmark::DoNotOptimize(total_result);
        state.counters["completion_time_us"] = duration.count();
        state.counters["throughput"] = (1000000.0 * num_tasks) / duration.count();
    }
    
    state.SetItemsProcessed(num_tasks);
}

// std::async with launch::deferred policy
static void StdAsync_LaunchDeferred(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int work_size = 10000;
    
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Launch all tasks with deferred policy (lazy evaluation)
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                std::async(std::launch::deferred, [i, work_size] {
                    return cpu_work(work_size + i);
                })
            );
        }
        
        // Collect all results (this is when work actually happens)
        int total_result = 0;
        for (auto& future : futures) {
            total_result += future.get();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        benchmark::DoNotOptimize(total_result);
        state.counters["completion_time_us"] = duration.count();
        state.counters["throughput"] = (1000000.0 * num_tasks) / duration.count();
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Manual promise-future with thread pool simulation
static void ManualPromiseFuture(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int work_size = 10000;
    const int num_threads = std::thread::hardware_concurrency();
    
    for (auto _ : state) {
        std::vector<std::promise<int>> promises(num_tasks);
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);
        
        // Create futures from promises
        for (auto& promise : promises) {
            futures.emplace_back(promise.get_future());
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Manually manage worker threads
        std::vector<std::thread> workers;
        std::atomic<int> task_index{0};
        
        for (int i = 0; i < num_threads; ++i) {
            workers.emplace_back([&, work_size] {
                while (true) {
                    int idx = task_index.fetch_add(1);
                    if (idx >= num_tasks) break;
                    
                    try {
                        int result = cpu_work(work_size + idx);
                        promises[idx].set_value(result);
                    } catch (...) {
                        promises[idx].set_exception(std::current_exception());
                    }
                }
            });
        }
        
        // Wait for all workers
        for (auto& worker : workers) {
            worker.join();
        }
        
        // Collect all results
        int total_result = 0;
        for (auto& future : futures) {
            total_result += future.get();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        benchmark::DoNotOptimize(total_result);
        state.counters["completion_time_us"] = duration.count();
        state.counters["throughput"] = (1000000.0 * num_tasks) / duration.count();
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Exception handling with async
static void StdAsync_ExceptionHandling(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const double error_rate = 0.1;  // 10% tasks throw exceptions
    
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Launch tasks with potential exceptions
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                std::async(std::launch::async, [i, error_rate] {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_real_distribution<> dis(0.0, 1.0);
                    bool should_throw = dis(gen) < error_rate;
                    return risky_work(i + 1000, should_throw);
                })
            );
        }
        
        // Collect results and handle exceptions
        int total_result = 0;
        int exception_count = 0;
        
        for (auto& future : futures) {
            try {
                total_result += future.get();
            } catch (const std::exception&) {
                ++exception_count;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        benchmark::DoNotOptimize(total_result);
        state.counters["completion_time_us"] = duration.count();
        state.counters["exception_count"] = exception_count;
        state.counters["success_rate"] = (1.0 - static_cast<double>(exception_count) / num_tasks) * 100;
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Future timeout simulation
static void StdAsync_Timeout(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const auto task_duration = std::chrono::milliseconds(100);
    const auto timeout_duration = std::chrono::milliseconds(150);
    
    for (auto _ : state) {
        std::vector<std::future<void>> futures;
        futures.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Launch I/O-bound tasks
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                std::async(std::launch::async, [task_duration] {
                    io_work(std::chrono::duration_cast<std::chrono::microseconds>(task_duration));
                })
            );
        }
        
        // Wait with timeout
        int completed_count = 0;
        int timeout_count = 0;
        
        for (auto& future : futures) {
            auto status = future.wait_for(timeout_duration);
            if (status == std::future_status::ready) {
                future.get();  // Complete the operation
                ++completed_count;
            } else {
                ++timeout_count;
                // In real code, you might cancel the task here
                future.get();  // Still need to wait for cleanup
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        state.counters["completion_time_us"] = duration.count();
        state.counters["completed"] = completed_count;
        state.counters["timeouts"] = timeout_count;
        state.counters["completion_rate"] = (static_cast<double>(completed_count) / num_tasks) * 100;
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Future chaining simulation (continuation-style)
static void StdAsync_Chaining(benchmark::State& state) {
    const int num_chains = state.range(0);
    const int chain_length = 5;  // Each chain has 5 steps
    const int work_per_step = 2000;
    
    for (auto _ : state) {
        std::vector<std::future<int>> final_futures;
        final_futures.reserve(num_chains);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create chained computations
        for (int chain = 0; chain < num_chains; ++chain) {
            auto future = std::async(std::launch::async, [work_per_step] {
                return cpu_work(work_per_step);
            });
            
            // Chain multiple async operations
            for (int step = 1; step < chain_length; ++step) {
                future = std::async(std::launch::async, [future = std::move(future), work_per_step]() mutable {
                    int prev_result = future.get();
                    return prev_result + cpu_work(work_per_step);
                });
            }
            
            final_futures.emplace_back(std::move(future));
        }
        
        // Wait for all chains to complete
        int total_result = 0;
        for (auto& future : final_futures) {
            total_result += future.get();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        benchmark::DoNotOptimize(total_result);
        state.counters["completion_time_us"] = duration.count();
        state.counters["chain_length"] = chain_length;
        state.counters["total_tasks"] = num_chains * chain_length;
    }
    
    state.SetItemsProcessed(num_chains * chain_length);
}

// Benchmark configurations
BENCHMARK(StdAsync_LaunchAsync)
    ->RangeMultiplier(2)
    ->Range(8, 256)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdAsync_LaunchDeferred)
    ->RangeMultiplier(2)
    ->Range(8, 256)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ManualPromiseFuture)
    ->RangeMultiplier(2)
    ->Range(8, 256)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdAsync_ExceptionHandling)
    ->RangeMultiplier(2)
    ->Range(16, 128)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdAsync_Timeout)
    ->RangeMultiplier(2)
    ->Range(8, 64)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdAsync_Chaining)
    ->RangeMultiplier(2)
    ->Range(4, 32)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
