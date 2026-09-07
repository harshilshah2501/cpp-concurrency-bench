#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <queue>
#include <semaphore>

// =============================================================================
// Bounded producer-consumer queue coordinated with counting semaphores.
// =============================================================================
// Lifecycle:
//   1. push/pop while producers are active
//   2. close() after producers finish — wakes consumers; remaining items drain
//   3. shutdown() is terminal (rejects push and further pops once empty)
// Construct a fresh queue per Google Benchmark iteration.
// =============================================================================

template <typename T>
class SemaphoreQueue {
private:
    mutable std::mutex mtx_;
    std::queue<T> queue_;
    std::counting_semaphore<> empty_slots_;
    std::counting_semaphore<> filled_slots_;
    size_t max_size_;
    std::atomic<bool> closed_{false};
    std::atomic<bool> shutdown_{false};

    void wake_consumers() {
        for (size_t i = 0; i < max_size_ + 1; ++i) {
            filled_slots_.release();
        }
    }

    void wake_producers() {
        for (size_t i = 0; i < max_size_ + 1; ++i) {
            empty_slots_.release();
        }
    }

public:
    explicit SemaphoreQueue(size_t max_size = 1000)
        : empty_slots_(static_cast<std::ptrdiff_t>(max_size)),
          filled_slots_(0),
          max_size_(max_size) {}

    SemaphoreQueue(const SemaphoreQueue&) = delete;
    SemaphoreQueue& operator=(const SemaphoreQueue&) = delete;

    bool push(const T& item) {
        if (shutdown_.load(std::memory_order_acquire) ||
            closed_.load(std::memory_order_acquire)) {
            return false;
        }

        empty_slots_.acquire();

        if (shutdown_.load(std::memory_order_acquire) ||
            closed_.load(std::memory_order_acquire)) {
            empty_slots_.release();
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (shutdown_.load(std::memory_order_relaxed) ||
                closed_.load(std::memory_order_relaxed)) {
                empty_slots_.release();
                return false;
            }
            queue_.push(item);
        }

        filled_slots_.release();
        return true;
    }

    bool pop(T& item) {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(mtx_);
                if (queue_.empty() &&
                    (shutdown_.load(std::memory_order_relaxed) ||
                     closed_.load(std::memory_order_relaxed))) {
                    return false;
                }
            }

            filled_slots_.acquire();

            {
                std::lock_guard<std::mutex> lock(mtx_);
                if (!queue_.empty()) {
                    item = queue_.front();
                    queue_.pop();
                    empty_slots_.release();
                    return true;
                }
            }

            // Woken with an empty queue: stop only once producers are done.
            if (shutdown_.load(std::memory_order_acquire) ||
                closed_.load(std::memory_order_acquire)) {
                filled_slots_.release();
                return false;
            }
        }
    }

    // Producers finished: allow consumers to drain, then return false when empty.
    void close() {
        closed_.store(true, std::memory_order_release);
        wake_consumers();
    }

    void shutdown() {
        shutdown_.store(true, std::memory_order_release);
        closed_.store(true, std::memory_order_release);
        wake_consumers();
        wake_producers();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    bool is_shutdown() const {
        return shutdown_.load(std::memory_order_acquire);
    }

    bool is_closed() const {
        return closed_.load(std::memory_order_acquire);
    }
};
