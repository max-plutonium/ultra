#include "core_p.h"

namespace ultra {

delayed_task::delayed_task(const task_ptr &t, const std::weak_ptr<scheduler> &sched)
    : _task(t), _timer(vm::impl::get()->next_io_service()), _sched(sched)
{
}

void delayed_task::start(std::size_t msecs)
{
    _timer.expires_from_now(boost::posix_time::milliseconds(msecs));
    _timer.async_wait(std::bind(&delayed_task::resume, shared_from_this()));
}

void delayed_task::resume()
{
    if(sched_ptr sched = _sched.lock()) {
        sched->push(_task);
    }
}

} // namespace ultra
