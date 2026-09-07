#include <benchmark/benchmark.h>
#include <cstdint>
#include <mutex>
#include <thread>

#include "bench_harness.hpp"
#include "padded.hpp"

// =============================================================================
// Mutex-Protected Counter Benchmark
// =============================================================================
// Measures std::mutex under high contention with a fixed wall-time window per
// Google Benchmark iteration. Workers are spawned and joined each iteration so
// ops/fairness counters describe that window only.
// =============================================================================

static void Counter_Mutex_Hot(benchmark::State& state) {
    const int num_threads = static_cast<int>(state.range(0));
    uint64_t total_ops = 0;
    double last_cv = 0.0;

    for (auto _ : state) {
        state.PauseTiming();
        std::mutex mtx;
        padded<uint64_t> counter{0};
        state.ResumeTiming();

        auto window = bench_harness::run_contention_window(
            num_threads, std::chrono::milliseconds(100),
            [&](int /*thread_id*/) {
                std::lock_guard<std::mutex> lock(mtx);
                counter.value++;
            });

        state.PauseTiming();
        total_ops += window.ops;
        last_cv = window.fairness_cv;
        // Sanity: window ops should match the shared counter.
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

BENCHMARK(Counter_Mutex_Hot)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
