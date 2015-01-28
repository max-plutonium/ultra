#include <thread>
#include <algorithm>
#include <memory>
#include <cassert>

#include "core_p.h"
#include "thread_pool.h"

namespace ultra { namespace core {

thread_pool::worker::worker(sched_ptr s, thread_pool *pool)
    : _sched(std::move(s)), _pool(pool)
{
}

class thread_worker : public thread_pool::worker
        , public std::enable_shared_from_this<thread_worker>
{
    task_ptr _task;
    std::shared_ptr<std::thread> _thread;

public:
    explicit thread_worker(std::shared_ptr<scheduler> s,
        thread_pool *pool) : worker(std::move(s), pool)
    { }

    virtual ~thread_worker() override {
        if(_thread && _thread->joinable())
            _thread->join();
    }

    virtual void start(task_ptr t) override
    {
        _task = std::move(t);
        if(_thread && _thread->joinable())
            _thread->join();
        _thread = std::make_shared<std::thread>(&worker::run, this);
    }

    void join() override {
        if(_thread && _thread->joinable())
            _thread->join();
        _thread.reset();
    }

    void inactivate() {
        if(--_pool->_active_threads == 0)
            _pool->_no_active_threads.notify_all();
    }

    void run() override
    {
        cds::gc::HP::thread_gc threadGC;

        bool expired = false;
        while(!expired)
        {
            do {
                task_ptr ptask = std::move(_task);

                if(!ptask)
                    ptask = _sched->schedule(_pool->_waiting_task_timeout);

                if(ptask) {
                    try {
                        ptask->run();

                    } catch(...) {
                        inactivate();
                        throw;
                    }
                }

            } while(!_sched->empty());

            std::unique_lock<std::mutex> lk { _pool->_lock };
            expired = _pool->_too_many_active_threads() || _pool->_shutdown;

            if(!expired)
            {
                _pool->_waiters.push_back(shared_from_this());
                inactivate();
                const std::cv_status waitres = _cond.wait_for(lk, _pool->_expiry_timeout);
                ++_pool->_active_threads;
                _pool->_waiters.remove(shared_from_this());
                if(std::cv_status::timeout == waitres
                   && !static_cast<bool>(_task))
                    expired = true;
                else
                    expired = !(static_cast<bool>(_task) || !_sched->empty());
            }

            if(expired) {
                _pool->_expired.push_back(shared_from_this());
                inactivate();
                break;
            }
        }
    }
};


/************************************************************************************
    thread_pool
 ***********************************************************************************/

void thread_pool::_start_thread(task_ptr ptask)
{
    std::shared_ptr<thread_worker> w = std::make_shared<thread_worker>(_sched, this);
    _threads.insert(w);
    ++_active_threads;
    w->start(std::move(ptask));
}

std::size_t thread_pool::_active_thread_count() const
{
    return _threads.size() - _waiters.size() - _expired.size() + _nr_reserved;
}

bool thread_pool::_too_many_active_threads() const
{
    const std::size_t count = _active_thread_count();
    return count > _nr_max_threads && (count - _nr_reserved) > 1;
}

bool thread_pool::_try_start(task_ptr ptask)
{
    if(_threads.empty()) {
        _start_thread(std::move(ptask));
        return true;
    }

    if(_active_thread_count() >= _nr_max_threads)
        return false;

    if(_waiters.size() > 0) {
        _sched->push(std::move(ptask));
        _waiters.front()->_cond.notify_one();
        _waiters.pop_front();
        return true;
    }

    if(!_expired.empty()) {
        worker_ptr w = _expired.front();
        _expired.pop_front();
        ++_active_threads;
        w->start(std::move(ptask));
        return true;
    }

    _start_thread(std::move(ptask));
    return true;
}

void thread_pool::_try_to_start_more()
{
    task_ptr ptask;
    while(!_sched->empty() && (ptask = _sched->schedule()))
        _try_start(std::move(ptask));
}

bool thread_pool::_wait_for_done(int msecs)
{
    std::unique_lock<std::mutex> lk(_lock);

    for(worker_ptr w : _waiters) {
        w->_cond.notify_one();
    }

    if(msecs < 0) {
        _no_active_threads.wait(lk, [this]() {
            return _sched->empty() && (_active_threads == 0);
        });

    } else {
        _no_active_threads.wait_for(lk,
            std::chrono::milliseconds(msecs), [this]() {
            return _sched->empty() && (_active_threads == 0);
        });
    }

    return _sched->empty() && _active_threads == 0;
}

void thread_pool::_reset()
{
    std::unique_lock<std::mutex> lk(_lock);
    _shutdown = true;
    _sched->stop();

    while(!_threads.empty()) {
        std::unordered_set<worker_ptr> victim;
        _threads.swap(victim);
        lk.unlock();

        for(worker_ptr w : victim) {
            w->_cond.notify_all();
            w->join();
        }

        lk.lock();
    }

    _waiters.clear();
    _expired.clear();
    _shutdown = false;
}

void thread_pool::execute(std::shared_ptr<task> ptask)
{
    std::lock_guard<std::mutex> lk(_lock);
    if(!_try_start(ptask)) {
        _sched->push(std::move(ptask));
        if(!_waiters.empty()) {
            _waiters.front()->_cond.notify_one();
            _waiters.pop_front();
        }
    }
}

void thread_pool::execute_with_delay(std::shared_ptr<task> ptask,
        std::size_t delay_msecs, std::size_t period_msecs)
{
    auto dtask = std::make_shared<timed_task>(std::move(ptask), this);
    dtask->start(delay_msecs, period_msecs);
}

/*!
 * \brief Конструирует пул с планировщиком \a st и с
 * максимальным числом потоков \a max_threads
 *
 * Если \c max_threads меньше либо равно нулю, количество потоков
 * выставляется равным количеству процессоров в системе.
 */
thread_pool::thread_pool(schedule_type st, std::size_t max_threads)
    : thread_pool(scheduler::make(st), max_threads)
{
}

/*!
 * \brief Конструирует пул с планировщиком \a s и с
 * максимальным числом потоков \a max_threads
 *
 * Если \c max_threads меньше либо равно нулю, количество потоков
 * выставляется равным количеству процессоров в системе.
 */
thread_pool::thread_pool(sched_ptr s, std::size_t max_threads)
    : _sched(std::move(s)), _shutdown(false)
    , _waiting_task_timeout(1000), _expiry_timeout(30000)
    , _nr_max_threads(max_threads <= 0 ?
        std::thread::hardware_concurrency() : max_threads)
    , _active_threads(0), _nr_reserved(0)
{
}

/*!
 * \brief Ждет окончания работ и уничтожает пул
 */
thread_pool::~thread_pool()
{
    wait_for_done();
}

/*!
 * \brief Возвращает таймаут истечения потоков
 */
std::chrono::milliseconds thread_pool::expiry_timeout() const
{
    std::lock_guard<std::mutex> lk(_lock);
    return _expiry_timeout;
}

/*!
 * \brief Устанавливает из \a timeout таймаут истечения потоков
 */
void thread_pool::set_expiry_timeout(std::chrono::milliseconds timeout)
{
    std::lock_guard<std::mutex> lk(_lock);
    _expiry_timeout = timeout;
}

/*!
 * \brief Возвращает максимальное число потоков
 */
std::size_t thread_pool::max_thread_count() const
{
    std::lock_guard<std::mutex> lk(_lock);
    return _nr_max_threads;
}

/*!
 * \brief Устанавливает максимальное число потоков
 */
void thread_pool::set_max_thread_count(std::size_t count)
{
    std::lock_guard<std::mutex> lk(_lock);
    _nr_max_threads = count;
    _try_to_start_more();
}

/*!
 * \brief Возвращает число активных в данный момент потоков
 */
std::size_t thread_pool::thread_count() const
{
    std::lock_guard<std::mutex> lk(_lock);
    return _active_thread_count();
}

/*!
 * \brief Резервирует поток
 */
void thread_pool::reserve_thread()
{
    std::lock_guard<std::mutex> lk(_lock);
    ++_nr_reserved;
}

/*!
 * \brief Освобождает поток
 */
void thread_pool::release_thread()
{
    std::lock_guard<std::mutex> lk(_lock);
    --_nr_reserved;
    _try_to_start_more();
}

/*!
 * \brief Ждет завершения всех потоков \a msecs миллисекунд
 *
 * Если \a msecs меньше нуля, то ждет неограниченное время
 *
 * \return true, если потоки были завершены при ожидании
 */
bool thread_pool::wait_for_done(int msecs)
{
    const bool res = _wait_for_done(msecs);
    if(res)
        _reset();
    return res;
}

/*!
 * \brief Очищает очередь задач планировщика
 */
void thread_pool::clear()
{
    _sched->clear();
}

void thread_pool::shutdown()
{
    _shutdown = true;
    _sched->stop();
}

bool thread_pool::stopped() const
{
    return _shutdown;
}

bool thread_pool::try_executing_one()
{
    task_ptr ptask = _sched->schedule(_waiting_task_timeout);

    if(ptask)
        ptask->run();

    return static_cast<bool>(ptask);
}

} // namespace core

} // namespace ultra
