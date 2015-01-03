#include "../../src/logic_time.h"
#include <gtest/gtest.h>
#include "benchmark.h"

TEST(test_logic_time, scalar_time)
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

TEST(test_logic_time, scalar_time_merging)
{
    ultra::scalar_time time1(1), time2(3);

    EXPECT_EQ(std::size_t(1), time1.time());
    time1.advance();
    EXPECT_EQ(std::size_t(2), time1.time());

    EXPECT_NE(time1, time2);
    EXPECT_LT(time1, time2);

    time1.merge(time2);
    EXPECT_EQ(std::size_t(3), time1.time());
    EXPECT_EQ(std::size_t(3), time2.time());
}

TEST(test_logic_time, vector_time)
{
    ultra::vector_time time1(1, 5);
    ultra::vector_time time2(2, 3);

    EXPECT_EQ(std::size_t(5), time1.time().size());
    EXPECT_EQ(std::size_t(3), time2.time().size());

    std::stringstream s;
    s << time1;
    s >> time2;
    EXPECT_EQ(time1, time2);

    EXPECT_EQ(std::size_t(5), time1.time().size());
    EXPECT_EQ(std::size_t(5), time2.time().size());

    benchmark("marshalling 10000", 10000) {
        std::stringstream s;
        s << time1;
        s >> time2;
    }
}

TEST(test_logic_time, vector_time_merging)
{
    ultra::vector_time time1(1, 3), time2(2, 3);

    std::vector<std::size_t> va1 { 0, 0, 0 };
    std::vector<std::size_t> va2 { 0, 1, 0 };
    std::vector<std::size_t> va3 { 0, 0, 1 };
    std::vector<std::size_t> va4 { 0, 1, 1 };
    std::vector<std::size_t> va5 { 0, 1, 2 };

    EXPECT_EQ(va1, time1.time());
    EXPECT_EQ(va1, time2.time());
    time1.advance();
    time2.advance();
    EXPECT_EQ(va2, time1.time());
    EXPECT_EQ(va3, time2.time());

    EXPECT_NE(time1, time2);

    time1.merge(time2);
    EXPECT_EQ(va4, time1.time());

    EXPECT_LT(time2, time1);

    time2.advance();
    time1.merge(time2);
    EXPECT_EQ(va5, time1.time());
}
