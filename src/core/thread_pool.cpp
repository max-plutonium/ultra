#include "thread_pool.h"
#include <thread>
#include <algorithm>
#include <cassert>

namespace ultra { namespace core {

/************************************************************************************
    thread_pool
 ***********************************************************************************/

class thread_pool::worker : public std::enable_shared_from_this<worker>
{
    thread_pool *_pool;
    task_ptr _task;
    std::thread _thread;
    std::list<thread_pool::worker_ptr>::iterator _id;
    std::condition_variable _cond;

    bool _wait_pred() {

    }

    void _expire_thread() {
        _status = expired;
        _pool->_active_threads.erase(_id);
        _pool->_expired_threads.push_back(shared_from_this());
        _id = --_pool->_expired_threads.end();
        _cond.notify_all();
    }

public:
    enum status {
        stop, expired, running, wait
    } _status;

    explicit worker(thread_pool *pool)
        : _pool(pool), _status(stop) { }

    ~worker() {
        if(_thread.joinable())
            _thread.join();
    }

    void join() { _thread.join(); }
    void wake_up() { _cond.notify_one(); }
    void wait_for_done(std::unique_lock<std::mutex> &lk) { _cond.wait(lk); }

    void start(task_ptr ptask, std::list<thread_pool::worker_ptr>::iterator id) {
        _task = std::move(ptask);
        _id = id;
        _thread = std::thread(std::bind(&worker::run, shared_from_this()));
    }

    void run() {
        while(true) {
            while(_task || _pool->_async_tasks.pop(_task)) {
                _status = running;
                task_ptr t = std::move(_task);
                try {
                    t->run();
                } catch(...) {
                    // TODO
                    break;
                }
            }

            std::unique_lock<std::mutex> lk { _pool->_lock };
            if(_pool->_exiting.load(std::memory_order_acquire)
               || _pool->_too_many_active_threads())
                break;

            if(!_cond.wait_for(lk, _pool->_expiry_timeout,
                    [this]() {
                        if(_pool->_async_tasks.pop(_task)) {
                            if(running != _status) {
                                _pool->_waiters.erase(this);
                                _status = running;
                            }

                            return true;
                        }

                        if(wait != _status) {
                            _pool->_waiters.insert(this);
                            _status = wait;
                        }

                        return false;
                    })) {
                _expire_thread();
                return;
            }
        }

        std::unique_lock<std::mutex> lk { _pool->_lock };
        _expire_thread();
    }
};

thread_pool::thread_pool()
    : _nr_max_threads(std::thread::hardware_concurrency())
    , _nr_reserved(0)
    , _exiting(false)
    , _expiry_timeout(30000)
{
}

thread_pool::~thread_pool()
{
    wait_for_done();
}

/*static*/
thread_pool *thread_pool::instance()
{
    static thread_pool s_pool;
    return &s_pool;
}

std::size_t thread_pool::_active_thread_count() const
{
    return _active_threads.size() + _nr_reserved;
}

bool thread_pool::_too_many_active_threads() const
{
    const std::size_t count = _active_thread_count();
    return count > _nr_max_threads && (count - _nr_reserved) > 1;
}

void thread_pool::_start_more()
{
    while(_active_thread_count() < _nr_max_threads) {
        task_ptr ptask;
        if(!_async_tasks.pop(ptask))
            break;
        _active_threads.push_back(std::make_shared<worker>(this));
        _active_threads.back()->start(std::move(ptask), --_active_threads.end());
    }
}

std::chrono::milliseconds thread_pool::get_expiry_timeout() const
{
    std::lock_guard<std::mutex> lk { _lock };
    return _expiry_timeout;
}

void thread_pool::set_expiry_timeout(std::chrono::milliseconds timeout)
{
    std::lock_guard<std::mutex> lk { _lock };
    _expiry_timeout = timeout;
}

std::size_t thread_pool::get_max_thread_count() const
{
    std::lock_guard<std::mutex> lk { _lock };
    return _nr_max_threads;
}

void thread_pool::set_max_thread_count(std::size_t count)
{
    std::lock_guard<std::mutex> lk { _lock };
    _nr_max_threads = count;
    _start_more();
}

std::size_t thread_pool::get_thread_count() const
{
    std::lock_guard<std::mutex> lk { _lock };
    return _active_thread_count();
}

void thread_pool::reserve_thread()
{
    std::lock_guard<std::mutex> lk { _lock };
    ++_nr_reserved;
}

void thread_pool::release_thread()
{
    std::lock_guard<std::mutex> lk { _lock };
    --_nr_reserved;
    _start_more();
}

void thread_pool::wait_for_done()
{
    _exiting.store(true, std::memory_order_release);
    std::unique_lock<std::mutex> lk { _lock };
    while(_active_threads.size()) {
        worker_ptr t = _active_threads.front();
        while(worker::expired < t->_status) {
            if(worker::wait == t->_status)
                t->wake_up();
            t->wait_for_done(lk);
        }
        lk.unlock();
        t->join();
        assert(2 == t.use_count());
        lk.lock();
    }

    _waiters.clear();
    _expired_threads.clear();

    _exiting.store(false, std::memory_order_release);
}

void thread_pool::schedule(std::launch policy, task_ptr ptask)
{
    (void) policy;
    {
        std::lock_guard<std::mutex> lk { _lock };
        if(!_waiters.empty()) {
            (*_waiters.begin())->wake_up();
            _async_tasks.push(std::move(ptask));
            return;
        }
        else if(!_expired_threads.empty()) {
            _active_threads.push_back(*_expired_threads.begin());
            _expired_threads.pop_front();
            _active_threads.back()->start(std::move(ptask), --_active_threads.end());
        }
        else if(_active_thread_count() < _nr_max_threads) {
            _active_threads.push_back(std::make_shared<worker>(this));
            _active_threads.back()->start(std::move(ptask), --_active_threads.end());
            return;
        }
    }

    //if(std::launch::async == (policy & std::launch::async | std::launch::deferred))
    _async_tasks.push(std::move(ptask));
}

} // namespace core

} // namespace ultra
