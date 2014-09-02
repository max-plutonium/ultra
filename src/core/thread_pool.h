#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <mutex>
#include <memory>
#include <list>
#include <thread>
#include <future>
#include "concurrent_queue.h"

namespace ultra { namespace core {

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
    task() : _state(task_state::undefined) { }
    virtual ~task() = default;
    virtual void run() = 0;
};

//template <typename Deleter = std::default_delete<task>>
using task_ptr = std::shared_ptr<task/*, Deleter*/>;

template <size_t... Indices>
  struct tuple_indices {
      typedef tuple_indices<Indices..., sizeof... (Indices)> next;
  };

template <size_t N>
  struct tuple_indices_builder {
      typedef typename tuple_indices_builder<N - 1>::type::next type;
  };

template <>
  struct tuple_indices_builder<0> {
      typedef tuple_indices<> type;
  };


template <typename Callable, typename... Args>
  class function_task : public task {
      using result_type = typename std::result_of<Callable(Args...)>::type;
      std::packaged_task<result_type(Args...)> _task;
      const std::tuple<Args...> _args;

    template <std::size_t... Indices>
      void call(tuple_indices<Indices...>) {
          _task(std::get<Indices>(_args)...);
      }

  public:
      explicit function_task(Callable &&fun, Args &&...args)
          : _task(std::forward<Callable>(fun)), _args(std::forward<Args>(args)...)
      { }

      virtual void run() override {
          call(typename tuple_indices_builder<sizeof... (Args)>::type());
      }

      std::future<result_type>
      get_future() { return _task.get_future(); }
  };


//template <typename Thread, typename Task, typename Queue>
class thread_pool
{
    std::atomic_bool _done;
    concurrent_queue<task_ptr, std::mutex> _pending_tasks;
    std::list<std::thread> _threads;

    void worker_thread()
    {
        while(!_done)
        {
            task_ptr ptask;
            if(_pending_tasks.dequeue(ptask))
                ptask->run();
            else
                std::this_thread::yield();
        }
    }

public:
    thread_pool() : _done(false)
    {
        std::size_t thread_count = 1;//std::thread::hardware_concurrency();
        try {
            while(thread_count--)
                _threads.emplace_back(&thread_pool::worker_thread, this);
        } catch(...) {
            _done = true;
            throw;
        }
    }

    ~thread_pool()
    {
        _done = true;
        for(std::thread &thread : _threads)
            thread.join();
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    void run_async(task_ptr &&ptask)
    {
        _pending_tasks.push(std::move(ptask));
    }

  template <typename Callable, typename... Args>
    std::future<typename std::result_of<Callable(Args...)>::type>
    run_async(Callable &&f, Args &&...args)
    {
        using result_type = typename std::result_of<Callable(Args...)>::type;
        using function_task_type = function_task<Callable, Args...>;

        task_ptr ptask = std::make_shared<function_task_type>(
                    std::forward<Callable>(f), std::forward<Args>(args)...);
        std::future<result_type> ret
                = std::static_pointer_cast<function_task_type>(ptask)->get_future();
        run_async(std::move(ptask));
        return ret;
    }
};

} // namespace core

} // namespace ultra

#endif // THREAD_POOL_H
