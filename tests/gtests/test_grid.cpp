#include "../../src/core/grid.h"
#include <gtest/gtest.h>
#include "benchmark.h"

#include <string>

using namespace ultra;

TEST(test_grid, insert)
{
    core::grid<int, int, std::string> g;

    EXPECT_EQ(std::size_t(0), g.size_x());
    EXPECT_EQ(std::size_t(0), g.size_y());

    g.insert_x(1);
    g.insert_x(2);
    g.insert_x(3);
    g.insert_y(7);
    g.insert_y(8);
    g.insert_y(9);

    EXPECT_EQ(std::size_t(3), g.size_x());
    EXPECT_EQ(std::size_t(3), g.size_y());

    EXPECT_TRUE(g.contains_x(1));
    EXPECT_TRUE(g.contains_x(2));
    EXPECT_TRUE(g.contains_x(3));
    EXPECT_FALSE(g.contains_x(7));
    EXPECT_FALSE(g.contains_x(8));
    EXPECT_FALSE(g.contains_x(9));

    EXPECT_TRUE(g.contains_y(7));
    EXPECT_TRUE(g.contains_y(8));
    EXPECT_TRUE(g.contains_y(9));
    EXPECT_FALSE(g.contains_y(1));
    EXPECT_FALSE(g.contains_y(2));
    EXPECT_FALSE(g.contains_y(3));

    g.dump();
}

TEST(test_grid, remove)
{
    core::grid<int, int, std::string> g;

    g.insert_x(1);
    g.insert_x(2);
    g.insert_x(3);
    g.insert_y(7);
    g.insert_y(8);
    g.insert_y(9);

    EXPECT_EQ(std::size_t(3), g.size_x());
    EXPECT_EQ(std::size_t(3), g.size_y());

    EXPECT_TRUE(g.remove_x(1));
    EXPECT_TRUE(g.remove_x(2));
    EXPECT_TRUE(g.remove_x(3));
    EXPECT_FALSE(g.remove_x(1));
    EXPECT_FALSE(g.remove_x(2));
    EXPECT_FALSE(g.remove_x(3));
    EXPECT_FALSE(g.remove_x(7));
    EXPECT_FALSE(g.remove_x(8));
    EXPECT_FALSE(g.remove_x(9));

    EXPECT_TRUE(g.remove_y(7));
    EXPECT_TRUE(g.remove_y(8));
    EXPECT_TRUE(g.remove_y(9));
    EXPECT_FALSE(g.remove_y(7));
    EXPECT_FALSE(g.remove_y(8));
    EXPECT_FALSE(g.remove_y(9));
    EXPECT_FALSE(g.remove_y(1));
    EXPECT_FALSE(g.remove_y(2));
    EXPECT_FALSE(g.remove_y(3));

    EXPECT_EQ(std::size_t(0), g.size_x());
    EXPECT_EQ(std::size_t(0), g.size_y());
}

TEST(test_grid, connect)
{
    core::grid<int, int, std::string> g;

    g.insert_x(1);
    g.insert_x(2);
    g.insert_x(3);
    g.insert_y(7);
    g.insert_y(8);
    g.insert_y(9);

    EXPECT_TRUE(g.connect(1, 7));
    EXPECT_TRUE(g.connect(2, 8));
    EXPECT_TRUE(g.connect(3, 9));
    EXPECT_FALSE(g.connect(1, 7));
    EXPECT_FALSE(g.connect(2, 8));
    EXPECT_FALSE(g.connect(3, 9));

    EXPECT_TRUE(g.connected(1, 7));
    EXPECT_TRUE(g.connected(2, 8));
    EXPECT_TRUE(g.connected(3, 9));
    EXPECT_FALSE(g.connected(3, 7));
    EXPECT_FALSE(g.connected(1, 8));
    EXPECT_FALSE(g.connected(2, 9));

    EXPECT_TRUE(g.disconnect(1, 7));
    EXPECT_TRUE(g.disconnect(2, 8));
    EXPECT_TRUE(g.disconnect(3, 9));
    EXPECT_FALSE(g.disconnect(1, 7));
    EXPECT_FALSE(g.disconnect(2, 8));
    EXPECT_FALSE(g.disconnect(3, 9));
}

TEST(test_grid, iterator)
{
    core::grid<int, int, std::string> g;

    g.insert_x(1);
    g.insert_x(2);
    g.insert_x(3);
    g.insert_y(7);
    g.insert_y(8);
    g.insert_y(9);

    EXPECT_TRUE(g.connect(1, 7));
    EXPECT_TRUE(g.connect(2, 8));
    EXPECT_TRUE(g.connect(3, 9));

    EXPECT_EQ(std::make_pair(1, 7), *g.xbegin(1));
    EXPECT_EQ(std::make_pair(2, 8), *g.xbegin(2));
    EXPECT_EQ(std::make_pair(3, 9), *g.xbegin(3));
    EXPECT_EQ(std::make_pair(1, 7), *g.ybegin(7));
    EXPECT_EQ(std::make_pair(2, 8), *g.ybegin(8));
    EXPECT_EQ(std::make_pair(3, 9), *g.ybegin(9));
}

#include <random>
#include <algorithm>

TEST(test_grid, benchmark)
{
    std::uniform_int_distribution<int> distr;
    std::mt19937_64 generator;
    constexpr int iterations = 1000;
    std::vector<int> vec(iterations);
    std::generate(vec.begin(), vec.end(), [&]() {
            return distr(generator);
        });

    core::grid<int, int, std::string> g;

    benchmark("inserting 1000000", 1000) {
        for(int v : vec)
            g.insert_x(v);
    }

    benchmark("removing 1000000", 1000) {
        for(int v : vec)
            g.remove_x(v);
    }

    benchmark("connecting 1000000", 1000) {
        for(int v : vec)
            g.connect(v, 1);
    }

    benchmark("test connected 1000000", 1000) {
        for(int v : vec)
            g.connected(v, 1);
    }

    benchmark("disconnecting 1000000", 1000) {
        for(int v : vec)
            g.disconnect(v, 1);
    }

    benchmark("test connected 1000000", 1000) {
        for(int v : vec)
            g.connected(v, 1);
    }
}
