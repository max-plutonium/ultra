#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <list>
#include <unordered_set>

#include "../ultra.h"

namespace ultra { namespace core {

/*!
 * \brief Класс пула потоков
 *
 * Пул потоков управляет выполнением задач потоками и позволяет
 * переиспользовать потоки, чтобы сократить затраты на их создание.
 *
 * \snippet core/thread_pool.cpp pool
 *
 * Потоки, которые не используются определенное время "истекают".
 * По-умолчанию, время истечения равно 30 секундам, но может быть
 * изменено вызовом \c set_expiry_timeout().
 *
 * Так же можно настраивать максимальное число потоков и резервировать
 * их для внешних нужд.
 */
class thread_pool : public execution_service
{
    sched_ptr  _sched;
    std::atomic_bool _shutdown;

    struct worker
    {
        sched_ptr _sched;
        thread_pool *_pool;
        std::condition_variable _cond;

        explicit worker(sched_ptr s, thread_pool *pool);
        virtual ~worker() = default;
        virtual void start(task_ptr t) = 0;
        virtual void join() = 0;
        virtual void run() = 0;
    };

    using worker_ptr = std::shared_ptr<worker>;
    using worker_list = std::list<worker_ptr>;
    using worker_id = worker_list::iterator;
    std::unordered_set<worker_ptr> _threads;
    worker_list _waiters;
    worker_list _expired;
    std::chrono::milliseconds _waiting_task_timeout;
    std::chrono::milliseconds _expiry_timeout;

    std::size_t _nr_max_threads;
    std::size_t _active_threads;
    std::size_t _nr_reserved;
    mutable std::mutex _lock;
    std::condition_variable _no_active_threads;

    void _start_thread(task_ptr);
    std::size_t _active_thread_count() const;
    bool _too_many_active_threads() const;
    bool _try_start(task_ptr);
    void _try_to_start_more();
    bool _wait_for_done(int msecs);
    void _reset();

public:
    explicit thread_pool(schedule_type st, std::size_t max_threads = -1);
    explicit thread_pool(sched_ptr s, std::size_t max_threads = -1);
    ~thread_pool();

#ifndef DOXYGEN
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;
#endif

    std::chrono::milliseconds expiry_timeout() const;
    void set_expiry_timeout(std::chrono::milliseconds timeout);

    std::size_t max_thread_count() const;
    void set_max_thread_count(std::size_t count);

    std::size_t thread_count() const;
    void reserve_thread();
    void release_thread();
    bool wait_for_done(int msecs = -1);
    void clear();

    virtual void execute(std::shared_ptr<task>) final override;
    virtual void execute_with_delay(std::shared_ptr<task>,
        std::size_t delay_msecs = 0, std::size_t period_msecs = 0) final override;

    virtual void shutdown() override;
    virtual bool stopped() const override;
    virtual bool try_executing_one() override;

    friend struct thread_worker;

    /*!
     * \brief Конструирует задачу из объекта действия \a fun
     * с приоритетом \a prio и передает её на исполнение
     *
     * \return Объект будущего результата задачи.
     */
    template <typename Res, typename... Args>
      std::future<Res> execute_callable(int prio, action<Res (Args...)> &&fun)
      {
          using task_type = function_task<Res (Args...)>;
          auto ptask = std::make_shared<task_type>(prio, std::move(fun));
          std::future<Res> ret = ptask->get_future();

          execute(std::move(ptask));
          return ret;
      }

    /*!
     * \brief Конструирует задачу из функции \a fun с
     * аргументами \a args и с приоритетом \a prio и передает
     * её на исполнение
     *
     * \return Объект будущего результата задачи.
     */
    template <typename Function, typename... Args>
      auto execute_callable(int prio, Function &&fun, Args &&...args)
      {
          return execute_callable(prio, core::make_action(
            std::forward<Function>(fun), std::forward<Args>(args)...));
      }
};

} // namespace core

} // namespace ultra

#endif // THREAD_POOL_H
