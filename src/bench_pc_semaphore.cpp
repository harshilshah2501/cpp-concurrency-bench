#include <benchmark/benchmark.h>
#include <semaphore>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <atomic>
#include "variance_tracker.hpp"

// =============================================================================
// Producer-Consumer with Semaphore Benchmark
// =============================================================================
// This benchmark compares semaphore-based coordination vs condition variables.
// Semaphores provide a more direct signaling mechanism compared to condition
// variables, which can result in better performance for certain patterns.
//
// Key insights for interviews:
// - Semaphores: Direct counting mechanism, no spurious wakeups
// - Condition Variables: More flexible but higher overhead
// - Binary vs Counting semaphores performance differences
// - Memory ordering implications with semaphore operations
// - RAII patterns with semaphore acquisition
//
// C++20 std::counting_semaphore provides:
// - Atomic counter with acquire/release semantics
// - FIFO ordering guarantees (implementation dependent)
// - Exception safety with RAII
// - Timeout support (try_acquire_for, try_acquire_until)
// =============================================================================

template<typename T>
class SemaphoreQueue {
private:
    mutable std::mutex mtx_;
    std::queue<T> queue_;
    std::counting_semaphore<> empty_slots_;  // Available space in queue
    std::counting_semaphore<> filled_slots_; // Items available to consume
    size_t max_size_;
    std::atomic<bool> shutdown_;

public:
    explicit SemaphoreQueue(size_t max_size = 1000) 
        : empty_slots_(max_size)    // Initially all slots are empty
        , filled_slots_(0)          // Initially no items to consume
        , max_size_(max_size)
        , shutdown_(false) {}
    
    bool push(const T& item) {
        if (shutdown_.load(std::memory_order_acquire)) return false;
        
        // Wait for available space
        empty_slots_.acquire();
        
        if (shutdown_.load(std::memory_order_acquire)) {
            empty_slots_.release(); // Return the slot we acquired
            return false;
        }
        
        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(item);
        }
        
        // Signal that an item is available
        filled_slots_.release();
        return true;
    }
    
    bool pop(T& item) {
        if (shutdown_.load(std::memory_order_acquire)) return false;
        
        // Wait for available item
        filled_slots_.acquire();
        
        if (shutdown_.load(std::memory_order_acquire)) {
            filled_slots_.release(); // Return the item we acquired
            return false;
        }
        
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (queue_.empty()) return false; // Should not happen with proper semaphore usage
            item = queue_.front();
            queue_.pop();
        }
        
        // Signal that a slot is available
        empty_slots_.release();
        return true;
    }
    
    void shutdown() {
        shutdown_.store(true, std::memory_order_release);
        
        // Release all waiting threads by making semaphores available
        for (size_t i = 0; i < max_size_; ++i) {
            empty_slots_.release();
            filled_slots_.release();
        }
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }
};

