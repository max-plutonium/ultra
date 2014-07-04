#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <mutex>
#include <queue>
#include <list>
#include <thread>
#include "../task.h"
#include "concurrent_queue.h"

namespace ultra { namespace core {

class thread_pool
{
    std::atomic_bool _done;
    concurrent_queue<task_ptr, std::mutex> _pending_tasks;
    static thread_local std::queue<task_ptr> _local_tasks;
    std::list<std::thread> _threads;

    void worker_thread()
    {
        while(!_done)
        {
            task_ptr ptask;
            if(_pending_tasks.dequeue(ptask))
                ptask->run();
            else
                std::this_thread::yield();
        }
    }

public:
    thread_pool() : _done(false)
    {
        std::size_t thread_count = 1;//std::thread::hardware_concurrency();
        try {
            while(thread_count--)
                _threads.emplace_back(&thread_pool::worker_thread, this);
        } catch(...) {
            _done = true;
            throw;
        }
    }

    ~thread_pool()
    {
        _done = true;
        for(std::thread &thread : _threads)
            thread.join();
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    void run_async(task_ptr ptask)
    {
        _pending_tasks.push(std::move(ptask));
    }

  template <typename Callable, typename... Args>
    std::future<typename std::result_of<Callable(Args...)>::type>
    run_async(Callable &&fun, Args &&...args)
    {
        using result_type = typename std::result_of<Callable(Args...)>::type;
        using task_type = function_task<Callable, Args...>;

        task_ptr ptask = std::make_shared<task_type>(
                std::forward<Callable>(fun), std::forward<Args>(args)...);
        std::future<result_type> ret
                = std::static_pointer_cast<task_type>(ptask)->get_future();

        run_async(std::move(ptask));
        return ret;
    }
};

} // namespace core

} // namespace ultra

#endif // THREAD_POOL_H
