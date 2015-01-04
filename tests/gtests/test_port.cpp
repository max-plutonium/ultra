#include "../../src/vm.h"
#include "mock_types.h"
#include "benchmark.h"

TEST(test_port, basic_test)
{
    using namespace ultra;

    ultra::vm vm(0, nullptr);
    port_ptr port1 = std::make_shared<mock_port>(address(1, 2, 3), openmode::out);
    port_ptr port2 = std::make_shared<mock_port>(address(2, 3, 4), openmode::in);

    EXPECT_EQ(address(1, 2, 3), port1->get_address());
    EXPECT_EQ(address(2, 3, 4), port2->get_address());
    EXPECT_EQ(std::size_t(0), port1->time().time());
    EXPECT_EQ(std::size_t(0), port2->time().time());
    EXPECT_EQ(openmode::out, port1->open_mode());
    EXPECT_EQ(openmode::in, port2->open_mode());
}

using testing::InSequence;
using testing::_;

MATCHER_P5(ScalarMessage, type, sender, receiver, time, data, "Message") {
    return ultra::scalar_message(type, sender, receiver, time, data) == *arg;
}

TEST(test_port, connect_disconnect)
{
    using namespace ultra;

    ultra::vm vm(0, nullptr);
    port_ptr port1 = std::make_shared<mock_port>(address(1, 2, 3), openmode::out);
    port_ptr port2 = std::make_shared<mock_port>(address(2, 3, 4), openmode::in);

    InSequence seq;
//    EXPECT_CALL(*std::static_pointer_cast<mock_port>(port2),
//                message(_)).Times(1);

//    EXPECT_CALL(*std::static_pointer_cast<mock_port>(port2),
//                message(ScalarMessage(
//                            scalar_message::port_data,
//                            address(1, 2, 3),
//                            address(2, 3, 4),
//                            ultra::scalar_time(2),
//                            std::string("123"))
//                        )).Times(1);

//    EXPECT_CALL(*std::static_pointer_cast<mock_port>(port2),
//                message(ScalarMessage(
//                            scalar_message::port_data,
//                            address(1, 2, 3),
//                            address(2, 3, 4),
//                            ultra::scalar_time(2),
//                            std::string("456"))
//                        )).Times(1);

//    EXPECT_CALL(*std::static_pointer_cast<mock_port>(port2),
//                message(_)).Times(2);

    EXPECT_TRUE(port1->connect(port2));
    EXPECT_FALSE(port1->connect(port2));

    *port1 << "123";
    *port1 << "456";

    usleep(50000);
    std::string ss;
    *port2 >> ss;
    *port2 >> ss;
    return;
}

