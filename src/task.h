#ifndef TASK_H
#define TASK_H

#include <atomic>
#include <memory>
#include <future>
#include "ultra_global.h"

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

public:
    task();
    virtual ~task() = default;
    virtual void run() = 0;
};

//template <typename Deleter = std::default_delete<task>>
using task_ptr = std::shared_ptr<task/*, Deleter*/>;

template <typename Callable, typename... Args>
  class function_task : public task
  {
      using result_type = typename std::result_of<Callable(Args...)>::type;
      std::packaged_task<result_type(Args...)> _task;
      const std::tuple<Args...> _args;

    template <std::size_t... Indices>
      void call(tuple_indices<Indices...>)
      { _task(std::get<Indices>(_args)...); }

  public:
      explicit function_task(Callable &&fun, Args &&...args)
          : _task(std::forward<Callable>(fun))
          , _args(std::forward<Args>(args)...)
      { }

      virtual void run() final override {
          call(typename tuple_indices_builder<
               sizeof... (Args)>::type());
      }

      std::future<result_type>
      get_future() { return _task.get_future(); }
  };

} // namespace ultra

#endif // TASK_H
