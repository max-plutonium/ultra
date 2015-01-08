#include "schedulers.h"
#include <cassert>
#include "system.h"
#include "core_p.h"

namespace ultra {

void scheduler::push_timed(std::shared_ptr<task> t,
                           std::size_t delay_msecs, std::size_t period_msecs)
{
    auto dtask = std::make_shared<timed_task>(std::move(t),
            vm::impl::get()->next_io_service(), shared_from_this());
    dtask->start(delay_msecs, period_msecs);
}

/*static*/ std::shared_ptr<scheduler> scheduler::make(schedule_type type)
{
    std::shared_ptr<scheduler> ret;

    switch(type) {

        case schedule_type::fifo:
            ret = std::make_shared<core::fifo_scheduler>();
            break;

        case schedule_type::lifo:
            ret = std::make_shared<core::lifo_scheduler>();
            break;

        case schedule_type::prio:
            ret = std::make_shared<core::prio_scheduler>();
            break;
    }

    return ret;
}

namespace core {

/************************************************************************************
    fifo_scheduler
 ***********************************************************************************/

void fifo_scheduler::push(task_ptr t)
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    _tasks.push_back(std::move(t));
    if(_nr_contenders)
        _cond.notify_one();
}

task_ptr fifo_scheduler::schedule(std::chrono::milliseconds msecs)
{
    task_ptr ret;

    std::unique_lock<decltype(_lock)> lk(_lock);
    ++_nr_contenders;
    _cond.wait_for(lk, msecs, [this]()
        { return !_tasks.empty() || !stopped; });
    --_nr_contenders;

    if(!_tasks.empty()) {
        ret = _tasks.front();
        _tasks.pop_front();
    }

    return ret;
}

std::size_t fifo_scheduler::size() const
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    return _tasks.size();
}

bool fifo_scheduler::empty() const
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    return _tasks.empty();
}

void fifo_scheduler::clear()
{
    decltype(_tasks) victim;
    std::unique_lock<decltype(_lock)> lk(_lock);
    _tasks.swap(victim);
}


/************************************************************************************
    lifo_scheduler
 ***********************************************************************************/

void lifo_scheduler::push(task_ptr t)
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    _tasks.push_front(std::move(t));
    if(_nr_contenders)
        _cond.notify_one();
}

task_ptr lifo_scheduler::schedule(std::chrono::milliseconds msecs)
{
    task_ptr ret;

    std::unique_lock<decltype(_lock)> lk(_lock);
    ++_nr_contenders;
    _cond.wait_for(lk, msecs, [this]()
        { return !_tasks.empty() || !stopped; });
    --_nr_contenders;

    if(!_tasks.empty()) {
        ret = _tasks.front();
        _tasks.pop_front();
    }

    return ret;
}

std::size_t lifo_scheduler::size() const
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    return _tasks.size();
}

bool lifo_scheduler::empty() const
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    return _tasks.empty();
}

void lifo_scheduler::clear()
{
    decltype(_tasks) victim;
    std::unique_lock<decltype(_lock)> lk(_lock);
    _tasks.swap(victim);
}


/************************************************************************************
    prio_scheduler
 ***********************************************************************************/

void prio_scheduler::push(task_ptr t)
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    _tasks.push(std::move(t));
    if(_nr_contenders)
        _cond.notify_one();
}

task_ptr prio_scheduler::schedule(std::chrono::milliseconds msecs)
{
    task_ptr ret;

    std::unique_lock<decltype(_lock)> lk(_lock);
    ++_nr_contenders;
    _cond.wait_for(lk, msecs, [this]()
        { return !_tasks.empty() || !stopped; });
    --_nr_contenders;

    if(!_tasks.empty()) {
        ret = _tasks.top();
        _tasks.pop();
    }

    return ret;
}

std::size_t prio_scheduler::size() const
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    return _tasks.size();
}

bool prio_scheduler::empty() const
{
    std::unique_lock<decltype(_lock)> lk(_lock);
    return _tasks.empty();
}

void prio_scheduler::clear()
{
    decltype(_tasks) victim;
    std::unique_lock<decltype(_lock)> lk(_lock);
    _tasks.swap(victim);
}

} // namespace core

} // namespace ultra
