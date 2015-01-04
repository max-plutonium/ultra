#include "../../src/vm.h"
#include <gmock/gmock.h>

TEST(test_port, basic_test)
{
    using namespace ultra;

    ultra::vm vm(0, nullptr);
    port_ptr port1 = std::make_shared<port>(address(1, 2, 3), openmode::out);
    port_ptr port2 = std::make_shared<port>(address(2, 3, 4), openmode::in);

    EXPECT_EQ(address(1, 2, 3), port1->get_address());
    EXPECT_EQ(address(2, 3, 4), port2->get_address());
    EXPECT_EQ(std::size_t(0), port1->time().time());
    EXPECT_EQ(std::size_t(0), port2->time().time());
    EXPECT_EQ(openmode::out, port1->open_mode());
    EXPECT_EQ(openmode::in, port2->open_mode());
}

TEST(test_port, connect_disconnect)
{
    using namespace ultra;

    ultra::vm vm(0, nullptr);
    port_ptr port1 = std::make_shared<port>(address(1, 2, 3), openmode::out);
    port_ptr port2 = std::make_shared<port>(address(2, 3, 4), openmode::in);

    EXPECT_TRUE(port1->connect(port2));
    EXPECT_FALSE(port1->connect(port2));
    port1->disconnect(port2);
    EXPECT_TRUE(port1->connect(port2));
}

TEST(test_port, data_sending)
{
    using namespace ultra;

    ultra::vm vm(0, nullptr);
    port_ptr port1 = std::make_shared<port>(address(1, 2, 3), openmode::out);
    port_ptr port2 = std::make_shared<port>(address(2, 3, 4), openmode::in);

    EXPECT_TRUE(port1->connect(port2));

    std::string ss;
    *port1 << "123";
    vm.loop();
    *port2 >> ss;
    EXPECT_EQ("123", ss);
}

