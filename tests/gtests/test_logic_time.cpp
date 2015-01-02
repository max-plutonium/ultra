#include "../../src/logic_time.h"
#include <gtest/gtest.h>
#include "benchmark.h"

TEST(test_logic_time, test_and_benchmark)
{
    ultra::scalar_time time1(123);
    ultra::scalar_time time2;

    EXPECT_EQ(std::size_t(123), time1.time());

    std::stringstream s;
    s << time1;
    s >> time2;
    EXPECT_EQ(time1, time2);
    EXPECT_EQ(time1.time(), time2.time());

    EXPECT_EQ(std::size_t(123), time2.time());

    benchmark("marshalling 10000", 10000) {
        std::stringstream s;
        s << time1;
        s >> time2;
    }
}

TEST(test_logic_time, merging)
{
    ultra::scalar_time time1(1), time2(3);

    EXPECT_EQ(std::size_t(1), time1.time());
    time1.advance();
    EXPECT_EQ(std::size_t(2), time1.time());

    time1.merge(time2);
    EXPECT_EQ(std::size_t(3), time1.time());
    EXPECT_EQ(std::size_t(3), time1.time());
}
