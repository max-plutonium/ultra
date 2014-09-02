#include "thread_pool.h"
#include <algorithm>
#include <cassert>

namespace ultra { namespace core {

/************************************************************************************
    thread_pool
 ***********************************************************************************/
/*static*/ thread_local
std::unique_ptr<thread_pool::task_queue> thread_pool::_deferred_tasks = nullptr;

thread_pool::thread_pool()
    : _nr_max_threads(std::thread::hardware_concurrency())
    , _nr_threads(0)
    , _nr_reserved(0)
    , _exiting(false)
    , _expiry_timeout(30000)
{
}

thread_pool::~thread_pool()
{
    _exiting.store(true, std::memory_order_release);
    wait_for_done();
}

/*static*/
thread_pool *thread_pool::instance()
{
    static thread_pool s_pool;
    return &s_pool;
}

void thread_pool::async_schedule(task_ptr &ptask)
{
    _deferred_tasks.reset(new task_queue);
    std::size_t yield_count = 10;
    while(!_exiting.load(std::memory_order_relaxed)) {
        if(!ptask) {
            if(_deferred_tasks && !_deferred_tasks->empty()) {
                ptask = std::move(_deferred_tasks->top());
                _deferred_tasks->pop();
            }
        }

        if(!ptask) {
            std::lock_guard<std::mutex> locker { _mutex };
            if(!_async_tasks.empty()) {
                ptask = std::move(_async_tasks.top());
                _async_tasks.pop();
            }
        }

        if(ptask) {
            yield_count = 10;
            task_ptr t = std::move(ptask);
            try {
                t->run();
            } catch(...) {
                // TODO
            }
        } else {
            if(--yield_count)
                std::this_thread::yield();
            else
                break;
        }
    }

    --_nr_threads;
}

std::size_t thread_pool::_active_thread_count() const
{
    return _nr_threads;
}

bool thread_pool::_too_many_active_threads() const
{
    const std::size_t count = _active_thread_count();
    return count > _nr_max_threads && (count - _nr_reserved) > 1;
}

int thread_pool::get_expiry_timeout() const
{
    return _expiry_timeout;
}

void thread_pool::set_expiry_timeout(int timeout)
{
    _expiry_timeout = timeout;
}

int thread_pool::get_max_thread_count() const
{
    return _nr_max_threads;
}

void thread_pool::set_max_thread_count(int count)
{
    _nr_max_threads = count;
}

int thread_pool::get_thread_count() const
{
    return _active_thread_count();
}

void thread_pool::reserve_thread()
{
    ++_nr_reserved;
}

void thread_pool::release_thread()
{
    --_nr_reserved;
}

void thread_pool::wait_for_done()
{
    for(std::thread &t : _threads)
        t.join();
}

void thread_pool::schedule(std::launch policy, task_ptr ptask)
{
    const bool pooled_thread = static_cast<bool>(_deferred_tasks);
    if(pooled_thread && std::launch::deferred == policy) {
        _deferred_tasks->push(std::move(ptask));
    }
    else if(!pooled_thread && (_active_thread_count() < _nr_max_threads)) {
        ++_nr_threads;
        std::lock_guard<decltype(_lock)> locker { _lock };
        _threads.emplace_back(std::bind(&thread_pool::async_schedule, this, std::move(ptask)));
    }
    else //if(std::launch::async == (policy & std::launch::async | std::launch::deferred))
        _async_tasks.push(std::move(ptask));
}

} // namespace core

} // namespace ultra
