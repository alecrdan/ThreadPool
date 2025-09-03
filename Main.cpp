#include <iostream>
#include <future>
#include <vector>
#include "ThreadPool.h"

int main()
{
    ThreadPool pool(8); // 8 threads

    // Launch 8 tasks, each thread gets one task
    auto results = pool.submit_n([](std::size_t i)
                                 {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return static_cast<int>(i * i); });

    // Collect results
    for (auto &f : results)
    {
        std::cout << f.get() << " ";
    }
    std::cout << "\n";
}

// Run: g++ -std=c++20 -O3 -pthread main.cpp -o mega