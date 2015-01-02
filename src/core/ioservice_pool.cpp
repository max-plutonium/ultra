#include "ioservice_pool.h"

namespace ultra { namespace core {

ioservice_pool::ioservice_pool(std::size_t pool_size)
    : _next_io_service(0)
{
    if (pool_size == 0)
        throw std::runtime_error("io_service_pool size is 0");

    // Give all the io_services work to do so that their run() functions will not
    // exit until they are explicitly stopped.
    for (std::size_t i = 0; i < pool_size; ++i)
    {
        io_service_ptr io_service(new boost::asio::io_service);
        work_ptr work(new boost::asio::io_service::work(*io_service));
        _ios.push_back(io_service);
        _work.push_back(work);
    }
}

void ioservice_pool::stop()
{
    // Explicitly stop all io_services.
    for (auto &ioservice_ptr : _ios)
        ioservice_ptr->stop();
}

boost::asio::io_service &ioservice_pool::next_io_service()
{
    std::lock_guard<decltype(_mtx)> lk(_mtx);

    // Use a round-robin scheme to choose the next io_service to use.
    boost::asio::io_service& io_service = *_ios[_next_io_service];
    ++_next_io_service;
    if (_next_io_service == _ios.size())
        _next_io_service = 0;
    return io_service;
}

} // namespace core

} // namespace ultra
