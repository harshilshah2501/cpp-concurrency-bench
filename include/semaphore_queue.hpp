#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <queue>
#include <semaphore>

// =============================================================================
// Bounded producer-consumer queue coordinated with counting semaphores.
// =============================================================================
// Each instance is single-use across a full produce/consume session. Call
// reset() (or construct a fresh queue) between Google Benchmark iterations —
// shutdown() is terminal for a given session.
// =============================================================================

template <typename T>
class SemaphoreQueue {
private:
    mutable std::mutex mtx_;
    std::queue<T> queue_;
    std::counting_semaphore<> empty_slots_;
    std::counting_semaphore<> filled_slots_;
    size_t max_size_;
    std::atomic<bool> shutdown_{false};

public:
    explicit SemaphoreQueue(size_t max_size = 1000)
        : empty_slots_(static_cast<std::ptrdiff_t>(max_size)),
          filled_slots_(0),
          max_size_(max_size) {}

    // Not copyable/movable: semaphores and mutex ownership are unique.
    SemaphoreQueue(const SemaphoreQueue&) = delete;
    SemaphoreQueue& operator=(const SemaphoreQueue&) = delete;

    bool push(const T& item) {
        if (shutdown_.load(std::memory_order_acquire)) {
            return false;
        }

        empty_slots_.acquire();

        if (shutdown_.load(std::memory_order_acquire)) {
            empty_slots_.release();
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (shutdown_.load(std::memory_order_relaxed)) {
                empty_slots_.release();
                return false;
            }
            queue_.push(item);
        }

        filled_slots_.release();
        return true;
    }

    bool pop(T& item) {
        if (shutdown_.load(std::memory_order_acquire)) {
            return false;
        }

        filled_slots_.acquire();

        if (shutdown_.load(std::memory_order_acquire)) {
            // Wake a sibling waiter; do not treat this as a consumed item.
            filled_slots_.release();
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (queue_.empty()) {
                // Shutdown race: restore the filled-slot token we took.
                filled_slots_.release();
                return false;
            }
            item = queue_.front();
            queue_.pop();
        }

        empty_slots_.release();
        return true;
    }

    void shutdown() {
        shutdown_.store(true, std::memory_order_release);
        // Unblock waiters. Extra releases are harmless once shutdown_ is set
        // because push/pop re-check and return false.
        for (size_t i = 0; i < max_size_ + 1; ++i) {
            empty_slots_.release();
            filled_slots_.release();
        }
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    bool is_shutdown() const {
        return shutdown_.load(std::memory_order_acquire);
    }
};
