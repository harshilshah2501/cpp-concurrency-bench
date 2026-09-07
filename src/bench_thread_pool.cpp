#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <queue>
#include <future>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <random>
#include "thread_pool.hpp"

// =============================================================================
// Thread Pool vs std::async Benchmark
// =============================================================================
// This benchmark compares thread pool implementations against std::async
// for task-based parallelism patterns.
//
// Key insights for interviews:
// - Thread pool: Reuse threads, avoid creation/destruction overhead
// - std::async: Convenient but potential thread creation overhead
// - Work stealing vs work sharing approaches
// - Task queue contention and scalability
// - CPU vs I/O bound task characteristics
//
// Patterns tested:
// 1. Thread pool vs std::async for CPU-bound tasks
// 2. Thread pool vs std::async for I/O simulation
// 3. Task queue scalability
// 4. Mixed workload patterns
// 5. Thread utilization efficiency
// =============================================================================

// CPU-intensive task simulation
int cpu_intensive_task(int n) {
    int result = 0;
    for (int i = 0; i < n; ++i) {
        result += i * i;
        // Add some computation to prevent optimization
        if (i % 1000 == 0) {
            result = result ^ (i >> 4);
        }
    }
    return result;
}

// I/O simulation task
void io_simulation_task(std::chrono::microseconds duration) {
    std::this_thread::sleep_for(duration);
}

// Mixed task that does both CPU work and I/O simulation
int mixed_task(int cpu_work, std::chrono::microseconds io_wait) {
    int cpu_result = cpu_intensive_task(cpu_work);
    io_simulation_task(io_wait);
    return cpu_result;
}

// Thread Pool: CPU-bound tasks
// Pool is reused across iterations (realistic usage). Creating/joining a fresh
// pool every iteration races the OS thread lifecycle and can hang under load.
static void ThreadPool_CPUBound(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int cpu_work_size = 10000;
    thread_pool pool;  // Uses hardware_concurrency threads by default

    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);

        auto start = std::chrono::high_resolution_clock::now();

        // Submit all tasks
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                pool.submit([i, cpu_work_size] {
                    return cpu_intensive_task(cpu_work_size + i);
                })
            );
        }

        // Wait for all results
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

// std::async: CPU-bound tasks for comparison
static void StdAsync_CPUBound(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int cpu_work_size = 10000;
    
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Submit all tasks
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                std::async(std::launch::async, [i, cpu_work_size] { 
                    return cpu_intensive_task(cpu_work_size + i); 
                })
            );
        }
        
        // Wait for all results
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

// Thread Pool: I/O-bound tasks
static void ThreadPool_IOBound(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const auto io_duration = std::chrono::microseconds(1000);  // 1ms per task
    thread_pool pool;

    for (auto _ : state) {
        std::vector<std::future<void>> futures;
        futures.reserve(num_tasks);

        auto start = std::chrono::high_resolution_clock::now();

        // Submit all tasks
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                pool.submit([io_duration] {
                    io_simulation_task(io_duration);
                })
            );
        }

        // Wait for all completions
        for (auto& future : futures) {
            future.get();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["completion_time_us"] = duration.count();
        state.counters["efficiency"] = (num_tasks * io_duration.count()) / static_cast<double>(duration.count());
    }

    state.SetItemsProcessed(num_tasks);
}

// std::async: I/O-bound tasks for comparison
static void StdAsync_IOBound(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const auto io_duration = std::chrono::microseconds(1000);  // 1ms per task
    
    for (auto _ : state) {
        std::vector<std::future<void>> futures;
        futures.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Submit all tasks
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                std::async(std::launch::async, [io_duration] { 
                    io_simulation_task(io_duration); 
                })
            );
        }
        
        // Wait for all completions
        for (auto& future : futures) {
            future.get();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        state.counters["completion_time_us"] = duration.count();
        state.counters["efficiency"] = (num_tasks * io_duration.count()) / static_cast<double>(duration.count());
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Thread Pool: Mixed workload (CPU + I/O)
static void ThreadPool_Mixed(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int cpu_work_size = 5000;
    const auto io_duration = std::chrono::microseconds(500);
    thread_pool pool;

    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);

        auto start = std::chrono::high_resolution_clock::now();

        // Submit mixed tasks
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                pool.submit([i, cpu_work_size, io_duration] {
                    return mixed_task(cpu_work_size + i, io_duration);
                })
            );
        }

        // Wait for all results
        int total_result = 0;
        for (auto& future : futures) {
            total_result += future.get();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        benchmark::DoNotOptimize(total_result);
        state.counters["completion_time_us"] = duration.count();
    }

    state.SetItemsProcessed(num_tasks);
}

// Thread Pool scalability test: varying task granularity
static void ThreadPool_Scalability(benchmark::State& state) {
    const int total_work = 1000000;  // Fixed total work
    const int num_tasks = state.range(0);
    const int work_per_task = total_work / num_tasks;
    thread_pool pool;

    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);

        auto start = std::chrono::high_resolution_clock::now();

        // Submit tasks with varying granularity
        for (int i = 0; i < num_tasks; ++i) {
            futures.emplace_back(
                pool.submit([work_per_task] {
                    return cpu_intensive_task(work_per_task);
                })
            );
        }

        // Wait for all results
        int total_result = 0;
        for (auto& future : futures) {
            total_result += future.get();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        benchmark::DoNotOptimize(total_result);
        state.counters["completion_time_us"] = duration.count();
        state.counters["work_per_task"] = work_per_task;
        state.counters["task_overhead_ns"] = duration.count() * 1000.0 / num_tasks;
    }

    state.SetItemsProcessed(total_work);
}

// Thread Pool: Burst workload pattern
static void ThreadPool_BurstWorkload(benchmark::State& state) {
    const int num_bursts = 5;
    const int tasks_per_burst = state.range(0);
    const int cpu_work_size = 8000;
    thread_pool pool;

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int burst = 0; burst < num_bursts; ++burst) {
            std::vector<std::future<int>> futures;
            futures.reserve(tasks_per_burst);

            // Submit burst of tasks
            for (int i = 0; i < tasks_per_burst; ++i) {
                futures.emplace_back(
                    pool.submit([cpu_work_size] {
                        return cpu_intensive_task(cpu_work_size);
                    })
                );
            }

            // Wait for burst completion
            int burst_result = 0;
            for (auto& future : futures) {
                burst_result += future.get();
            }
            benchmark::DoNotOptimize(burst_result);

            // Small gap between bursts
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["completion_time_us"] = duration.count();
        state.counters["bursts"] = num_bursts;
    }

    state.SetItemsProcessed(num_bursts * tasks_per_burst);
}

// Benchmark configurations
BENCHMARK(ThreadPool_CPUBound)
    ->RangeMultiplier(2)
    ->Range(8, 512)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdAsync_CPUBound)
    ->RangeMultiplier(2)
    ->Range(8, 512)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ThreadPool_IOBound)
    ->RangeMultiplier(2)
    ->Range(16, 256)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(StdAsync_IOBound)
    ->RangeMultiplier(2)
    ->Range(16, 256)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ThreadPool_Mixed)
    ->RangeMultiplier(2)
    ->Range(16, 128)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ThreadPool_Scalability)
    ->RangeMultiplier(4)
    ->Range(1, 1024)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ThreadPool_BurstWorkload)
    ->RangeMultiplier(2)
    ->Range(4, 64)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
