#include <iostream>
#include <thread>
#include "threadpool.h"
namespace test3
{

    std::mutex m_print;

    int hello(const int &x)
    {
        std::lock_guard<std::mutex> lc{m_print};
        std::cout << "hello thread     " << x
                  << " PID: " << std::this_thread::get_id() << std::endl;
        return x * x;
    }

}

int main()
{

    test3::ThreadPool tp{8};
    std::vector<std::future<int>> futures;
    std::atomic<int> sum = 0;
    for (int i = 0; i < 10; i++)
    {
        futures.emplace_back(tp.submit([&sum, i]
                                       {
            int res = test3::hello(i);
            sum.fetch_add(res,std::memory_order_relaxed);
            return res; }));
    }

    for (auto &f : futures)
    {
        f.get();
    }

    std::cout << "SUM:" << sum << std::endl;

    return 0;
}