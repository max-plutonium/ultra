#ifndef IOSERVICE_POOL_H
#define IOSERVICE_POOL_H

#include <boost/noncopyable.hpp>
#include <memory>
#include <boost/asio/io_service.hpp>
#include <mutex>

namespace ultra { namespace core {

class ioservice_pool : private boost::noncopyable
{
public:
    /// Construct the io_service pool
    ioservice_pool(std::size_t pool_size);

    /// Stop all io_service objects in the pool
    void stop();

    /// Get an io_service to use
    std::shared_ptr<boost::asio::io_service> next_io_service();

private:
    using io_service_ptr = std::shared_ptr<boost::asio::io_service>;
    using work_ptr = std::shared_ptr<boost::asio::io_service::work>;

    std::mutex _mtx;

    /// The pool of io_services.
    std::vector<io_service_ptr> _ios;

    /// The work that keeps the io_services running
    std::vector<work_ptr> _work;

    /// The next io_service to use for a connection
    std::size_t _next_io_service;
};

} // namespace core

} // namespace ultra

#endif // IOSERVICE_POOL_H
