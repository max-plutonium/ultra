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
    const std::tuple<int, int, int, int, int, int> colums;

public:
    test_address()
        : colums(distr(generator), distr(generator), distr(generator),
                   distr(generator), distr(generator), distr(generator))
    {
    }
};


TEST_F(test_address, complex)
{
    int x  = std::get<0>(colums),
        y  = std::get<1>(colums),
        z  = std::get<2>(colums),
        x2 = std::get<3>(colums),
        y2 = std::get<4>(colums),
        z2 = std::get<5>(colums);

    ultra::address addr1;
    EXPECT_EQ(0, addr1.x());
    EXPECT_EQ(0, addr1.y());
    EXPECT_EQ(0, addr1.z());

    ultra::address addr2(x, y, z);
    EXPECT_EQ(x, addr2.x());
    EXPECT_EQ(y, addr2.y());
    EXPECT_EQ(z, addr2.z());

    ultra::address addr3 = { x2, y2, z2 };
    EXPECT_EQ(x2, addr3.x());
    EXPECT_EQ(y2, addr3.y());
    EXPECT_EQ(z2, addr3.z());

    ultra::address addr4(addr2); // copy ctor
    addr1 = addr3;               // copy assign
    EXPECT_TRUE(addr1 == addr1);
    EXPECT_TRUE(addr1 == addr3);
    EXPECT_TRUE(addr2 == addr4);
    EXPECT_TRUE(addr2 != addr3); // Крайне врядли совпадут три координаты
    EXPECT_TRUE(addr1 != addr4);
    EXPECT_TRUE(addr1 != addr2);
    EXPECT_TRUE(addr3 != addr4);

    addr1.set_x(x);
    addr1.set_y(y);
    addr1.set_z(z);
    EXPECT_EQ(x, addr1.x());
    EXPECT_EQ(y, addr1.y());
    EXPECT_EQ(z, addr1.z());
    EXPECT_TRUE(addr1 != addr3);
    EXPECT_TRUE(addr1 == addr2);
    EXPECT_TRUE(addr1 == addr4);
}

#include "benchmark.h"

TEST_F(test_address, ctors_benchmark)
{
    benchmark("address ctors 1", 1000000) {
        ultra::address addr1(std::get<0>(colums), std::get<1>(colums), std::get<2>(colums));
    }

    benchmark("address ctors 2", 1000000) {
        ultra::address addr2 = { std::get<3>(colums), std::get<4>(colums), std::get<5>(colums) };
    }
}

TEST_F(test_address, hash_benchmark)
{
    constexpr int iterations = 1000000;
    std::vector<ultra::address> vec(iterations);
    std::generate(vec.begin(), vec.end(), [&]() {
            return ultra::address {
                distr(generator), distr(generator), distr(generator)
            };
        });

    benchmark("address_hash 1000000", 1) {
        ultra::address_hash hash;
        for(const ultra::address &addr : vec)
            hash(addr);
    }
}
