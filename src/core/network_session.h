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
    explicit network_session(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
    void start();
};

} // namespace core

} // namespace ultra

#endif // NETWORK_SESSION_H
