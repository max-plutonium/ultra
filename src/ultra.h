#ifndef ULTRA_H
#define ULTRA_H

#include <future>
#include <mutex>
#include <condition_variable>
#include <boost/coroutine/all.hpp>

#include "ultra_global.h"
#include "core/action.h"

namespace ultra {

/*!
 * \brief Общий интерфейс задачи, которую необходимо выполнить
 */
class task : public std::enable_shared_from_this<task>
{
protected:
    int _prio; ///< Приоритет задачи

public:
    explicit task(int prio = 0);
    virtual ~task();
    virtual void run() = 0;
    void set_prio(int prio);
    int prio() const;

    friend struct task_prio_less;
    friend struct task_prio_greather;
};

using task_ptr = std::shared_ptr<task>;

/*!
 * \brief Сравнивает две задачи по уменьшению приоритета
 */
struct task_prio_less : std::binary_function<task_ptr, task_ptr, bool>
{
    bool operator()(const task_ptr &lhs, const task_ptr &rhs) const;
    bool operator()(const task &lhs, const task &rhs) const;
};

/*!
 * \brief Сравнивает две задачи по увеличению приоритета
 */
struct task_prio_greather : std::binary_function<task_ptr, task_ptr, bool>
{
    bool operator()(const task_ptr &lhs, const task_ptr &rhs) const;
    bool operator()(const task &lhs, const task &rhs) const;
};

/*!
 * \internal
 */
template <typename Res>
class function_task_base : public task
{
protected:
    std::promise<Res> _promise;

    template <typename Task, typename... Args, std::size_t... Indices>
      void call(Task &task, const std::tuple<Args...> &args,
                core::details::index_sequence<Indices...>)
      {
          try {
              _promise.set_value(task(std::get<Indices>(args)...));

          } catch(...) {
              _promise.set_exception(std::current_exception());
          }
      }

public:
      function_task_base(int prio = 0) : task(prio) { }
};

/*!
 * \internal
 */
template <>
class function_task_base<void> : public task
{
protected:
    std::promise<void> _promise;

    template <typename Task, typename... Args, std::size_t... Indices>
      void call(Task &task, const std::tuple<Args...> &args,
                core::details::index_sequence<Indices...>)
      {
          try {
              task(std::get<Indices>(args)...);
              _promise.set_value();

          } catch(...) {
              _promise.set_exception(std::current_exception());
          }
      }

public:
      function_task_base(int prio = 0) : task(prio) { }
};

template <typename...> class function_task;

/*!
 * \brief Класс, содержащий упакованную конкретную задачу и позволяющий
 * не только отправить ее на выполнение, но и получить объект
 * будущего результата этой задачи
 *
 * Так же может сохранять аргументы и подставлять их позже при вызове
 * функции, осуществляя тем самым замыкание.
 *
 * \snippet ultra.cpp function_task
 *
 * \tparam Res Результат функции.
 * \tparam Args... Аргументы функции.
 */
template <typename Res, typename... Args>
class function_task<Res (Args...)> : public function_task_base<Res>
{
    using _base = function_task_base<Res>;
    using result_type = Res;
    core::action<result_type (Args...)> _task;
    std::tuple<std::decay_t<Args>...> _args;

public:
    /*!
     * \brief Конструирует задачу по переданным параметрам
     *
     * \param prio Приоритет задачи.
     * \param fun Функция задачи.
     * \param args Аргументы задачи.
     */
  template <typename Function, typename... Args2>
    explicit function_task(int prio, Function &&fun, Args2 &&...args)
        : _base(prio), _task(std::forward<Function>(fun))
        , _args(std::forward<Args2>(args)...)
    { }

    virtual void run() final override {
        this->call(_task, _args, core::details::make_index_sequence_for<Args...>());
    }

    /*!
     * \brief Возвращает будущий результат выполнения задачи
     */
    std::future<result_type>
    get_future() { return this->_promise.get_future(); }
};

/*!
 * \brief Тип планирования исполнения задач
 */
enum class schedule_type {
    fifo, lifo, prio
};

/*!
 * \brief Интерфейс планировщика исполнения задач
 */
class scheduler : public std::enable_shared_from_this<scheduler>
{
protected:
    mutable std::mutex _lock;
    std::condition_variable _cond;
    bool stopped = false;

public:
    virtual ~scheduler() = default;

    /*!
     * \brief Помещает задачу в очередь
     */
    virtual void push(std::shared_ptr<task>) = 0;

    /*!
     * \brief Помещает задачу в очередь, планируя время её исполнения
     *
     * Исполняет задачу отложенно с задержкой \a delay_msecs миллисекунд
     * и затем повторяя каждые \a period_msecs миллисекунд.
     */
    void push_timed(std::shared_ptr<task>,
                    std::size_t delay_msecs = 0,
                    std::size_t period_msecs = 0);
    /*!
     * \brief Возвращает задачу, которую надо исполнить, при этом
     * может ждать ее появления \a msecs миллисекунд
     */
    virtual std::shared_ptr<task> schedule(std::chrono::milliseconds msecs =
            std::chrono::milliseconds(0)) = 0;

    /*!
     * \brief Возвращает текущий размер очереди задач
     */
    virtual std::size_t size() const = 0;

    /*!
     * \brief Возвращает true, если очередь задач пуста
     */
    virtual bool empty() const = 0;

    /*!
     * \brief Очищает очередь задач планировщика
     */
    virtual void clear() = 0;

    /*!
     * \brief Останавливает выполнение планировщика
     */
    inline void stop() { std::lock_guard<std::mutex> lk(_lock); stopped = true; }

    /*!
     * \brief Возвращает объект планировщика исходя из переданного
     * в \a type типа планирования задач
     */
    static std::shared_ptr<scheduler> make(schedule_type type);
};

using sched_ptr = std::shared_ptr<scheduler>;

namespace bc = boost::coroutines;

template <typename AsyncResult>
class execution_unit : public function_task<AsyncResult ()>,
        public bc::symmetric_coroutine<void>::call_type
{
protected:
    using _base1 = function_task<AsyncResult ()>;
    using _base2 = bc::symmetric_coroutine<void>::call_type;

    std::unique_ptr<typename bc::symmetric_coroutine<AsyncResult>::call_type> _coro;

    virtual void sched_context(bc::symmetric_coroutine<void>::yield_type &yield) = 0;
    virtual AsyncResult task_context(typename
        bc::symmetric_coroutine<AsyncResult>::yield_type &yield) = 0;

public:
    explicit execution_unit(int prio = 0, std::size_t stack_size = 8192)
        : _base1(prio, [this]() { (*this)(); })
        , _base2(std::bind(&execution_unit::sched_context, this, std::placeholders::_1),
                boost::coroutines::attributes(stack_size))
        , _coro(new typename bc::symmetric_coroutine<AsyncResult>::call_type(
                    std::bind(&execution_unit::task_context, this, std::placeholders::_1),
                                    boost::coroutines::attributes(stack_size)))
    {
    }
};

} // namespace ultra

#endif // ULTRA_H
