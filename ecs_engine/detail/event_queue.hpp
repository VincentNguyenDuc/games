#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class EventQueue {
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

public:
    void push(T event) {
        {
            std::lock_guard lock(mutex_);
            queue_.push(std::move(event));
        }
        cv_.notify_one();
    }

    // Blocks until an event is available.
    T pop_blocking() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        T event = std::move(queue_.front());
        queue_.pop();
        return event;
    }

    // Returns immediately — nullopt if the queue is empty.
    std::optional<T> try_pop() {
        std::lock_guard lock(mutex_);
        if (queue_.empty())
            return std::nullopt;
        T event = std::move(queue_.front());
        queue_.pop();
        return event;
    }

    bool empty() const {
        std::lock_guard lock(mutex_);
        return queue_.empty();
    }
};
