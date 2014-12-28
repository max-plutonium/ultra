#include "thread_pool.h"
#include <thread>
#include <algorithm>
#include <memory>
#include <cassert>

namespace ultra { namespace core {

class thread_worker : public thread_pool_base::worker
        , public std::enable_shared_from_this<thread_worker>
{
    task_ptr _task;
    std::shared_ptr<std::thread> _thread;

public:
    explicit thread_worker(std::shared_ptr<scheduler> s,
        thread_pool_base *pool) : worker(std::move(s), pool) { }

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
        bool expired = false;
        while(!expired)
        {
            _status = thread_pool_base::running;
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
                _status = thread_pool_base::wait;
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
    thread_pool_base
 ***********************************************************************************/

void thread_pool_base::_start_thread(task_ptr ptask)
{
    std::shared_ptr<thread_worker> w = std::make_shared<thread_worker>(_sched, this);
    _threads.insert(w);
    ++_active_threads;
    w->start(std::move(ptask));
}

std::size_t thread_pool_base::_active_thread_count() const
{
    return _threads.size() - _waiters.size() - _expired.size() + _nr_reserved;
}

bool thread_pool_base::_too_many_active_threads() const
{
    const std::size_t count = _active_thread_count();
    return count > _nr_max_threads && (count - _nr_reserved) > 1;
}

bool thread_pool_base::_try_start(task_ptr ptask)
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

void thread_pool_base::_try_to_start_more()
{
    task_ptr ptask;
    while(!_sched->empty() && (ptask = _sched->schedule()))
        _try_start(std::move(ptask));
}

bool thread_pool_base::_wait_for_done(int msecs)
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

void thread_pool_base::_reset()
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

void thread_pool_base::execute(task_ptr ptask)
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

thread_pool_base::thread_pool_base(std::shared_ptr<scheduler> s, std::size_t max_threads)
    : _sched(std::move(s)), _shutdown(false)
    , _waiting_task_timeout(10), _expiry_timeout(10000)
    , _active_threads(0), _nr_max_threads(max_threads), _nr_reserved(0)
{
}

thread_pool_base::~thread_pool_base()
{
    wait_for_done();
}

std::chrono::milliseconds thread_pool_base::expiry_timeout() const
{
    std::lock_guard<std::mutex> lk(_lock);
    return _expiry_timeout;
}

void thread_pool_base::set_expiry_timeout(std::chrono::milliseconds timeout)
{
    std::lock_guard<std::mutex> lk(_lock);
    _expiry_timeout = timeout;
}

std::size_t thread_pool_base::max_thread_count() const
{
    std::lock_guard<std::mutex> lk(_lock);
    return _nr_max_threads;
}

void thread_pool_base::set_max_thread_count(std::size_t count)
{
    std::lock_guard<std::mutex> lk(_lock);
    _nr_max_threads = count;
    _try_to_start_more();
}

std::size_t thread_pool_base::thread_count() const
{
    std::lock_guard<std::mutex> lk(_lock);
    return _active_thread_count();
}

void thread_pool_base::reserve_thread()
{
    std::lock_guard<std::mutex> lk(_lock);
    ++_nr_reserved;
}

void thread_pool_base::release_thread()
{
    std::lock_guard<std::mutex> lk(_lock);
    --_nr_reserved;
    _try_to_start_more();
}

bool thread_pool_base::wait_for_done(int msecs)
{
    const bool res = _wait_for_done(msecs);
    if(res)
        _reset();
    return res;
}

void thread_pool_base::clear()
{
    _sched->clear();
}

void thread_pool_base::shutdown()
{
    _shutdown = true;
    _sched->stop();
}

} // namespace core

} // namespace ultra
