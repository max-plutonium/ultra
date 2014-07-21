#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "../task.h"
#include <mutex>
#include <unordered_set>
#include <deque>

namespace ultra { namespace core {

class thread_pool
{
    class pool_thread;
    using thread_ptr = std::shared_ptr<pool_thread>;
    mutable std::mutex _mutex;
    std::unordered_set<thread_ptr> _threads;
    std::deque<pool_thread *> _waiting_threads;
    std::deque<pool_thread *> _expired_threads;
    std::condition_variable _no_active_threads;

    std::atomic_bool _exiting;
    int _expiry_timeout;
    int _max_thread_count;
    int _reserved_threads;
    int _active_threads;

    void _start_thread(task_ptr);
    int _active_thread_count() const;
    bool _too_many_active_threads() const;

public:
    thread_pool();
    ~thread_pool();
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;
    static thread_pool *instance();

    void run_async(task_ptr);

    int get_expiry_timeout() const;
    void set_expiry_timeout(int timeout);

    int get_max_thread_count() const;
    void set_max_thread_count(int count);

    int get_thread_count() const;

    void reserve_thread();
    void release_thread();

    bool wait_for_done(int msecs = INT_MAX);
    void reset();

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
