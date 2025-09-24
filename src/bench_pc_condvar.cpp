// src/bench_pc_condvar.cpp
#include <benchmark/benchmark.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
static void PC_CondVar(benchmark::State& st) {
  const int producers = st.range(0), consumers = st.range(1), N = 1'000'000;
  std::mutex m; std::condition_variable cv; std::queue<int> q; bool done=false;
  auto prod = [&]{ for (int i=0;i<N/producers;i++){ {std::lock_guard lk(m); q.push(i);} cv.notify_one(); } };
  auto cons = [&]{ int x=0; for(;;){ std::unique_lock lk(m);
      cv.wait(lk,[&]{ return !q.empty()||done;});
      if (q.empty() && done) break;
      x += q.front(); q.pop();
    } benchmark::DoNotOptimize(x); };
  for (auto _ : st) {
    done=false; while(!q.empty()) q.pop();
    std::vector<std::thread> ps(producers), cs(consumers);
    for(auto& t:ps) t=std::thread(prod);
    for(auto& t:cs) t=std::thread(cons);
    for(auto& t:ps) t.join();
    { std::lock_guard lk(m); done=true; } cv.notify_all();
    for(auto& t:cs) t.join();
  }
}
BENCHMARK(PC_CondVar)->Args({1,1})->Args({2,2})->Args({4,4})->UseRealTime();
BENCHMARK_MAIN();