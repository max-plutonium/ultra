#include "../../src/core/network_session.h"
#include <gmock/gmock.h>
#include <memory>

#include <boost/asio/streambuf.hpp>
#include <boost/asio/read_until.hpp>
#include <iostream>
#include "../../src/vm.h"

#include "../../src/msg.pb.h"

class test_network_session : public testing::Test
{
protected:
    boost::asio::io_service _ios;
    boost::asio::ip::tcp::endpoint _endpoint;
    std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
    boost::asio::steady_timer _timer;
    boost::asio::streambuf _buf;

public:
    test_network_session()
        : _endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 55789)
        , _socket(std::make_shared<boost::asio::ip::tcp::socket>(_ios))
        , _timer(_ios) { }
};

TEST_F(test_network_session, ping_pong)
{
    const char *argv[] = { "vm",
                           "--num-threads=2",
                           "--num-network-threads=2",
                           "--num-reactors=1",
                           "--address=127.0.0.1",
                           "--port=55789",
                           "--cluster=0"};
    ultra::vm vm(7, argv);

    boost::system::error_code ec;
    _socket->connect(_endpoint, ec);
    ASSERT_FALSE(ec);

    ultra::internal::request req;
    req.set_type(ultra::internal::request::ping);
    req.set_data("ping");
    std::string data = req.SerializeAsString();
    data.push_back('\n');

    std::size_t n = _socket->write_some(boost::asio::buffer(data), ec);
    ASSERT_FALSE(ec);

    n = boost::asio::read_until(*_socket, _buf, '\n');
    ASSERT_FALSE(ec);

    std::istream in(&_buf);
    ultra::internal::reply rep;
    rep.ParseFromIstream(&in);
    EXPECT_EQ(ultra::internal::reply::pong, rep.type());
    EXPECT_EQ("pong", rep.data());
}
