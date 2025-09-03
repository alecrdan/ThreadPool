#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

// Unbounded, thread-safe queue
// No capacity, always accepts push

template <typename T>
class SimpleQueue
{
public:
    SimpleQueue() : closed_(false) {}

    bool push(T v)
    {
        std::unique_lock<std::mutex> lk(mu_);
        if (closed_)
            return false;
        q_.push(std::move(v));
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lk(mu_);
        not_empty_.wait(lk, [&]
                        { return closed_ || !q_.empty(); });
        if (q_.empty())
            return std::nullopt;
        T v = std::move(q_.front());
        q_.pop();
        return v;
    }

    void close()
    {
        std::lock_guard<std::mutex> lk(mu_);
        closed_ = true;
        not_empty_.notify_all();
    }

private:
    mutable std::mutex mu_;
    std::condition_variable not_empty_;
    std::queue<T> q_;
    bool closed_;
};