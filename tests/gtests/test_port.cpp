#include "../../src/vm.h"
#include <gmock/gmock.h>

#include <thread>

TEST(test_port, basic_test)
{
    using namespace ultra;

    port port1(openmode::out);
    port port2(openmode::in);

    EXPECT_EQ(std::size_t(0), port1.get_time().time());
    EXPECT_EQ(std::size_t(0), port2.get_time().time());
    EXPECT_EQ(openmode::out, port1.open_mode());
    EXPECT_EQ(openmode::in, port2.open_mode());
}

TEST(test_port, connect_disconnect)
{
    using namespace ultra;

    port_ptr port1 = std::make_shared<port>(openmode::out);
    port_ptr port2 = std::make_shared<port>(openmode::in);

    EXPECT_TRUE(port1->connect(*port2));
    EXPECT_FALSE(port1->connect(*port2));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    port1->disconnect(*port2);
    EXPECT_TRUE(port1->connect(*port2));
}

TEST(test_port, data_sending)
{
    using namespace ultra;

    port_ptr port1 = std::make_shared<port>(openmode::out);
    port_ptr port2 = std::make_shared<port>(openmode::in);

    EXPECT_TRUE(port1->connect(*port2));

    std::string ss;
    *port1 << "123";
    *port1 >> ss;
    EXPECT_EQ("", ss);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    *port2 >> ss;
    EXPECT_EQ("123", ss);
}

