#include "../../src/logic_time.h"
#include <gtest/gtest.h>
#include "benchmark.h"

TEST(test_logic_time, marshalling)
{
    ultra::scalar_time time1(123);
    ultra::scalar_time time2;

    benchmark("marshalling 10000", 10000) {
        std::stringstream s;
        s << time1;
        s >> time2;
        EXPECT_EQ(time1, time2);
    }
}
