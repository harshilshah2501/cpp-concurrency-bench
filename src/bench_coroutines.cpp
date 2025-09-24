#include <benchmark/benchmark.h>
#include <coroutine>
#include <thread>
#include <vector>
#include <queue>
#include <future>
#include <functional>
#include <chrono>
#include <random>
#include <atomic>

// =============================================================================
// Coroutines vs Thread Pool Benchmark
// =============================================================================
// This benchmark compares C++20 coroutines against traditional thread-based
// approaches for asynchronous task execution patterns.
//
// Key insights for interviews:
// - Coroutines: Lightweight, stackless, cooperative multitasking
// - Thread-based: Preemptive multitasking, higher memory overhead
// - Async/await patterns vs callback-based approaches
// - Memory efficiency: coroutine frames vs thread stacks
// - Context switching overhead differences
//
// Patterns tested:
// 1. Simple async task execution
// 2. I/O simulation with suspend/resume
// 3. Task chaining and composition
// 4. Memory usage comparison
// 5. Scalability with high task counts
// =============================================================================

// Simple coroutine task type
struct Task {
    struct promise_type {
        std::coroutine_handle<> continuation = std::noop_coroutine();
        
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() noexcept { return {}; }
        
        auto final_suspend() noexcept {
            struct awaiter {
                std::coroutine_handle<> continuation;
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
                    return continuation;
                }
                void await_resume() noexcept {}
            };
            return awaiter{continuation};
        }
        
        void unhandled_exception() {}
        void return_void() {}
    };
    
    std::coroutine_handle<promise_type> h;
    
    explicit Task(std::coroutine_handle<promise_type> handle) : h(handle) {}
    
    ~Task() {
        if (h) h.destroy();
    }
    
    Task(Task&& other) noexcept : h(std::exchange(other.h, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (h) h.destroy();
            h = std::exchange(other.h, {});
        }
        return *this;
    }
    
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    
    void resume() {
        if (h && !h.done()) {
            h.resume();
        }
    }
    
    bool done() const {
        return h.done();
    }
};

// Awaitable for simulating async I/O
struct IoAwaiter {
    std::chrono::microseconds duration;
    
    bool await_ready() noexcept { return false; }
    
    void await_suspend(std::coroutine_handle<> h) {
        // Simulate async I/O by scheduling resumption after delay
        std::thread([h, this] {
            std::this_thread::sleep_for(duration);
            h.resume();
        }).detach();
    }
    
    void await_resume() noexcept {}
};

// Awaitable for yielding control
struct YieldAwaiter {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        // Yield control - resume immediately
        std::thread([h] { h.resume(); }).detach();
    }
    void await_resume() noexcept {}
};

// Coroutine-based async task
Task async_cpu_task(int work_size) {
    // Simulate some initial work
    volatile int result = 0;
    for (int i = 0; i < work_size / 2; ++i) {
        result += i * i;
    }
    
    // Yield control to allow other coroutines to run
    co_await YieldAwaiter{};
    
    // Continue with remaining work
    for (int i = work_size / 2; i < work_size; ++i) {
        result += i * i;
    }
    
    benchmark::DoNotOptimize(result);
}

// Coroutine-based I/O task
Task async_io_task(std::chrono::microseconds duration) {
    // Simulate async I/O operation
    co_await IoAwaiter{duration};
}

// Coroutine-based mixed task
Task async_mixed_task(int cpu_work, std::chrono::microseconds io_duration) {
    // Initial CPU work
    volatile int result = 0;
    for (int i = 0; i < cpu_work / 2; ++i) {
        result += i * i;
    }
    
    // Async I/O
    co_await IoAwaiter{io_duration};
    
    // Final CPU work
    for (int i = cpu_work / 2; i < cpu_work; ++i) {
        result += i * i;
    }
    
    benchmark::DoNotOptimize(result);
}

// Simple scheduler for coroutines
class SimpleScheduler {
private:
    std::queue<std::coroutine_handle<>> ready_queue;
    std::mutex queue_mutex;
    std::atomic<bool> running{true};
    
public:
    void schedule(std::coroutine_handle<> h) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        ready_queue.push(h);
    }
    
    void run_until_complete() {
        while (running.load()) {
            std::coroutine_handle<> h;
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (ready_queue.empty()) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    continue;
                }
                h = ready_queue.front();
                ready_queue.pop();
            }
            
            if (h && !h.done()) {
                h.resume();
            }
        }
    }
    
    void stop() {
        running = false;
    }
    
    bool has_work() {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return !ready_queue.empty();
    }
};

