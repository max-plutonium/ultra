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

/*static*/ std::intptr_t coroutine_task_base::start(std::intptr_t arg)
{
    auto *self = reinterpret_cast<coroutine_task_base *>(arg);
    assert(self);
    assert(self->_state == running);

    try {
        self->schedule();

    } catch(const task_forced_unwind &) {
        return canceled;

    } catch(...) {
        self->_exception = std::current_exception();
        return error;
    }

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
    _exception = std::make_exception_ptr(task_forced_unwind());
    auto result = core::system::install_context(_ctx, std::intptr_t(this));
    assert(canceled == result);
    _state = canceled;
}

void coroutine_task_base::set_stack_size(std::size_t stack_size)
{
    _stack.first = stack_size;
}

std::size_t coroutine_task_base::stack_size() const
{
    return _stack.first;
}

void coroutine_task_base::yield()
{
    if(!core::system::inside_context())
        return;

    auto *task = reinterpret_cast<coroutine_task_base *>(
                     core::system::yield_context(paused));
    assert(this == task);

    if(_exception)
        std::rethrow_exception(_exception);
}

} // namespace ultra
