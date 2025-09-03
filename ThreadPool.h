#pragma once
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
#include <stdexcept>

class ThreadPool
{
public:
    explicit ThreadPool(std::size_t threads = 1) : threads_(threads) {}
    ~ThreadPool() { wait_for_tasks(); }

    // Submit exactly N tasks, one for each thread, returns futures in thread order
    template <class F>
    std::vector<std::future<typename std::invoke_result<F, std::size_t>::type>> submit_n(F &&f)
    {
        using R = typename std::invoke_result<F, std::size_t>::type;
        std::vector<std::future<R>> futures;
        tasks_.clear();
        for (std::size_t i = 0; i < threads_; ++i)
        {
            auto ptask = std::make_shared<std::packaged_task<R()>>(std::bind(f, i));
            futures.push_back(ptask->get_future());
            tasks_.push_back([ptask]
                             { (*ptask)(); });
        }
        in_flight_ = threads_;
        workers_.clear();
        for (std::size_t i = 0; i < threads_; ++i)
        {
            workers_.emplace_back([this, i]
                                  {
                tasks_[i]();
                std::lock_guard<std::mutex> lk(mu_);
                if (--in_flight_ == 0) cv_.notify_all(); });
        }
        return futures;
    }

    void wait_for_tasks()
    {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [this]
                 { return in_flight_ == 0; });
        for (auto &t : workers_)
            if (t.joinable())
                t.join();
    }

private:
    std::size_t threads_;
    std::vector<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::size_t in_flight_ = 0;
};