#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable work_cv_;
    std::condition_variable done_cv_;
    std::atomic<int> pending_{0};
    bool stop_{false};

public:
    explicit ThreadPool(std::size_t n = std::thread::hardware_concurrency()) {
        workers_.reserve(n);
        for (std::size_t i = 0; i < n; ++i)
            workers_.emplace_back([this] { loop(); });
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            stop_ = true;
        }
        work_cv_.notify_all();
        for (auto& t : workers_)
            t.join();
    }

    void submit(std::function<void()> f) {
        ++pending_;
        {
            std::unique_lock<std::mutex> lk(mutex_);
            tasks_.push(std::move(f));
        }
        work_cv_.notify_one();
    }

    // Blocks until all submitted tasks have finished.
    void wait() {
        std::unique_lock<std::mutex> lk(mutex_);
        done_cv_.wait(lk, [this] { return pending_.load() == 0; });
    }

private:
    void loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(mutex_);
                work_cv_.wait(lk, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty())
                    return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
            if (--pending_ == 0)
                done_cv_.notify_all();
        }
    }
};