// Coroutines: CPU-bound tasks
static void Coroutines_CPUBound(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int cpu_work_size = 10000;
    
    for (auto _ : state) {
        std::vector<Task> tasks;
        tasks.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create all coroutine tasks
        for (int i = 0; i < num_tasks; ++i) {
            tasks.emplace_back(async_cpu_task(cpu_work_size));
        }
        
        // Execute all tasks (simple sequential execution for comparison)
        for (auto& task : tasks) {
            task.resume();
            while (!task.done()) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        state.counters["completion_time_us"] = duration.count();
        state.counters["throughput"] = (1000000.0 * num_tasks) / duration.count();
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Thread-based CPU tasks for comparison
static void Threads_CPUBound(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int cpu_work_size = 10000;
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create all threads
        for (int i = 0; i < num_tasks; ++i) {
            threads.emplace_back([cpu_work_size] {
                volatile int result = 0;
                for (int j = 0; j < cpu_work_size; ++j) {
                    result += j * j;
                }
                benchmark::DoNotOptimize(result);
            });
        }
        
        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        state.counters["completion_time_us"] = duration.count();
        state.counters["throughput"] = (1000000.0 * num_tasks) / duration.count();
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Coroutines: I/O simulation
static void Coroutines_IOBound(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const auto io_duration = std::chrono::microseconds(1000);
    
    for (auto _ : state) {
        std::vector<Task> tasks;
        tasks.reserve(num_tasks);
        std::atomic<int> completed{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create all I/O tasks
        for (int i = 0; i < num_tasks; ++i) {
            tasks.emplace_back(async_io_task(io_duration));
        }
        
        // Start all tasks
        for (auto& task : tasks) {
            task.resume();
        }
        
        // Wait for completion (simplified polling)
        while (completed.load() < num_tasks) {
            for (auto& task : tasks) {
                if (task.done() && completed.load() < num_tasks) {
                    completed.fetch_add(1);
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        state.counters["completion_time_us"] = duration.count();
        state.counters["efficiency"] = (num_tasks * io_duration.count()) / static_cast<double>(duration.count());
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Coroutines: Memory usage pattern (high task count)
static void Coroutines_HighTaskCount(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int cpu_work_size = 1000;  // Smaller work per task
    
    for (auto _ : state) {
        std::vector<Task> tasks;
        tasks.reserve(num_tasks);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create many lightweight coroutine tasks
        for (int i = 0; i < num_tasks; ++i) {
            tasks.emplace_back(async_cpu_task(cpu_work_size));
        }
        
        // Execute with cooperative scheduling
        bool all_done = false;
        while (!all_done) {
            all_done = true;
            for (auto& task : tasks) {
                if (!task.done()) {
                    task.resume();
                    all_done = false;
                }
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        state.counters["completion_time_us"] = duration.count();
        state.counters["memory_efficiency"] = static_cast<double>(num_tasks) / duration.count();
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Mixed coroutine pattern
static void Coroutines_Mixed(benchmark::State& state) {
    const int num_tasks = state.range(0);
    const int cpu_work_size = 5000;
    const auto io_duration = std::chrono::microseconds(500);
    
    for (auto _ : state) {
        std::vector<Task> tasks;
        tasks.reserve(num_tasks);
        std::atomic<int> completed{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create mixed tasks
        for (int i = 0; i < num_tasks; ++i) {
            tasks.emplace_back(async_mixed_task(cpu_work_size, io_duration));
        }
        
        // Start all tasks
        for (auto& task : tasks) {
            task.resume();
        }
        
        // Cooperative execution
        while (completed.load() < num_tasks) {
            completed = 0;
            for (auto& task : tasks) {
                if (task.done()) {
                    completed.fetch_add(1);
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        state.counters["completion_time_us"] = duration.count();
    }
    
    state.SetItemsProcessed(num_tasks);
}

// Benchmark configurations
BENCHMARK(Coroutines_CPUBound)
    ->RangeMultiplier(2)
    ->Range(8, 128)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(Threads_CPUBound)
    ->RangeMultiplier(2)
    ->Range(8, 128)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(Coroutines_IOBound)
    ->RangeMultiplier(2)
    ->Range(16, 256)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(Coroutines_HighTaskCount)
    ->RangeMultiplier(4)
    ->Range(64, 4096)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(Coroutines_Mixed)
    ->RangeMultiplier(2)
    ->Range(16, 128)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
