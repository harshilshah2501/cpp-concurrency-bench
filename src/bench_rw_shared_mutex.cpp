#include <benchmark/benchmark.h>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <random>
#include "variance_tracker.hpp"

// =============================================================================
// Reader-Writer Shared Mutex Benchmark
// =============================================================================
// This benchmark measures the performance of std::shared_mutex for read-heavy
// workloads vs regular mutex. Reader-writer locks allow multiple concurrent
// readers but exclusive writer access.
//
// Key insights for interviews:
// - Shared mutex: Multiple readers, single writer
// - Performance break-even point for read/write ratios
// - Writer starvation potential in read-heavy scenarios
// - Cache effects with shared vs exclusive locks
// - Overhead of reader counting mechanisms
//
// Patterns tested:
// 1. Various read/write ratios (50/50, 70/30, 90/10, 95/5)
// 2. Shared mutex vs regular mutex comparison
// 3. Reader fairness analysis
// 4. Writer starvation detection
// =============================================================================

class ReadWriteData {
private:
    mutable std::shared_mutex shared_mtx_;
    mutable std::mutex exclusive_mtx_;
    std::unordered_map<int, int> shared_data_;
    std::unordered_map<int, int> exclusive_data_;

    static void initialize(std::unordered_map<int, int>& data, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            data[static_cast<int>(i)] = static_cast<int>(i * 2);
        }
    }

public:
    ReadWriteData(size_t initial_size = 1000) {
        initialize(shared_data_, initial_size);
        initialize(exclusive_data_, initial_size);
    }

    int shared_read(int key) const {
        std::shared_lock<std::shared_mutex> lock(shared_mtx_);
        auto it = shared_data_.find(key);
        return it != shared_data_.end() ? it->second : -1;
    }

    void shared_write(int key, int value) {
        std::unique_lock<std::shared_mutex> lock(shared_mtx_);
        shared_data_[key] = value;
    }

    int exclusive_read(int key) const {
        std::lock_guard<std::mutex> lock(exclusive_mtx_);
        auto it = exclusive_data_.find(key);
        return it != exclusive_data_.end() ? it->second : -1;
    }

    void exclusive_write(int key, int value) {
        std::lock_guard<std::mutex> lock(exclusive_mtx_);
        exclusive_data_[key] = value;
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(shared_mtx_);
        return shared_data_.size();
    }
};

// Helper function to create random keys
std::vector<int> generate_keys(size_t count, int max_key) {
    std::vector<int> keys;
    keys.reserve(count);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, max_key - 1);
    
    for (size_t i = 0; i < count; ++i) {
        keys.push_back(dis(gen));
    }
    return keys;
}

