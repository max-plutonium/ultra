#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include <mutex>
#include <queue>
#include "../task.h"

namespace ultra { namespace core {

struct scheduler
{
    virtual void push(task_ptr) = 0;
    virtual task_ptr schedule(std::chrono::milliseconds) = 0;
    virtual std::size_t size() const = 0;
    virtual bool empty() const = 0;
    virtual void clear() = 0;
};

struct executor
{
    virtual void execute(task_ptr) = 0;
};

class fifo_scheduler : public scheduler
{
    mutable std::mutex _lock;
    std::condition_variable _cond;
    std::deque<task_ptr> _tasks;
    std::size_t _nr_contenders = 0;

    // scheduler interface
public:
    virtual void push(task_ptr) override;
    virtual task_ptr schedule(std::chrono::milliseconds) override;
    virtual std::size_t size() const override;
    virtual bool empty() const override;
    virtual void clear() override;
};

class lifo_scheduler : public scheduler
{
    mutable std::mutex _lock;
    std::condition_variable _cond;
    std::deque<task_ptr> _tasks;
    std::size_t _nr_contenders = 0;

    // scheduler interface
public:
    virtual void push(task_ptr) override;
    virtual task_ptr schedule(std::chrono::milliseconds) override;
    virtual std::size_t size() const override;
    virtual bool empty() const override;
    virtual void clear() override;
};

class prio_scheduler : public scheduler
{
    mutable std::mutex _lock;
    std::condition_variable _cond;
    using task_queue = std::priority_queue<task_ptr,
        std::vector<task_ptr>, task_prio_less>;
    task_queue _tasks;
    std::size_t _nr_contenders = 0;

    // scheduler interface
public:
    virtual void push(task_ptr) override;
    virtual task_ptr schedule(std::chrono::milliseconds) override;
    virtual std::size_t size() const override;
    virtual bool empty() const override;
    virtual void clear() override;
};

} // namespace core

} // namespace ultra


#endif // SCHEDULERS_H
