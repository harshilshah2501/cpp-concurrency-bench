#include <benchmark/benchmark.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

// =============================================================================
// Coroutines vs Thread Pool Benchmark
// =============================================================================
// Lightweight coroutine tasks with a joinable timer thread pool for async
// delays — avoids detached-thread use-after-free on awaiter temporaries.
// =============================================================================

class TimerService {
public:
    TimerService() {
        worker_ = std::thread([this] { run(); });
    }

    ~TimerService() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            stop_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    TimerService(const TimerService&) = delete;
    TimerService& operator=(const TimerService&) = delete;

    void schedule(std::chrono::microseconds delay, std::coroutine_handle<> h) {
        const auto when = std::chrono::steady_clock::now() + delay;
        {
            std::lock_guard<std::mutex> lock(mu_);
            heap_.push(Entry{when, h});
        }
        cv_.notify_one();
    }

private:
    struct Entry {
        std::chrono::steady_clock::time_point when;
        std::coroutine_handle<> handle;
        bool operator>(const Entry& other) const { return when > other.when; }
    };

    void run() {
        std::unique_lock<std::mutex> lock(mu_);
        while (true) {
            if (stop_ && heap_.empty()) {
                return;
            }
            if (heap_.empty()) {
                cv_.wait(lock, [&] { return stop_ || !heap_.empty(); });
                continue;
            }
            auto entry = heap_.top();
            if (entry.when > std::chrono::steady_clock::now()) {
                cv_.wait_until(lock, entry.when);
                continue;
            }
            heap_.pop();
            lock.unlock();
            entry.handle.resume();
            lock.lock();
        }
    }

    std::mutex mu_;
    std::condition_variable cv_;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> heap_;
    bool stop_ = false;
    std::thread worker_;
};

struct Task {
    struct promise_type {
        std::coroutine_handle<> continuation = std::noop_coroutine();
        std::atomic<bool>* done_flag = nullptr;

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        auto final_suspend() noexcept {
            struct awaiter {
                std::coroutine_handle<> continuation;
                std::atomic<bool>* done_flag;
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<>) noexcept {
                    if (done_flag) {
                        done_flag->store(true, std::memory_order_release);
                    }
                    return continuation;
                }
                void await_resume() noexcept {}
            };
            return awaiter{continuation, done_flag};
        }

        void unhandled_exception() {}
        void return_void() {}
    };

    std::coroutine_handle<promise_type> h;

    explicit Task(std::coroutine_handle<promise_type> handle) : h(handle) {}

    ~Task() {
        if (h) {
            h.destroy();
        }
    }

    Task(Task&& other) noexcept : h(std::exchange(other.h, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (h) {
                h.destroy();
            }
            h = std::exchange(other.h, {});
        }
        return *this;
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    void set_done_flag(std::atomic<bool>* flag) {
        if (h) {
            h.promise().done_flag = flag;
        }
    }

    void resume() {
        if (h && !h.done()) {
            h.resume();
        }
    }

    bool done() const { return !h || h.done(); }
};

struct IoAwaiter {
    TimerService* timers;
    std::chrono::microseconds duration;

    bool await_ready() noexcept { return duration.count() <= 0; }

    void await_suspend(std::coroutine_handle<> h) {
        // Capture duration by value via member copy already stored on *this
        // before suspend; schedule through the owned TimerService so the
        // awaiter temporary can be destroyed safely.
        timers->schedule(duration, h);
    }

    void await_resume() noexcept {}
};

struct YieldAwaiter {
    TimerService* timers;

    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        timers->schedule(std::chrono::microseconds(0), h);
    }
    void await_resume() noexcept {}
};

static Task async_cpu_task(TimerService& timers, int work_size) {
    volatile int result = 0;
    for (int i = 0; i < work_size / 2; ++i) {
        result += i * i;
    }
    co_await YieldAwaiter{&timers};
    for (int i = work_size / 2; i < work_size; ++i) {
        result += i * i;
    }
    benchmark::DoNotOptimize(result);
}

static Task async_io_task(TimerService& timers, std::chrono::microseconds duration) {
    co_await IoAwaiter{&timers, duration};
}

static Task async_mixed_task(TimerService& timers, int cpu_work,
                             std::chrono::microseconds io_duration) {
    volatile int result = 0;
    for (int i = 0; i < cpu_work / 2; ++i) {
        result += i * i;
    }
    co_await IoAwaiter{&timers, io_duration};
    for (int i = cpu_work / 2; i < cpu_work; ++i) {
        result += i * i;
    }
    benchmark::DoNotOptimize(result);
}

