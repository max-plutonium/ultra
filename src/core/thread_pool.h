#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "schedulers.h"
#include <unordered_set>
#include <list>
#include <future>

namespace ultra { namespace core {

struct executor
{
    virtual void execute(task_ptr) = 0;
};

class thread_pool_base : public executor
{
    std::shared_ptr<scheduler>  _sched;
    std::atomic_bool _shutdown;

    enum worker_status
    {
        ready, running, wait, stopped
    };

    struct worker;
    using worker_ptr = std::shared_ptr<worker>;
    using worker_list = std::list<worker_ptr>;
    using worker_id = worker_list::iterator;
    std::unordered_set<worker_ptr> _threads;
    worker_list _waiters;
    worker_list _expired;
    std::chrono::milliseconds _waiting_task_timeout;
    std::chrono::milliseconds _expiry_timeout;

    std::size_t _active_threads;
    std::size_t _nr_max_threads;
    std::size_t _nr_reserved;
    mutable std::mutex _lock;
    std::condition_variable _no_active_threads;

    struct worker
    {
        std::shared_ptr<scheduler> _sched;
        thread_pool_base *_pool;
        worker_status _status;
        std::condition_variable _cond;

        explicit worker(std::shared_ptr<scheduler> s, thread_pool_base *pool)
            : _sched(std::move(s)), _pool(pool), _status(thread_pool_base::ready) { }
        virtual ~worker() = default;
        virtual void start(task_ptr t) = 0;
        virtual void join() = 0;
        virtual void run() = 0;
    };

    void _start_thread(task_ptr);
    std::size_t _active_thread_count() const;
    bool _too_many_active_threads() const;
    bool _try_start(task_ptr);
    void _try_to_start_more();
    bool _wait_for_done(int msecs);
    void _reset();

    friend struct worker;
    friend struct thread_worker;

    // executor interface
public:
    virtual void execute(task_ptr) final;

    explicit thread_pool_base(std::shared_ptr<scheduler> s, std::size_t max_threads);
    ~thread_pool_base();
    thread_pool_base(const thread_pool_base&) = delete;
    thread_pool_base(thread_pool_base&&) = delete;
    thread_pool_base& operator=(const thread_pool_base&) = delete;
    thread_pool_base& operator=(thread_pool_base&&) = delete;

    std::shared_ptr<scheduler> sched() const { return _sched; }

    std::chrono::milliseconds expiry_timeout() const;
    void set_expiry_timeout(std::chrono::milliseconds timeout);

    std::size_t max_thread_count() const;
    void set_max_thread_count(std::size_t count);

    std::size_t thread_count() const;
    void reserve_thread();
    void release_thread();
    bool wait_for_done(int msecs = -1);
    void reset();
    void clear();
    void shutdown();
};

template <typename Scheduler>
class thread_pool : public thread_pool_base
{
public:
    thread_pool(std::size_t max_threads = std::thread::hardware_concurrency())
        : thread_pool_base(std::make_shared<Scheduler>(), max_threads) { }

  template <typename Callable, typename... Args>
      std::future<typename std::result_of<Callable (Args...)>::type>
    execute_callable(Callable &&fun, Args &&...args)
    {
        using result_type = typename std::result_of<Callable(Args...)>::type;
        using task_type = function_task<Callable, Args...>;

        task_ptr ptask = std::make_shared<task_type>(
                std::forward<Callable>(fun), std::forward<Args>(args)...);
        std::future<result_type> ret
                = std::static_pointer_cast<task_type>(ptask)->get_future();

        execute(std::move(ptask));
        return ret;
    }
};


} // namespace core

} // namespace ultra

#endif // THREAD_POOL_H
