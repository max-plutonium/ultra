#include "task.h"

ultra::task::task(int prio)
    : _state(task_state::undefined), _prio(prio)
{
}
