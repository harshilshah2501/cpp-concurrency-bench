#include <benchmark/benchmark.h>
#include <atomic>
#include <cstdint>
#include <thread>

#include "bench_harness.hpp"
#include "padded.hpp"

// =============================================================================
// Atomic Counter Benchmark
// =============================================================================
// Compares memory orderings under contention. Each iteration uses a fresh
// counter and a fixed wall-time worker window via bench_harness.
// =============================================================================

template <std::memory_order Order>
static void Counter_Atomic_Impl(benchmark::State& state) {
    const int num_threads = static_cast<int>(state.range(0));
    uint64_t total_ops = 0;
    double last_cv = 0.0;

    for (auto _ : state) {
        state.PauseTiming();
        padded<std::atomic<uint64_t>> counter{0};
        state.ResumeTiming();

        auto window = bench_harness::run_contention_window(
            num_threads, std::chrono::milliseconds(100),
            [&](int /*thread_id*/) {
                counter.fetch_add(1, Order);
            });

        state.PauseTiming();
        total_ops += window.ops;
        last_cv = window.fairness_cv;
        benchmark::DoNotOptimize(counter.load());
        state.ResumeTiming();
    }

    state.counters["ops_total"] = static_cast<double>(total_ops);
    state.counters["fairness_cv"] = last_cv;
    state.SetItemsProcessed(static_cast<int64_t>(total_ops));
}

static void Counter_Atomic_SeqCst(benchmark::State& state) {
    Counter_Atomic_Impl<std::memory_order_seq_cst>(state);
}

static void Counter_Atomic_Relaxed(benchmark::State& state) {
    Counter_Atomic_Impl<std::memory_order_relaxed>(state);
}

static void Counter_Atomic_AcqRel(benchmark::State& state) {
    Counter_Atomic_Impl<std::memory_order_acq_rel>(state);
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
