#ifndef NETWORK_SESSION_H
#define NETWORK_SESSION_H

#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>

namespace ultra { namespace core {

using socket_ptr = std::shared_ptr<boost::asio::ip::tcp::socket>;

class network_session : public std::enable_shared_from_this<network_session>
{
    socket_ptr _socket;
    boost::asio::steady_timer _timer;
    boost::asio::io_service::strand _strand;

public:
    explicit network_session(socket_ptr sock);
    void start();

protected:
    void handle_ping(std::string data, boost::asio::yield_context yield);
    void handle_input(std::string data, boost::asio::yield_context yield);
};

} // namespace core

} // namespace ultra

#endif // NETWORK_SESSION_H
