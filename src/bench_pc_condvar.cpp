#include <benchmark/benchmark.h>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// =============================================================================
// Producer-Consumer with Condition Variables
// =============================================================================

static void PC_CondVar(benchmark::State& state) {
    const int producers = static_cast<int>(state.range(0));
    const int consumers = static_cast<int>(state.range(1));
    const int N = 1'000'000;

    for (auto _ : state) {
        std::mutex m;
        std::condition_variable cv;
        std::queue<int> q;
        bool done = false;

        auto prod = [&] {
            const int per = N / producers;
            for (int i = 0; i < per; ++i) {
                {
                    std::lock_guard<std::mutex> lk(m);
                    q.push(i);
                }
                cv.notify_one();
            }
        };

        auto cons = [&] {
            int x = 0;
            for (;;) {
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk, [&] { return !q.empty() || done; });
                if (q.empty() && done) {
                    break;
                }
                x += q.front();
                q.pop();
            }
            benchmark::DoNotOptimize(x);
        };

        std::vector<std::thread> ps;
        std::vector<std::thread> cs;
        ps.reserve(static_cast<size_t>(producers));
        cs.reserve(static_cast<size_t>(consumers));

        for (int i = 0; i < producers; ++i) {
            ps.emplace_back(prod);
        }
        for (int i = 0; i < consumers; ++i) {
            cs.emplace_back(cons);
        }
        for (auto& t : ps) {
            t.join();
        }
        {
            std::lock_guard<std::mutex> lk(m);
            done = true;
        }
        cv.notify_all();
        for (auto& t : cs) {
            t.join();
        }
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * N);
}

BENCHMARK(PC_CondVar)
    ->Args({1, 1})
    ->Args({2, 2})
    ->Args({4, 4})
    ->UseRealTime();

BENCHMARK_MAIN();