// Shared Mutex: Read-heavy workload (90% reads, 10% writes)
static void SharedMutex_ReadHeavy_90_10(benchmark::State& state) {
    const int num_threads = state.range(0);
    const size_t operations_per_thread = 10000;
    const int read_percentage = 90;
    
    ReadWriteData data(1000);
    variance_tracker reader_tracker(num_threads);
    variance_tracker writer_tracker(num_threads);
    std::atomic<size_t> total_reads{0};
    std::atomic<size_t> total_writes{0};
    
    auto keys = generate_keys(operations_per_thread, 1000);
    
    for (auto _ : state) {
        reader_tracker.reset();
        writer_tracker.reset();
        total_reads = 0;
        total_writes = 0;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i] {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> percent(0, 99);
                
                for (size_t j = 0; j < operations_per_thread; ++j) {
                    if (percent(gen) < read_percentage) {
                        // Read operation
                        int key = keys[j % keys.size()];
                        benchmark::DoNotOptimize(data.shared_read(key));
                        reader_tracker.record_operation(thread_id);
                        total_reads.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        // Write operation
                        int key = keys[j % keys.size()];
                        data.shared_write(key, static_cast<int>(j));
                        writer_tracker.record_operation(thread_id);
                        total_writes.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.counters["total_reads"] = total_reads.load();
    state.counters["total_writes"] = total_writes.load();
    state.counters["read_percentage"] = (100.0 * total_reads.load()) / (total_reads.load() + total_writes.load());
    state.counters["reader_fairness"] = reader_tracker.coefficient_of_variation();
    state.counters["writer_fairness"] = writer_tracker.coefficient_of_variation();
    state.SetItemsProcessed(total_reads.load() + total_writes.load());
}

// Shared Mutex: Balanced workload (70% reads, 30% writes)
static void SharedMutex_Balanced_70_30(benchmark::State& state) {
    const int num_threads = state.range(0);
    const size_t operations_per_thread = 10000;
    const int read_percentage = 70;
    
    ReadWriteData data(1000);
    std::atomic<size_t> total_reads{0};
    std::atomic<size_t> total_writes{0};
    
    auto keys = generate_keys(operations_per_thread, 1000);
    
    for (auto _ : state) {
        total_reads = 0;
        total_writes = 0;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> percent(0, 99);
                
                for (size_t j = 0; j < operations_per_thread; ++j) {
                    if (percent(gen) < read_percentage) {
                        int key = keys[j % keys.size()];
                        benchmark::DoNotOptimize(data.shared_read(key));
                        total_reads.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        int key = keys[j % keys.size()];
                        data.shared_write(key, static_cast<int>(j));
                        total_writes.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.counters["total_reads"] = total_reads.load();
    state.counters["total_writes"] = total_writes.load();
    state.counters["read_percentage"] = (100.0 * total_reads.load()) / (total_reads.load() + total_writes.load());
    state.SetItemsProcessed(total_reads.load() + total_writes.load());
}

// Exclusive Mutex: Same workload for comparison (90% reads, 10% writes)
static void ExclusiveMutex_ReadHeavy_90_10(benchmark::State& state) {
    const int num_threads = state.range(0);
    const size_t operations_per_thread = 10000;
    const int read_percentage = 90;
    
    ReadWriteData data(1000);
    std::atomic<size_t> total_reads{0};
    std::atomic<size_t> total_writes{0};
    
    auto keys = generate_keys(operations_per_thread, 1000);
    
    for (auto _ : state) {
        total_reads = 0;
        total_writes = 0;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> percent(0, 99);
                
                for (size_t j = 0; j < operations_per_thread; ++j) {
                    if (percent(gen) < read_percentage) {
                        int key = keys[j % keys.size()];
                        benchmark::DoNotOptimize(data.exclusive_read(key));
                        total_reads.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        int key = keys[j % keys.size()];
                        data.exclusive_write(key, static_cast<int>(j));
                        total_writes.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.counters["total_reads"] = total_reads.load();
    state.counters["total_writes"] = total_writes.load();
    state.counters["read_percentage"] = (100.0 * total_reads.load()) / (total_reads.load() + total_writes.load());
    state.SetItemsProcessed(total_reads.load() + total_writes.load());
}

// Write-heavy workload to test writer starvation (50% reads, 50% writes)
static void SharedMutex_WriteHeavy_50_50(benchmark::State& state) {
    const int num_threads = state.range(0);
    const size_t operations_per_thread = 5000;  // Shorter due to higher write contention
    const int read_percentage = 50;
    
    ReadWriteData data(500);
    std::atomic<size_t> total_reads{0};
    std::atomic<size_t> total_writes{0};
    
    auto keys = generate_keys(operations_per_thread, 500);
    
    for (auto _ : state) {
        total_reads = 0;
        total_writes = 0;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> percent(0, 99);
                
                for (size_t j = 0; j < operations_per_thread; ++j) {
                    if (percent(gen) < read_percentage) {
                        int key = keys[j % keys.size()];
                        benchmark::DoNotOptimize(data.shared_read(key));
                        total_reads.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        int key = keys[j % keys.size()];
                        data.shared_write(key, static_cast<int>(j));
                        total_writes.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.counters["total_reads"] = total_reads.load();
    state.counters["total_writes"] = total_writes.load();
    state.counters["read_percentage"] = (100.0 * total_reads.load()) / (total_reads.load() + total_writes.load());
    state.SetItemsProcessed(total_reads.load() + total_writes.load());
}

// Benchmark configurations
BENCHMARK(SharedMutex_ReadHeavy_90_10)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ExclusiveMutex_ReadHeavy_90_10)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(SharedMutex_Balanced_70_30)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(SharedMutex_WriteHeavy_50_50)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
