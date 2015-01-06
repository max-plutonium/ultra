#include "../../src/address.h"
#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <algorithm>

class test_address : public ::testing::Test
{
protected:
    std::uniform_int_distribution<int> distr;
    std::mt19937_64 generator;
    const std::tuple<int, int, int, int,
                     int, int, int, int> colums;

public:
    test_address()
        : colums(distr(generator), distr(generator), distr(generator), distr(generator),
                 distr(generator), distr(generator), distr(generator), distr(generator))
    {
    }
};


TEST_F(test_address, complex)
{
    int     cluster = std::get<0>(colums),
            space = std::get<1>(colums),
            field = std::get<2>(colums),
            node = std::get<3>(colums),
            cluster2 = std::get<4>(colums),
            space2 = std::get<5>(colums),
            field2 = std::get<6>(colums),
            node2 = std::get<7>(colums);

    ultra::address addr1;
    EXPECT_EQ(0, addr1.cluster());
    EXPECT_EQ(0, addr1.space());
    EXPECT_EQ(0, addr1.field());
    EXPECT_EQ(0, addr1.node());

    ultra::address addr2(cluster, space, field, node);
    EXPECT_EQ(cluster, addr2.cluster());
    EXPECT_EQ(space, addr2.space());
    EXPECT_EQ(field, addr2.field());
    EXPECT_EQ(node, addr2.node());

    ultra::address addr3 = { cluster2, space2, field2, node2 };
    EXPECT_EQ(cluster2, addr3.cluster());
    EXPECT_EQ(space2, addr3.space());
    EXPECT_EQ(field2, addr3.field());
    EXPECT_EQ(node2, addr3.node());

    ultra::address addr4(addr2); // copy ctor
    addr1 = addr3;               // copy assign
    EXPECT_TRUE(addr1 == addr1);
    EXPECT_TRUE(addr1 == addr3);
    EXPECT_TRUE(addr2 == addr4);
    EXPECT_TRUE(addr2 != addr3); // Крайне врядли совпадут 4 поля
    EXPECT_TRUE(addr1 != addr4);
    EXPECT_TRUE(addr1 != addr2);
    EXPECT_TRUE(addr3 != addr4);

    addr1.set_cluster(cluster);
    addr1.set_space(space);
    addr1.set_field(field);
    addr1.set_node(node);
    EXPECT_EQ(cluster, addr1.cluster());
    EXPECT_EQ(space, addr1.space());
    EXPECT_EQ(field, addr1.field());
    EXPECT_EQ(node, addr1.node());
    EXPECT_TRUE(addr1 != addr3);
    EXPECT_TRUE(addr1 == addr2);
    EXPECT_TRUE(addr1 == addr4);
}

#include "benchmark.h"

TEST_F(test_address, ctors_benchmark)
{
    benchmark("address ctors 1", 1000000) {
        ultra::address addr1(std::get<0>(colums), std::get<1>(colums),
                             std::get<2>(colums), std::get<3>(colums));
    }

    benchmark("address ctors 2", 1000000) {
        ultra::address addr2 = { std::get<4>(colums), std::get<5>(colums),
                                 std::get<6>(colums), std::get<7>(colums) };
    }
}

#include <algorithm>

TEST_F(test_address, hash_benchmark)
{
    constexpr int iterations = 1000000;
    std::vector<ultra::address> vec(iterations);
    std::generate(vec.begin(), vec.end(), [&]() {
            return ultra::address {
                distr(generator), distr(generator),
                distr(generator), distr(generator)
            };
        });

    benchmark("address_hash 1000000", 1) {
        ultra::address_hash hash;
        for(const ultra::address &addr : vec)
            hash(addr);
    }
}

TEST_F(test_address, marshalling)
{
    ultra::address addr1(123, 456, 789, 012);
    ultra::address addr2;

    benchmark("marshalling 10000", 10000) {
        std::stringstream s;
        s << addr1;
        s >> addr2;
        EXPECT_EQ(addr1, addr2);
    }
}
