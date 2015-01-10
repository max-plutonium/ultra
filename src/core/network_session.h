#ifndef NETWORK_SESSION_H
#define NETWORK_SESSION_H

#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>

namespace ultra { namespace core {

class network_session : public std::enable_shared_from_this<network_session>
{
    std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
    boost::asio::steady_timer _timer;
    boost::asio::io_service::strand _strand;

public:
    explicit network_session(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
        : _socket(std::move(sock)), _timer(_socket->get_io_service())
        , _strand(_socket->get_io_service())
    {
    }

    void start()
    {
        auto self = shared_from_this();

        boost::asio::spawn(_strand,
            [this, self](boost::asio::yield_context yield) {
                try {
                    char data[512];

                    for (;;)
                    {
                        _timer.expires_from_now(std::chrono::seconds(10));
                        std::size_t n = _socket->async_read_some(boost::asio::buffer(data), yield);
                        _socket->async_write_some(boost::asio::buffer(data, n), yield);
                    }
                }
                catch (std::exception &e)
                {
                    _socket->close();
                    _timer.cancel();
                }
            });


        boost::asio::spawn(_strand,
            [this, self](boost::asio::yield_context yield) {
                while (_socket->is_open()) {
                    boost::system::error_code ignored_ec;
                    _timer.async_wait(yield[ignored_ec]);
                    if (_timer.expires_from_now() <= std::chrono::seconds(0))
                        _socket->close();
                }
            });
    }
};

} // namespace core

} // namespace ultra

#endif // NETWORK_SESSION_H
