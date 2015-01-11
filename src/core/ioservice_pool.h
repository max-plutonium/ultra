#ifndef IOSERVICE_POOL_H
#define IOSERVICE_POOL_H

#include <boost/noncopyable.hpp>
#include <memory>
#include <boost/asio/io_service.hpp>
#include <mutex>

namespace ultra { namespace core {

/*!
 * \brief Пул io_service-ов
 *
 * Необходим для хранения и равномерного распределения объектов
 * класса \c io_service объектам, которые в них нуждаются.
 */
class ioservice_pool : private boost::noncopyable
{
public:
    /// Создает пул из \a pool_size io_service-ов
    ioservice_pool(std::size_t pool_size);

    /// Останавливает все io_service-ы в пуле
    void stop();

    /// Возвращает указатель на следующий io_service
    std::shared_ptr<boost::asio::io_service> next_io_service();

private:
    using io_service_ptr = std::shared_ptr<boost::asio::io_service>;
    using work_ptr = std::shared_ptr<boost::asio::io_service::work>;

    std::mutex _mtx;
    std::vector<io_service_ptr> _ios;
    std::vector<work_ptr> _work;
    std::size_t _next_io_service;
};

} // namespace core

} // namespace ultra

#endif // IOSERVICE_POOL_H
