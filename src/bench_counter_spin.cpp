#include <benchmark/benchmark.h>
#include <cstdint>
#include <thread>

#include "bench_harness.hpp"
#include "padded.hpp"
#include "spinlock.hpp"

// =============================================================================
// Spinlock-Protected Counter Benchmark
// =============================================================================

static void Counter_Spin_Hot(benchmark::State& state) {
    const int num_threads = static_cast<int>(state.range(0));
    uint64_t total_ops = 0;
    double last_cv = 0.0;

    for (auto _ : state) {
        state.PauseTiming();
        spinlock lock;
        padded<uint64_t> counter{0};
        state.ResumeTiming();

        auto window = bench_harness::run_contention_window(
            num_threads, std::chrono::milliseconds(100),
            [&](int /*thread_id*/) {
                lock.lock();
                counter.value++;
                lock.unlock();
            });

        state.PauseTiming();
        total_ops += window.ops;
        last_cv = window.fairness_cv;
        benchmark::DoNotOptimize(counter.value);
        state.ResumeTiming();
    }

    state.counters["ops_total"] = static_cast<double>(total_ops);
    state.counters["fairness_cv"] = last_cv;
    state.counters["ops_per_thread"] =
        static_cast<double>(total_ops) /
        (static_cast<double>(num_threads) * state.iterations());
    state.SetItemsProcessed(static_cast<int64_t>(total_ops));
}

static void Counter_Spin_WithWork(benchmark::State& state) {
    const int num_threads = static_cast<int>(state.range(0));
    uint64_t total_ops = 0;
    double last_cv = 0.0;

    for (auto _ : state) {
        state.PauseTiming();
        spinlock lock;
        padded<uint64_t> counter{0};
        state.ResumeTiming();

        auto window = bench_harness::run_contention_window(
            num_threads, std::chrono::milliseconds(100),
            [&](int /*thread_id*/) {
                lock.lock();
                counter.value++;
                volatile int dummy = 0;
                for (int j = 0; j < 10; ++j) {
                    dummy += j;
                }
                benchmark::DoNotOptimize(dummy);
                lock.unlock();
            });

        state.PauseTiming();
        total_ops += window.ops;
        last_cv = window.fairness_cv;
        state.ResumeTiming();
    }

    state.counters["ops_total"] = static_cast<double>(total_ops);
    state.counters["fairness_cv"] = last_cv;
    state.SetItemsProcessed(static_cast<int64_t>(total_ops));
}

static void Counter_Spin_Fairness(benchmark::State& state) {
    const int num_threads = static_cast<int>(state.range(0));
    uint64_t total_ops = 0;
    double last_cv = 0.0;

    for (auto _ : state) {
        state.PauseTiming();
        spinlock lock;
        padded<uint64_t> counter{0};
        state.ResumeTiming();

        auto window = bench_harness::run_contention_window(
            num_threads, std::chrono::milliseconds(200),
            [&](int /*thread_id*/) -> bool {
                if (lock.try_lock()) {
                    counter.value++;
                    lock.unlock();
                    return true;
                }
                std::this_thread::yield();
                return false;
            });

        state.PauseTiming();
        total_ops += window.ops;
        last_cv = window.fairness_cv;
        state.ResumeTiming();
    }

    state.counters["ops_total"] = static_cast<double>(total_ops);
    state.counters["fairness_cv"] = last_cv;
    state.SetItemsProcessed(static_cast<int64_t>(total_ops));
}

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
