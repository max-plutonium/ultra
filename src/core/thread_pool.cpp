#include "thread_pool.h"
#include "concurrent_queue.h"
#include <thread>
#include <algorithm>
#include <cassert>

namespace ultra { namespace core {

/************************************************************************************
    thread_pool::pool_thread
 ***********************************************************************************/

class thread_pool::pool_thread : public std::enable_shared_from_this<pool_thread>
{
    thread_pool *_pool;
    std::thread _thread;

    mutable std::mutex _tasks_lock;
    std::condition_variable _new_task;
    concurrent_queue<task_ptr, std::mutex> _tasks;
    std::atomic_size_t _nr_tasks;

    enum state {
        stopped, running, waiting
    };

    std::atomic<state> _state;

    void run();
    void inactivate_thread();

public:
    explicit pool_thread(thread_pool *pool);
    ~pool_thread();
    void start();
    void add_task(task_ptr);
    std::size_t get_task_count() const;

    friend class thread_pool;
};

thread_pool::pool_thread::pool_thread(thread_pool *pool)
    : _pool(pool), _nr_tasks(0), _state(stopped)
{
}

thread_pool::pool_thread::~pool_thread()
{
    _thread.join();
}

void thread_pool::pool_thread::run()
{
    while(true)
    {
        _state.store(running, std::memory_order_release);
        task_ptr ptask;
        while(_tasks.pop(ptask))
        {
            try {
                ptask->run();
            } catch(...) {
                inactivate_thread();
                // TODO
            }
            _nr_tasks.fetch_sub(1, std::memory_order_release);
        }

        if(_pool->_exiting.load(std::memory_order_acquire)) {
            inactivate_thread();
            break;
        }

        bool expired = _pool->_too_many_active_threads();
        if(!expired) {
            std::unique_lock<std::mutex> locker { _pool->_mutex };
            _pool->_waiting_threads.push_back(this);
            inactivate_thread();
            _state.store(waiting, std::memory_order_relaxed);
            _new_task.wait_for(locker, std::chrono::milliseconds(_pool->_expiry_timeout));
            ++_pool->_active_threads;
            auto it = std::find(_pool->_waiting_threads.begin(),
                                _pool->_waiting_threads.end(), this);
            if(_pool->_waiting_threads.end() != it) {
                _pool->_waiting_threads.erase(it);
                expired = true;
            }
        }

        if(expired) {
            _pool->_expired_threads.push_back(this);
            inactivate_thread();
            break;
        }
    }

    _state.store(stopped, std::memory_order_relaxed);
}

void thread_pool::pool_thread::inactivate_thread()
{
    if(!--_pool->_active_threads)
        _pool->_no_active_threads.notify_all();
}

void thread_pool::pool_thread::start()
{
    _thread = std::move(std::thread(std::bind(&pool_thread::run, this)));
}

void thread_pool::pool_thread::add_task(task_ptr ptask)
{
    _nr_tasks.fetch_add(1, std::memory_order_release);
    _tasks.push(std::move(ptask));
    if(running != _state.load(std::memory_order_relaxed))
        _new_task.notify_one();
}

std::size_t thread_pool::pool_thread::get_task_count() const
{
    return _nr_tasks.load(std::memory_order_acquire);
}

/************************************************************************************
    thread_pool
 ***********************************************************************************/
thread_pool::thread_pool()
    : _exiting(false)
    , _expiry_timeout(30000)
    , _max_thread_count(std::thread::hardware_concurrency())
    , _reserved_threads(0)
    , _active_threads(0)

{
}

thread_pool::~thread_pool()
{
    if(wait_for_done())
        reset();
}

/*static*/
thread_pool *thread_pool::instance()
{
    static thread_pool s_pool;
    return &s_pool;
}

void thread_pool::_start_thread(task_ptr ptask)
{
    auto thread = std::make_shared<pool_thread>(this);
    thread->add_task(std::move(ptask));
    _threads.insert(thread);
    ++_active_threads;
    thread->start();
}

int thread_pool::_active_thread_count() const
{
    int count = _threads.size();
    count -= _waiting_threads.size();
    count -= _expired_threads.size();
    count += _reserved_threads;
    return count;
}

bool thread_pool::_too_many_active_threads() const
{
    const int count = _active_thread_count();
    return count > _max_thread_count && (count - _reserved_threads) > 1;
}

void thread_pool::run_async(task_ptr ptask)
{
    std::unique_lock<std::mutex> locker { _mutex };

    if(_threads.empty() || (_active_thread_count() < _max_thread_count))
        _start_thread(std::move(ptask));

    else if(!_waiting_threads.empty()) {
        pool_thread *thread = _waiting_threads.front();
        thread->add_task(std::move(ptask));
        _waiting_threads.pop_front();
    }

    else if(!_expired_threads.empty()) {
        pool_thread *thread = _expired_threads.front();
        _expired_threads.pop_front();
        ++_active_threads;
        thread->add_task(std::move(ptask));
        thread->start();
    }

    else {
        std::size_t max_tasks = UINT32_MAX;
        thread_ptr thread;
        for(thread_ptr t : _threads) {
            if(t->get_task_count() < max_tasks)
                thread = t;
        }
        thread->add_task(std::move(ptask));
    }
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
    return _max_thread_count;
}

void thread_pool::set_max_thread_count(int count)
{
    std::unique_lock<std::mutex> locker { _mutex };
    _max_thread_count = count;
}

int thread_pool::get_thread_count() const
{
    std::unique_lock<std::mutex> locker { _mutex };
    return _active_thread_count();
}

void thread_pool::reserve_thread()
{
    std::unique_lock<std::mutex> locker { _mutex };
    ++_reserved_threads;
}

void thread_pool::release_thread()
{
    std::unique_lock<std::mutex> locker { _mutex };
    --_reserved_threads;
}

bool thread_pool::wait_for_done(int msecs)
{
    std::unique_lock<std::mutex> locker { _mutex };
    return _no_active_threads.wait_for(locker, std::chrono::milliseconds(msecs),
        [this]() { return !_active_threads; });
}

void thread_pool::reset()
{
    std::unique_lock<std::mutex> locker { _mutex };
    _exiting = true;
    while(!_threads.empty()) {
        std::unordered_set<thread_ptr> threads_copy = std::move(_threads);
        locker.unlock();
        for(thread_ptr t : threads_copy) {
            t->_new_task.notify_all();
        }
        threads_copy.clear();
        locker.lock();
    }

    _waiting_threads.clear();
    _expired_threads.clear();
    _exiting = false;
}

} // namespace core

} // namespace ultra
