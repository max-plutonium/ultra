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
        : _endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 55769)
        , _socket(std::make_shared<boost::asio::ip::tcp::socket>(_ios))
        , _timer(_ios) { }
};

TEST_F(test_network_session, ping_pong)
{
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

#include <random>

TEST_F(test_network_session, input_data)
{
    boost::system::error_code ec;
    _socket->connect(_endpoint, ec);
    ASSERT_FALSE(ec);

    std::ostringstream oss;
    std::uniform_int_distribution<int> distr;
    std::mt19937_64 generator;
    for(int i = 0; i < 100; ++i)
        oss << distr(generator);
    std::string data = oss.str();
    std::string data2 = data;
    data.push_back('\n');

    ultra::internal::request req;
    req.set_type(ultra::internal::request::input_data);
    req.set_data(data);
    data = req.SerializeAsString();
    data.push_back('\n');

    std::size_t n = _socket->write_some(boost::asio::buffer(data), ec);
    ASSERT_FALSE(ec);

    n = boost::asio::read_until(*_socket, _buf, '\n');
    ASSERT_FALSE(ec);

    std::istream in(&_buf);
    ultra::internal::reply rep;
    rep.ParseFromIstream(&in);
    EXPECT_EQ(ultra::internal::reply::output_data, rep.type());
    EXPECT_EQ(data2 + '\n', rep.data());
}
