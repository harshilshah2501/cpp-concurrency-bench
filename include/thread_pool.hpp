#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
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
// - Graceful shutdown (stop flag guarded by the queue mutex)
// =============================================================================

class thread_pool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_ = false;  // guarded by queue_mutex_

public:
    explicit thread_pool(
        size_t num_threads = std::thread::hardware_concurrency()) {
        if (num_threads == 0) {
            num_threads = 1;
        }
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty()) {
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

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    ~thread_pool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
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
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("submit on stopped thread_pool");
            }
            tasks_.emplace([task] { (*task)(); });
        }
        condition_.notify_one();
        return future;
    }

    size_t size() const { return workers_.size(); }
};
