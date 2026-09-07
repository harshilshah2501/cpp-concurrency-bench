#include <benchmark/benchmark.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

#include "semaphore_queue.hpp"
#include "variance_tracker.hpp"

// =============================================================================
// Producer-Consumer with Semaphore Benchmark
// =============================================================================
// Uses a fresh SemaphoreQueue per Google Benchmark iteration so shutdown()
// cannot poison subsequent runs.
// =============================================================================

static void ProducerConsumer_SPSC_Semaphore(benchmark::State& state) {
    const size_t queue_capacity = static_cast<size_t>(state.range(0));
    const size_t items_per_iteration = 10000;

    for (auto _ : state) {
        SemaphoreQueue<int> queue(queue_capacity);
        std::atomic<size_t> items_consumed{0};

        std::thread producer([&] {
            for (size_t i = 0; i < items_per_iteration; ++i) {
                if (!queue.push(static_cast<int>(i))) {
                    break;
                }
            }
        });

        std::thread consumer([&] {
            int item = 0;
            while (items_consumed.load(std::memory_order_relaxed) <
                   items_per_iteration) {
                if (queue.pop(item)) {
                    items_consumed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    break;
                }
            }
        });

        producer.join();
        if (items_consumed.load(std::memory_order_relaxed) <
            items_per_iteration) {
            queue.shutdown();
        }
        consumer.join();
        if (!queue.is_shutdown()) {
            queue.shutdown();
        }
        benchmark::DoNotOptimize(items_consumed.load());
    }

    state.counters["queue_capacity"] = static_cast<double>(queue_capacity);
    state.counters["items_processed"] =
        static_cast<double>(items_per_iteration);
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations() * items_per_iteration));
}

static void ProducerConsumer_MPMC_Semaphore(benchmark::State& state) {
    const size_t num_threads = static_cast<size_t>(state.range(0));
    const size_t num_producers = num_threads / 2;
    const size_t num_consumers = num_threads - num_producers;
    const size_t items_per_producer = 1000;
    const size_t total_items = num_producers * items_per_producer;

    if (num_producers == 0 || num_consumers == 0) {
        state.SkipWithError("Need at least 1 producer and 1 consumer");
        return;
    }

    double last_producer_fairness = 0.0;
    double last_consumer_fairness = 0.0;

    for (auto _ : state) {
        SemaphoreQueue<int> queue(50);
        variance_tracker producer_tracker(num_producers);
        variance_tracker consumer_tracker(num_consumers);
        std::atomic<size_t> items_consumed{0};

        std::vector<std::thread> producers;
        producers.reserve(num_producers);
        for (size_t i = 0; i < num_producers; ++i) {
            producers.emplace_back([&, producer_id = i] {
                for (size_t j = 0; j < items_per_producer; ++j) {
                    int item =
                        static_cast<int>(producer_id * items_per_producer + j);
                    if (!queue.push(item)) {
                        break;
                    }
                    producer_tracker.record_operation(producer_id);
                }
            });
        }

        std::vector<std::thread> consumers;
        consumers.reserve(num_consumers);
        for (size_t i = 0; i < num_consumers; ++i) {
            consumers.emplace_back([&, consumer_id = i] {
                int item = 0;
                while (items_consumed.load(std::memory_order_relaxed) <
                       total_items) {
                    if (queue.pop(item)) {
                        items_consumed.fetch_add(1, std::memory_order_relaxed);
                        consumer_tracker.record_operation(consumer_id);
                    } else {
                        break;
                    }
                }
            });
        }

        for (auto& producer : producers) {
            producer.join();
        }
        if (items_consumed.load(std::memory_order_relaxed) < total_items) {
            queue.shutdown();
        }
        for (auto& consumer : consumers) {
            consumer.join();
        }
        if (!queue.is_shutdown()) {
            queue.shutdown();
        }

        last_producer_fairness = producer_tracker.coefficient_of_variation();
        last_consumer_fairness = consumer_tracker.coefficient_of_variation();
    }

    state.counters["num_producers"] = static_cast<double>(num_producers);
    state.counters["num_consumers"] = static_cast<double>(num_consumers);
    state.counters["producer_fairness"] = last_producer_fairness;
    state.counters["consumer_fairness"] = last_consumer_fairness;
    state.counters["total_items"] = static_cast<double>(total_items);
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations() * total_items));
}

static void BinarySemaphore_vs_Mutex(benchmark::State& state) {
    const int num_threads = static_cast<int>(state.range(0));
    const size_t operations_per_thread = 10000;

    uint64_t last_binary = 0;
    uint64_t last_mutex = 0;
    double last_binary_fairness = 0.0;
    double last_mutex_fairness = 0.0;

    for (auto _ : state) {
        std::binary_semaphore binary_sem{1};
        std::atomic<uint64_t> binary_counter{0};
        std::mutex mtx;
        std::atomic<uint64_t> mutex_counter{0};
        variance_tracker binary_tracker(static_cast<size_t>(num_threads));
        variance_tracker mutex_tracker(static_cast<size_t>(num_threads));

        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(num_threads));

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i] {
                for (size_t j = 0; j < operations_per_thread; ++j) {
                    binary_sem.acquire();
                    binary_counter.fetch_add(1, std::memory_order_relaxed);
                    binary_sem.release();
                    binary_tracker.record_operation(
                        static_cast<size_t>(thread_id));
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        threads.clear();

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i] {
                for (size_t j = 0; j < operations_per_thread; ++j) {
                    std::lock_guard<std::mutex> lock(mtx);
                    mutex_counter.fetch_add(1, std::memory_order_relaxed);
                    mutex_tracker.record_operation(
                        static_cast<size_t>(thread_id));
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }

        last_binary = binary_counter.load();
        last_mutex = mutex_counter.load();
        last_binary_fairness = binary_tracker.coefficient_of_variation();
        last_mutex_fairness = mutex_tracker.coefficient_of_variation();
    }

    state.counters["binary_sem_ops"] = static_cast<double>(last_binary);
    state.counters["mutex_ops"] = static_cast<double>(last_mutex);
    state.counters["binary_fairness"] = last_binary_fairness;
    state.counters["mutex_fairness"] = last_mutex_fairness;
    state.SetItemsProcessed(static_cast<int64_t>(
        state.iterations() * num_threads * operations_per_thread * 2));
}

BENCHMARK(ProducerConsumer_SPSC_Semaphore)
    ->RangeMultiplier(10)
    ->Range(10, 1000)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(ProducerConsumer_MPMC_Semaphore)
    ->RangeMultiplier(2)
    ->Range(2, 8)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BinarySemaphore_vs_Mutex)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
