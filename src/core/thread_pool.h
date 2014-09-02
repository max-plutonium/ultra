#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "../task.h"
#include <mutex>
#include <queue>
#include <list>

namespace ultra { namespace core {

class thread_pool
{
    mutable std::mutex _mutex;
    using task_queue = std::priority_queue<
        task_ptr, std::vector<task_ptr>, task_prio_less>;
    std::priority_queue<task_ptr, std::vector<task_ptr>, task_prio_less> _async_tasks;
    static thread_local std::unique_ptr<task_queue> _deferred_tasks;

    std::atomic_size_t _nr_max_threads;
    std::atomic_size_t _nr_threads;
    std::atomic_size_t _nr_reserved;
    std::atomic_bool _exiting;
    int _expiry_timeout;

    mutable std::mutex _lock;
    std::list<std::thread> _threads;
    std::condition_variable _no_active_threads;

    void async_schedule(task_ptr &);
    std::size_t _active_thread_count() const;
    bool _too_many_active_threads() const;

public:
    thread_pool();
    ~thread_pool();
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;
    static thread_pool *instance();

    int get_expiry_timeout() const;
    void set_expiry_timeout(int timeout);

    int get_max_thread_count() const;
    void set_max_thread_count(int count);

    int get_thread_count() const;

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
