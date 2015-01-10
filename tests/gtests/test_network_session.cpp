#include "../../src/core/network_session.h"
#include <gmock/gmock.h>
#include <memory>

#include "../../src/vm.h"

class test_network_session : public testing::Test
{
protected:
    boost::asio::io_service _ios;
    boost::asio::ip::tcp::endpoint _endpoint;
    std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
    boost::asio::steady_timer _timer;
    char _data[512];

public:
    test_network_session()
        : _endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 55799)
        , _socket(std::make_shared<boost::asio::ip::tcp::socket>(_ios))
        , _timer(_ios)
    {
        std::memset(_data, 0, 512);
    }
};

TEST_F(test_network_session, test)
{
    const char *argv[] = { "vm",
                           "--num-threads=2",
                           "--num-network-threads=2",
                           "--num-reactors=1",
                           "--address=127.0.0.1",
                           "--port=55799",
                           "--cluster=0"};
    ultra::vm vm(7, argv);

    boost::system::error_code ec;
    _socket->connect(_endpoint, ec);
    ASSERT_FALSE(ec);

    *_data = 1;

    _socket->write_some(boost::asio::buffer(_data), ec);
    ASSERT_FALSE(ec);

    _socket->read_some(boost::asio::buffer(_data), ec);
    ASSERT_FALSE(ec);
}
