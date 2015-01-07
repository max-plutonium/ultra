#include "ultra.h"

namespace ultra {

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

} // namespace ultra
