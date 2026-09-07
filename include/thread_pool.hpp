#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

// =============================================================================
// Simple Thread Pool Implementation
// =============================================================================
// A basic thread pool for benchmarking purposes. Not intended for production
// use — focuses on simplicity and measurable performance characteristics.
//
// Features:
// - Fixed number of worker threads
// - Shared mutex-protected FIFO task queue (not work-stealing)
// - std::future-based task submission
// - Graceful shutdown
// =============================================================================

class thread_pool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};

public:
    explicit thread_pool(
        size_t num_threads = std::thread::hardware_concurrency()) {
        if (num_threads == 0) {
            num_threads = 1;
        }
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_.load() || !tasks_.empty();
                        });
                        if (stop_.load() && tasks_.empty()) {
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~thread_pool() {
        stop_.store(true);
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        auto future = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_.load()) {
                throw std::runtime_error("submit on stopped thread_pool");
            }
            tasks_.emplace([task] { (*task)(); });
        }
        condition_.notify_one();
        return future;
    }

    size_t size() const { return workers_.size(); }
};
