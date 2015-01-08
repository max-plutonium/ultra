#include "core_p.h"

namespace ultra {

/************************************************************************************
    timed_task
 ***********************************************************************************/
timed_task::timed_task(const task_ptr &t,
                       std::shared_ptr<boost::asio::io_service> ios,
                       const std::weak_ptr<scheduler> &sched)
    : _task(t), _ios(std::move(ios)), _timer(*_ios), _sched(sched)
{
}

void timed_task::start(std::size_t delay_msecs, std::size_t period_msecs)
{
    if(delay_msecs == 0) {
        if(sched_ptr sched = _sched.lock())
            sched->push(_task);

        if(period_msecs > 0) {
            _timer.expires_from_now(boost::posix_time::milliseconds(period_msecs));
            _timer.async_wait(std::bind(&timed_task::start, shared_from_this(),
                                        0, period_msecs));
        }

        return;
    }

    _timer.expires_from_now(boost::posix_time::milliseconds(delay_msecs));
    _timer.async_wait(std::bind(&timed_task::start, shared_from_this(), 0, period_msecs));
}

} // namespace ultra
