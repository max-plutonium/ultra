#include "core_p.h"

namespace ultra {

/************************************************************************************
    timed_task
 ***********************************************************************************/
timed_task::timed_task(const task_ptr &t, execution_service *executor)
    : _task(t), _timer(*g_instance->d->next_io_service()), _exec(executor)
{
}

timed_task::timed_task(const task_ptr &t,
                       const std::shared_ptr<boost::asio::io_service> &ios,
                       execution_service *executor)
    : _task(t), _timer(*ios), _exec(executor)
{
}

void timed_task::start(std::size_t delay_msecs, std::size_t period_msecs)
{
    if(_exec->stopped())
        return;

    if(delay_msecs == 0) {
        _exec->execute(_task);

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

network_task::network_task(int prio, const std::shared_ptr<boost::asio::io_service> &ios)
    : task(prio), _ios(ios)
{
}

void network_task::run()
{
    _ios->run_one();
}

} // namespace ultra
