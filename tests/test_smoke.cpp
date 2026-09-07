#include "affinity.hpp"
#include "padded.hpp"
#include "semaphore_queue.hpp"
#include "spinlock.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

int failures = 0;

void expect(bool cond, const std::string& msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << "\n";
        ++failures;
    } else {
        std::cout << "ok: " << msg << "\n";
    }
}

void test_spinlock() {
    spinlock lock;
    expect(lock.try_lock(), "spinlock try_lock succeeds when free");
    expect(!lock.try_lock(), "spinlock try_lock fails when held");
    lock.unlock();
    expect(lock.try_lock(), "spinlock re-acquire after unlock");
    lock.unlock();

    constexpr int threads_n = 4;
    constexpr int iters = 10000;
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < threads_n; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < iters; ++j) {
                lock.lock();
                int v = counter.load(std::memory_order_relaxed);
                counter.store(v + 1, std::memory_order_relaxed);
                lock.unlock();
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    expect(counter.load() == threads_n * iters, "spinlock protects counter");
}

void test_semaphore_queue() {
    SemaphoreQueue<int> q(2);
    expect(q.push(1), "push first item");
    expect(q.push(2), "push second item");

    int item = 0;
    expect(q.pop(item) && item == 1, "pop FIFO first");
    expect(q.pop(item) && item == 2, "pop FIFO second");

    q.shutdown();
    expect(q.is_shutdown(), "queue reports shutdown");
    expect(!q.push(3), "push rejected after shutdown");
    expect(!q.pop(item), "pop rejected after shutdown");

    // Fresh queue each session — mirrors benchmark iteration lifecycle.
    SemaphoreQueue<int> q2(4);
    std::atomic<int> consumed{0};
    std::thread producer([&] {
        for (int i = 0; i < 100; ++i) {
            if (!q2.push(i)) {
                return;
            }
        }
        q2.close();  // producers done — consumers may drain then exit
    });
    std::thread consumer([&] {
        int v = 0;
        while (q2.pop(v)) {
            consumed.fetch_add(1);
        }
    });
    producer.join();
    consumer.join();
    expect(consumed.load() == 100, "SPSC session drains 100 items");
}

void test_padded_and_affinity() {
    padded<std::atomic<int>> counter{0};
    counter.fetch_add(1, std::memory_order_relaxed);
    expect(counter.load() == 1, "padded atomic fetch_add");

    expect(affinity::get_num_logical_cores() >= 1, "logical core count");
    // Calling the API must link; result is platform-dependent.
    (void)affinity::is_affinity_supported();
    expect(true, "is_affinity_supported links");
}

}  // namespace

int main() {
    test_spinlock();
    test_semaphore_queue();
    test_padded_and_affinity();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "all smoke tests passed\n";
    return EXIT_SUCCESS;
}
