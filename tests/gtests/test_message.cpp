#include "../../src/message.h"
#include <gtest/gtest.h>
#include "benchmark.h"

//using namespace ultra;

//TEST(test_message, test_and_benchmark)
//{
//    message msg1(message::unknown, scalar_time(123), "data");
//    message msg2;

//    EXPECT_EQ(message::unknown, msg1.type());
//    EXPECT_EQ(address(1, 2, 3), msg1.sender());
//    EXPECT_EQ(address(1, 2, 3), msg1.receiver());
//    EXPECT_EQ(scalar_time(123), msg1.time());
//    EXPECT_EQ("data", msg1.data());

//    std::stringstream s;
//    s << msg1;
//    s >> msg2;
//    EXPECT_EQ(msg1, msg2);

//    EXPECT_EQ(message::unknown, msg2.type());
//    EXPECT_EQ(address(1, 2, 3), msg2.sender());
//    EXPECT_EQ(address(1, 2, 3), msg2.receiver());
//    EXPECT_EQ(scalar_time(123), msg2.time());
//    EXPECT_EQ("data", msg2.data());

//    benchmark("marshalling 1000", 1000) {
//        std::stringstream s;
//        s << msg1;
//        s >> msg2;
//    }
//}
