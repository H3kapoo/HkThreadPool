#include "Utility.hpp"

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace hk
{
class ThreadPool
{
#define HK_THREAD_POOL_MAX_POOLS 128

public:
    ThreadPool(const uint16_t threadCount)
    {
        /* Ensure a normal number of threads can be spawned. */
        if (threadCount < 1 || threadCount > HK_THREAD_POOL_MAX_POOLS)
        {
            printlne("Bad Pool init: %d", threadCount);
            stop = true;
            return;
        }

        println("Starting %d threads..", threadCount);

        /* Spawn threads */
        for (uint16_t i = 0; i < threadCount; i++)
        {
            threads.emplace_back(&ThreadPool::newThread, this);
        }
    }

private:
    void newThread()
    {
        /* Thread runs indefinitely */
        while (true)
        {
            /* Prepare task */
            std::function<void()> task;
            {
                /* Aquire lock for current thread. Blocking if mutex is already locked. */
                std::unique_lock<std::mutex> lock{mtx};

                /* If we managed to aquire the lock, check if this condition is true and proceed. Otherwise release lock
                   and put this thread to sleep indefinitely. */
                cond.wait(lock, [this]() { return stop || !workQueue.empty(); });

                /* Nothing to do if stop was requested or no more work needs to be done. Finish thread. */
                if (stop || workQueue.empty())
                {
                    return;
                }

                /* Move front element out of the queue, we don't need it there anyway and then pop it. */
                task = std::move(workQueue.front());
                workQueue.pop();
            }

            /* Run task */
            task();
        }
    }

public:
    ~ThreadPool()
    {
        println("Joining threads..");

        /* Aquire lock for pool to be able to safely say stop. */
        {
            std::unique_lock<std::mutex> lock{mtx};
            stop = true;
        }

        /* Notify all threads that stop was requested. */
        cond.notify_all();

        /* Join all threads and exit. */
        for (auto& th : threads)
        {
            th.join();
        }
        println("All threads joined");
    }

    template <class Fun, class... Args>
    auto enqueue(Fun&& func, Args&&... args) -> std::future<decltype(func(std::forward<Args>(args)...))>
    {
        /* Get the function and it's arguments in here and create a packaged_task from which we can get_future of the
         * func. We return that future and we can call get() on it to get the return value of the threaded function. */

        using RetType = decltype(func(std::forward<Args>(args)...));

        /* Bind all parameters to the function and return a mock function we can pass to packaged_task */
        std::function<RetType()> funWithBindings = std::bind(std::forward<Fun>(func), std::forward<Args>(args)...);
        auto pt = std::make_shared<std::packaged_task<RetType()>>(funWithBindings);

        /* Return the future from the packaged_task. We will use this to call get() on it. */
        std::future<RetType> res = pt->get_future();

        /* Safely lock the work queue to put the new pacakged_task inside. */
        {
            std::lock_guard<std::mutex> lock{mtx};
            workQueue.emplace([pt]() { (*pt)(); });
        }

        /* Notifiy a thread to wake up and process the work. */
        cond.notify_one();

        return res;
    };

private:
    bool stop{false};
    std::mutex mtx;
    std::condition_variable cond;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> workQueue;
};
} // namespace hk
