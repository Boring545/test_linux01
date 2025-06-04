#ifndef THREAD_POOL
#define THREAD_POOL

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <future>
#include <type_traits>
#include <functional>
namespace test3
{
    constexpr size_t NUM_MINIMUM_DEFAULT_THREAD = 2;
    size_t get_size_thread_pool()
    {
        auto num = std::thread::hardware_concurrency();
        if (num == 0)
        {
            num = NUM_MINIMUM_DEFAULT_THREAD;
        }
        return num;
    }
    class ThreadPool
    {
    private:
        using Task = std::packaged_task<void()>;

        std::mutex mutex_tp; // tasks队列锁
        std::condition_variable cv_tp;
        std::atomic<bool> active_flag;
        std::atomic<size_t> num_thread;
        std::queue<Task> tasks;
        std::vector<std::thread> threads;

    public:
        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;

        ThreadPool(size_t pool_size = get_size_thread_pool())
            : active_flag(true), num_thread(pool_size)
        {
            start();
        }
        ~ThreadPool()
        {
            stop();
        }
        // 启动线程池
        void start()
        {
            for (size_t i = 0; i < num_thread; i++)
            {
                threads.emplace_back([this]
                                     {
                    while(active_flag){
                        Task task;
                        {
                            std::unique_lock<std::mutex>lc{mutex_tp};
                            // 有任务或收到关闭信号都唤醒
                            cv_tp.wait(lc,[this]{return !tasks.empty()||active_flag.load()==false;});
                            
                            if(active_flag.load()==false&&tasks.empty()){
                                // 线程收到关闭信号后，如果没有任务可执行就退出
                                return;
                            }
                            if(tasks.empty()){
                                continue;
                            }


                            task=std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    } });
            }
        }
        // 关闭线程池
        void stop()
        {
            active_flag.store(false);
            cv_tp.notify_all();
            for (auto &t : threads)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }
            threads.clear();
        }

        // 提交任务,返回future
        template <typename F, typename... Args>
        std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> submit(F &&function, Args &&...args)
        {
            using FuncType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

            if (active_flag.load() == false)
            {
                // 线程池停止时立刻抛出异常，提示当前任务无法提交
                throw std::runtime_error("ThreadPoll has stopped, Task can not been submit");
            }
            auto task = std::make_shared<std::packaged_task<FuncType()>>(
                std::bind(std::forward<F>(function), std::forward<Args>(args)...));


            auto res = task->get_future();
            {
                std::lock_guard<std::mutex> lc{mutex_tp};
                tasks.emplace([task]
                              { (*task)(); });
            }
            cv_tp.notify_one();
            return res;
        }
    };


}

#endif //THREAD_POOL