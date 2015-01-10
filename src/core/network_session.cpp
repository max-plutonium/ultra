#include <boost/asio/streambuf.hpp>
#include <boost/asio/read_until.hpp>
#include <iostream>

#include "network_session.h"

namespace ultra { namespace core {

network_session::network_session(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
    : _socket(std::move(sock)), _timer(_socket->get_io_service())
    , _strand(_socket->get_io_service())
{
}

void network_session::start()
{
    auto self = shared_from_this();

    boost::asio::spawn(_strand,
        [this, self](boost::asio::yield_context yield) {
            try {
                for(;;) {
                    _timer.expires_from_now(std::chrono::seconds(10));

                    boost::asio::streambuf buf;

                    std::size_t n = boost::asio::async_read_until(*_socket, buf, '\n', yield);
                    std::istream in(&buf);
                    std::string data;
                    in >> data;

                    data.push_back('\n');
                    _socket->async_write_some(boost::asio::buffer(data), yield);
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

} // namespace core

} // namespace ultra
