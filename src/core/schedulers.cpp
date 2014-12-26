#include "schedulers.h"

namespace ultra { namespace core {

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
    task_ptr ret; bool hasTask = true;

    std::unique_lock<decltype(_lock)> lk(_lock);
    if(!_tasks.empty()) {
        ++_nr_contenders;
        while(hasTask = _cond.wait_for(lk, msecs));
        --_nr_contenders;
    }

    if(hasTask) {
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
    task_ptr ret; bool hasTask = true;

    std::unique_lock<decltype(_lock)> lk(_lock);
    if(!_tasks.empty()) {
        ++_nr_contenders;
        while(hasTask = _cond.wait_for(lk, msecs));
        --_nr_contenders;
    }

    if(hasTask) {
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
    task_ptr ret; bool hasTask = true;

    std::unique_lock<decltype(_lock)> lk(_lock);
    if(!_tasks.empty()) {
        ++_nr_contenders;
        while(hasTask = _cond.wait_for(lk, msecs));
        --_nr_contenders;
    }

    if(hasTask) {
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