static void wait_all(std::vector<std::atomic<bool>>& done_flags) {
    while (true) {
        bool all_done = true;
        for (auto& flag : done_flags) {
            if (!flag.load(std::memory_order_acquire)) {
                all_done = false;
                break;
            }
        }
        if (all_done) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

static void Coroutines_CPUBound(benchmark::State& state) {
    const int num_tasks = static_cast<int>(state.range(0));
    const int cpu_work_size = 10000;

    for (auto _ : state) {
        TimerService timers;
        std::vector<Task> tasks;
        std::vector<std::atomic<bool>> done(static_cast<size_t>(num_tasks));
        tasks.reserve(static_cast<size_t>(num_tasks));

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_tasks; ++i) {
            done[static_cast<size_t>(i)].store(false, std::memory_order_relaxed);
            tasks.emplace_back(async_cpu_task(timers, cpu_work_size));
            tasks.back().set_done_flag(&done[static_cast<size_t>(i)]);
            tasks.back().resume();
        }
        wait_all(done);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["completion_time_us"] =
            static_cast<double>(duration.count());
        state.counters["throughput"] =
            (1000000.0 * num_tasks) / static_cast<double>(duration.count());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * num_tasks);
}

static void Threads_CPUBound(benchmark::State& state) {
    const int num_tasks = static_cast<int>(state.range(0));
    const int cpu_work_size = 10000;

    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(num_tasks));

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_tasks; ++i) {
            threads.emplace_back([cpu_work_size] {
                volatile int result = 0;
                for (int j = 0; j < cpu_work_size; ++j) {
                    result += j * j;
                }
                benchmark::DoNotOptimize(result);
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["completion_time_us"] =
            static_cast<double>(duration.count());
        state.counters["throughput"] =
            (1000000.0 * num_tasks) / static_cast<double>(duration.count());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * num_tasks);
}

static void Coroutines_IOBound(benchmark::State& state) {
    const int num_tasks = static_cast<int>(state.range(0));
    const auto io_duration = std::chrono::microseconds(1000);

    for (auto _ : state) {
        TimerService timers;
        std::vector<Task> tasks;
        std::vector<std::atomic<bool>> done(static_cast<size_t>(num_tasks));
        tasks.reserve(static_cast<size_t>(num_tasks));

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_tasks; ++i) {
            done[static_cast<size_t>(i)].store(false, std::memory_order_relaxed);
            tasks.emplace_back(async_io_task(timers, io_duration));
            tasks.back().set_done_flag(&done[static_cast<size_t>(i)]);
            tasks.back().resume();
        }
        wait_all(done);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["completion_time_us"] =
            static_cast<double>(duration.count());
        state.counters["efficiency"] =
            (num_tasks * io_duration.count()) /
            static_cast<double>(duration.count());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * num_tasks);
}

static void Coroutines_HighTaskCount(benchmark::State& state) {
    const int num_tasks = static_cast<int>(state.range(0));
    const int cpu_work_size = 1000;

    for (auto _ : state) {
        TimerService timers;
        std::vector<Task> tasks;
        std::vector<std::atomic<bool>> done(static_cast<size_t>(num_tasks));
        tasks.reserve(static_cast<size_t>(num_tasks));

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_tasks; ++i) {
            done[static_cast<size_t>(i)].store(false, std::memory_order_relaxed);
            tasks.emplace_back(async_cpu_task(timers, cpu_work_size));
            tasks.back().set_done_flag(&done[static_cast<size_t>(i)]);
            tasks.back().resume();
        }
        wait_all(done);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["completion_time_us"] =
            static_cast<double>(duration.count());
        state.counters["memory_efficiency"] =
            static_cast<double>(num_tasks) /
            static_cast<double>(std::max<int64_t>(1, duration.count()));
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * num_tasks);
}

static void Coroutines_Mixed(benchmark::State& state) {
    const int num_tasks = static_cast<int>(state.range(0));
    const int cpu_work_size = 5000;
    const auto io_duration = std::chrono::microseconds(500);

    for (auto _ : state) {
        TimerService timers;
        std::vector<Task> tasks;
        std::vector<std::atomic<bool>> done(static_cast<size_t>(num_tasks));
        tasks.reserve(static_cast<size_t>(num_tasks));

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_tasks; ++i) {
            done[static_cast<size_t>(i)].store(false, std::memory_order_relaxed);
            tasks.emplace_back(
                async_mixed_task(timers, cpu_work_size, io_duration));
            tasks.back().set_done_flag(&done[static_cast<size_t>(i)]);
            tasks.back().resume();
        }
        wait_all(done);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["completion_time_us"] =
            static_cast<double>(duration.count());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * num_tasks);
}

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
