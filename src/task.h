#ifndef TASK_H
#define TASK_H

#include <atomic>
#include <memory>
#include <future>
#include "core/action.h"

namespace ultra {

enum class task_state : char
{
    undefined = -1,
    zombie,
    running,
    timed_wait,
    monitor,
    wait,

    initializing,
    starting,
    suspended,
    stopping
};

class task : public std::enable_shared_from_this<task>
{
protected:
    std::atomic<task_state> _state;
    int _prio;

public:
    explicit task(int prio = 0);
    virtual ~task() = default;
    virtual void run() = 0;

    friend struct task_prio_less;
    friend struct task_prio_greather;
};

using task_ptr = std::shared_ptr<task>;

struct task_prio_less : std::binary_function<task_ptr, task_ptr, bool>
{
    bool operator()(const ultra::task_ptr &lhs, const ultra::task_ptr &rhs) const
    { return lhs->_prio < rhs->_prio; }
};

struct task_prio_greather : std::binary_function<task_ptr, task_ptr, bool>
{
    bool operator()(const ultra::task_ptr &lhs, const ultra::task_ptr &rhs) const
    { return lhs->_prio > rhs->_prio; }
};

template <typename Callable, typename... Args>
class function_task : public task
{
    using result_type = typename std::result_of<Callable (Args...)>::type;
    std::packaged_task<result_type (Args...)> _task;
    const std::tuple<Args...> _args;

    template <std::size_t... Indices>
    void call(core::details::index_sequence<Indices...>)
    { _task(std::get<Indices>(_args)...); }

public:
    explicit function_task(Callable &&fun, Args &&...args)
        : _task(std::forward<Callable>(fun))
        , _args(std::forward<Args>(args)...)
    { }

    virtual void run() final override {
        call(core::details::make_index_sequence_for<Args...>());
    }

    std::future<result_type>
    get_future() { return _task.get_future(); }
};

} // namespace ultra

#endif // TASK_H
