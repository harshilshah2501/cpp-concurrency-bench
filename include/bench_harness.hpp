#pragma once

#include "affinity.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// =============================================================================
// Shared multi-thread contention harness for counter-style benchmarks.
// =============================================================================
// Spawns worker threads each iteration, measures a fixed wall-time window, and
// tears threads down before the next Google Benchmark iteration. This avoids
// the cumulative-counter / racy start-stop issues of long-lived workers.
// =============================================================================

namespace bench_harness {

inline void maybe_pin(int thread_index) {
    if (!affinity::is_affinity_supported()) {
        return;
    }
    const auto cores = affinity::get_available_cores();
    if (cores.empty()) {
        return;
    }
    // Leave core 0 freer for the main / OS threads when possible.
    const int core = cores[static_cast<size_t>(thread_index + 1) % cores.size()];
    affinity::pin_thread_to_core(core);
}

struct WindowResult {
    uint64_t ops = 0;
    double fairness_cv = 0.0;
};

// Run `num_threads` workers for `window`. Each worker repeatedly invokes
// `critical_section(thread_id)`. If the callable returns bool, only `true`
// results are counted as ops (useful for try_lock fairness). Void callables
// count every invocation.
template <typename Fn>
WindowResult run_contention_window(
    int num_threads,
    std::chrono::milliseconds window,
    Fn&& critical_section) {
    std::atomic<bool> start{false};
    std::atomic<bool> stop{false};
    std::vector<std::atomic<uint64_t>> counts(static_cast<size_t>(num_threads));
    for (auto& c : counts) {
        c.store(0, std::memory_order_relaxed);
    }

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i] {
            maybe_pin(thread_id);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            while (!stop.load(std::memory_order_acquire)) {
                if constexpr (std::is_void_v<std::invoke_result_t<Fn&, int>>) {
                    critical_section(thread_id);
                    counts[static_cast<size_t>(thread_id)].fetch_add(
                        1, std::memory_order_relaxed);
                } else {
                    if (critical_section(thread_id)) {
                        counts[static_cast<size_t>(thread_id)].fetch_add(
                            1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    start.store(true, std::memory_order_release);
    std::this_thread::sleep_for(window);
    stop.store(true, std::memory_order_release);

    for (auto& t : threads) {
        t.join();
    }

    WindowResult result;
    double mean = 0.0;
    for (auto& c : counts) {
        const uint64_t v = c.load(std::memory_order_relaxed);
        result.ops += v;
        mean += static_cast<double>(v);
    }
    mean /= static_cast<double>(num_threads);

    if (mean > 0.0) {
        double variance = 0.0;
        for (auto& c : counts) {
            const double d =
                static_cast<double>(c.load(std::memory_order_relaxed)) - mean;
            variance += d * d;
        }
        variance /= static_cast<double>(num_threads);
        result.fairness_cv = std::sqrt(variance) / mean;
    }
    return result;
}

}  // namespace bench_harness
