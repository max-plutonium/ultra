#include "task.h"

ultra::task::task(int prio)
    : _state(task_state::ready), _prio(prio)
{
}

ultra::task::~task()
{
}