// Single Producer, Single Consumer with Semaphores
static void ProducerConsumer_SPSC_Semaphore(benchmark::State& state) {
    const size_t queue_capacity = state.range(0);
    const size_t items_per_iteration = 10000;
    
    SemaphoreQueue<int> queue(queue_capacity);
    std::atomic<size_t> items_produced{0};
    std::atomic<size_t> items_consumed{0};
    
    for (auto _ : state) {
        items_produced = 0;
        items_consumed = 0;
        
        // Producer thread
        std::thread producer([&] {
            for (size_t i = 0; i < items_per_iteration; ++i) {
                queue.push(static_cast<int>(i));
                items_produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
        
        // Consumer thread
        std::thread consumer([&] {
            int item;
            while (items_consumed.load(std::memory_order_relaxed) < items_per_iteration) {
                if (queue.pop(item)) {
                    items_consumed.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
        
        producer.join();
        consumer.join();
        queue.shutdown();
    }
    
    state.counters["queue_capacity"] = queue_capacity;
    state.counters["items_processed"] = items_per_iteration;
    state.SetItemsProcessed(items_per_iteration);
}

// Multiple Producers, Multiple Consumers with Semaphores
static void ProducerConsumer_MPMC_Semaphore(benchmark::State& state) {
    const size_t num_threads = state.range(0);
    const size_t num_producers = num_threads / 2;
    const size_t num_consumers = num_threads - num_producers;
    const size_t items_per_producer = 1000;
    const size_t total_items = num_producers * items_per_producer;
    
    if (num_producers == 0 || num_consumers == 0) {
        state.SkipWithError("Need at least 1 producer and 1 consumer");
        return;
    }
    
    SemaphoreQueue<int> queue(50);  // Moderate queue size for contention
    variance_tracker producer_tracker(num_producers);
    variance_tracker consumer_tracker(num_consumers);
    std::atomic<size_t> items_consumed{0};
    
    for (auto _ : state) {
        producer_tracker.reset();
        consumer_tracker.reset();
        items_consumed = 0;
        
        // Producer threads
        std::vector<std::thread> producers;
        for (size_t i = 0; i < num_producers; ++i) {
            producers.emplace_back([&, producer_id = i] {
                for (size_t j = 0; j < items_per_producer; ++j) {
                    int item = static_cast<int>(producer_id * items_per_producer + j);
                    queue.push(item);
                    producer_tracker.record_operation(producer_id);
                }
            });
        }
        
        // Consumer threads
        std::vector<std::thread> consumers;
        for (size_t i = 0; i < num_consumers; ++i) {
            consumers.emplace_back([&, consumer_id = i] {
                int item;
                while (items_consumed.load(std::memory_order_relaxed) < total_items) {
                    if (queue.pop(item)) {
                        items_consumed.fetch_add(1, std::memory_order_relaxed);
                        consumer_tracker.record_operation(consumer_id);
                    }
                }
            });
        }
        
        for (auto& producer : producers) {
            producer.join();
        }
        for (auto& consumer : consumers) {
            consumer.join();
        }
        queue.shutdown();
    }
    
    state.counters["num_producers"] = num_producers;
    state.counters["num_consumers"] = num_consumers;
    state.counters["producer_fairness"] = producer_tracker.coefficient_of_variation();
    state.counters["consumer_fairness"] = consumer_tracker.coefficient_of_variation();
    state.counters["total_items"] = total_items;
    state.SetItemsProcessed(total_items);
}

// Binary Semaphore vs Mutex comparison
static void BinarySemaphore_vs_Mutex(benchmark::State& state) {
    const int num_threads = state.range(0);
    const size_t operations_per_thread = 10000;
    
    // Binary semaphore (max count = 1)
    std::binary_semaphore binary_sem{1};
    std::atomic<uint64_t> binary_counter{0};
    
    // Mutex for comparison
    std::mutex mtx;
    std::atomic<uint64_t> mutex_counter{0};
    
    variance_tracker binary_tracker(num_threads);
    variance_tracker mutex_tracker(num_threads);
    
    for (auto _ : state) {
        binary_tracker.reset();
        mutex_tracker.reset();
        binary_counter = 0;
        mutex_counter = 0;
        
        std::vector<std::thread> threads;
        
        // Test binary semaphore
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i] {
                for (size_t j = 0; j < operations_per_thread; ++j) {
                    binary_sem.acquire();
                    binary_counter.fetch_add(1, std::memory_order_relaxed);
                    binary_sem.release();
                    binary_tracker.record_operation(thread_id);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        threads.clear();
        
        // Test mutex
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i] {
                for (size_t j = 0; j < operations_per_thread; ++j) {
                    std::lock_guard<std::mutex> lock(mtx);
                    mutex_counter.fetch_add(1, std::memory_order_relaxed);
                    mutex_tracker.record_operation(thread_id);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.counters["binary_sem_ops"] = binary_counter.load();
    state.counters["mutex_ops"] = mutex_counter.load();
    state.counters["binary_fairness"] = binary_tracker.coefficient_of_variation();
    state.counters["mutex_fairness"] = mutex_tracker.coefficient_of_variation();
    state.SetItemsProcessed(num_threads * operations_per_thread * 2); // Both semaphore and mutex
}

// Benchmark configurations
BENCHMARK(ProducerConsumer_SPSC_Semaphore)
    ->RangeMultiplier(10)
    ->Range(10, 1000)  // Queue capacity
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ProducerConsumer_MPMC_Semaphore)
    ->RangeMultiplier(2)
    ->Range(2, 8)  // Total threads (split between producers/consumers)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BinarySemaphore_vs_Mutex)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
