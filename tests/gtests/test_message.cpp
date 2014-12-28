#include "../../src/message.h"
#include <gtest/gtest.h>
#include "benchmark.h"

using namespace ultra;

TEST(test_message, marshalling)
{
    scalar_message msg1(scalar_time(123), { 1, 2, 3 }, "data");
    scalar_message msg2;

    benchmark("marshalling 10000", 10000) {
        std::stringstream s;
        s << msg1;
        s >> msg2;
        EXPECT_EQ(msg1, msg2);
    }
}

