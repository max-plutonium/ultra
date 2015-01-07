#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include <deque>
#include <queue>

#include "../ultra.h"

namespace ultra { namespace core {

class fifo_scheduler : public scheduler
{
    std::deque<task_ptr> _tasks;
    std::size_t _nr_contenders = 0;

    // scheduler interface
public:
    virtual void push(task_ptr) override;
    virtual task_ptr schedule(std::chrono::milliseconds =
            std::chrono::milliseconds(0)) override;
    virtual std::size_t size() const override;
    virtual bool empty() const override;
    virtual void clear() override;
};

class lifo_scheduler : public scheduler
{
    std::deque<task_ptr> _tasks;
    std::size_t _nr_contenders = 0;

    // scheduler interface
public:
    virtual void push(task_ptr) override;
    virtual task_ptr schedule(std::chrono::milliseconds =
            std::chrono::milliseconds(0)) override;
    virtual std::size_t size() const override;
    virtual bool empty() const override;
    virtual void clear() override;
};

class prio_scheduler : public scheduler
{
    using task_queue = std::priority_queue<task_ptr,
        std::vector<task_ptr>, task_prio_less>;
    task_queue _tasks;
    std::size_t _nr_contenders = 0;

    // scheduler interface
public:
    virtual void push(task_ptr) override;
    virtual task_ptr schedule(std::chrono::milliseconds =
            std::chrono::milliseconds(0)) override;
    virtual std::size_t size() const override;
    virtual bool empty() const override;
    virtual void clear() override;
};

} // namespace core

} // namespace ultra

#endif // SCHEDULERS_H
