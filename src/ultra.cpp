#include "ultra.h"

namespace ultra {

const char *task_forced_unwind::what() const noexcept
{
    return "task forced unwind exception";
}

/*!
 * \brief Конструктор интерфейса задачи
 *
 * \param prio Приоритет задачи.
 */
task::task(int prio) : _prio(prio)
{
}

/*!
 * \brief Деструктор
 */
task::~task()
{
}

/*!
 * \fn task::run
 *
 * \brief Метод, содержащий логику выполнения задачи
 */

/*!
 * \brief Устанавливает приоритет задачи
 *
 * \param prio Приоритет задачи.
 */
void task::set_prio(int prio)
{
    _prio = prio;
}

/*!
 * \brief Возвращает приоритет задачи
 */
int task::prio() const
{
    return _prio;
}

bool task_prio_less::operator()(const task_ptr &lhs, const task_ptr &rhs) const
{
    return lhs->_prio < rhs->_prio;
}

bool task_prio_less::operator()(const task &lhs, const task &rhs) const
{
    return lhs._prio < rhs._prio;
}

bool task_prio_greather::operator()(const task_ptr &lhs, const task_ptr &rhs) const
{
    return lhs->_prio > rhs->_prio;
}

bool task_prio_greather::operator()(const task &lhs, const task &rhs) const
{
    return lhs._prio > rhs._prio;
}

static thread_local coroutine_task_base *t_this_coro;

/*static*/ std::intptr_t coroutine_task_base::start(std::intptr_t arg)
{
    auto *self = reinterpret_cast<coroutine_task_base *>(arg);
    assert(self);
    assert(self->_state == running);
    t_this_coro = self;

    try {
        self->schedule();

    } catch(const task_forced_unwind &) {
        t_this_coro = nullptr;
        return canceled;

    } catch(...) {
        self->_exception = std::current_exception();
        t_this_coro = nullptr;
        return error;
    }

    t_this_coro = nullptr;
    return finished;
}

coroutine_task_base::coroutine_task_base(std::size_t stack_size)
    : _ctx(nullptr), _stack(stack_size, nullptr), _state(not_init)
{
}

coroutine_task_base::~coroutine_task_base()
{
    if(_state != not_init)
        core::system::deallocate_stack(_stack);
}

void coroutine_task_base::lazy_init()
{
    _stack = core::system::allocate_stack(_stack.first);
    assert(_stack.second);
    _ctx = core::system::make_context(_stack, &start);
    assert(_ctx);
}

void coroutine_task_base::unwind()
{
    if(core::system::inside_context())
        return;

    _exception = std::make_exception_ptr(task_forced_unwind());
    auto result = core::system::install_context(_ctx, std::intptr_t(this));
    assert(canceled == result);
    _state = canceled;
}

/*!
 * \brief Устанавливает размер стека сопроцедуры до ее фактического запуска
 *
 * \param stack_size Размер стека
 */
void coroutine_task_base::set_stack_size(std::size_t stack_size)
{
    _stack.first = stack_size;
}

/*!
 * \brief Возвращает размер стека сопроцедуры
 */
std::size_t coroutine_task_base::stack_size() const
{
    return _stack.first;
}

/*!
 * \brief Выходит из контекста выполнения сопроцедуры
 *
 * \note Если функция вызвана вне контекста сопроцедуры, она ничего не делает.
 */
void coroutine_task_base::yield()
{
    if(!core::system::inside_context())
        return;

    t_this_coro = nullptr;
    auto *task = reinterpret_cast<coroutine_task_base *>(
                     core::system::yield_context(paused));
    assert(this == task);
    t_this_coro = this;

    if(_exception)
        std::rethrow_exception(_exception);
}

/*!
 * \brief Выходит из текущего контекста выполнения сопроцедуры
 *
 * \note Если функция вызвана вне контекста сопроцедуры, она ничего не делает.
 */
void this_coroutine::yield()
{
    if(t_this_coro)
        t_this_coro->yield();
}

} // namespace ultra
