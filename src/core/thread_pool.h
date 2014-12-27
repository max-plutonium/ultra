#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "schedulers.h"
#include <list>
#include <future>

namespace ultra { namespace core {


struct executor
{
    virtual void execute(task_ptr) = 0;
};

class thread_pool
{
    mutable std::mutex _lock;
    using task_queue = std::priority_queue<task_ptr,
        std::vector<task_ptr>, task_prio_less>;
    task_queue _async_tasks;

    std::size_t _nr_max_threads;
    std::size_t _nr_reserved;
    std::atomic_bool _shutdown;
    std::chrono::milliseconds _expiry_timeout;

    class worker;
    using worker_ptr = std::shared_ptr<worker>;
    std::list<worker_ptr> _active_threads;
    std::list<worker_ptr> _waiters;
    std::list<worker_ptr> _expired_threads;
    std::condition_variable _no_active_threads;

    std::size_t _active_thread_count() const;
    bool _too_many_active_threads() const;
    void _start_more();

public:
    thread_pool();
    ~thread_pool();
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;
    static thread_pool *instance();

    std::chrono::milliseconds expiry_timeout() const;
    void set_expiry_timeout(std::chrono::milliseconds timeout);

    std::size_t max_thread_count() const;
    void set_max_thread_count(std::size_t count);

    std::size_t thread_count() const;

    void reserve_thread();
    void release_thread();

    void wait_for_done();

    void schedule(std::launch, task_ptr);

    inline void schedule(task_ptr ptask)
    {
        schedule(std::launch::async | std::launch::deferred, std::move(ptask));
    }

  template <typename Callable, typename... Args>
    std::future<typename std::result_of<Callable(Args...)>::type>
    schedule(std::launch policy, Callable &&fun, Args &&...args)
    {
        using result_type = typename std::result_of<Callable(Args...)>::type;
        using task_type = function_task<Callable, Args...>;

        task_ptr ptask = std::make_shared<task_type>(
                std::forward<Callable>(fun), std::forward<Args>(args)...);
        std::future<result_type> ret
                = std::static_pointer_cast<task_type>(ptask)->get_future();

        schedule(policy, std::move(ptask));
        return ret;
    }

  template <typename Callable, typename... Args>
    inline std::future<typename std::result_of<Callable(Args...)>::type>
    schedule(Callable &&fun, Args &&...args)
    {
        return schedule(std::launch::async | std::launch::deferred,
            std::forward<Callable>(fun), std::forward<Args>(args)...);
    }
};

} // namespace core

} // namespace ultra

#endif // THREAD_POOL_H
