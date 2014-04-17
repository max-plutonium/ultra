#include <QString>
#include <QtTest>

#include <cds/init.h>
#include <cds/gc/hp.h> // for cds::HP (Hazard Pointer) garbage collector
#include <cds/container/cuckoo_map.h>

class tst_maps : public QObject
{
    Q_OBJECT

    // Initialize Hazard Pointer singleton
    cds::gc::HP hpGC;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void cockoo_unordered();
    void cockoo_ordered();
};


void tst_maps::initTestCase()
{
    // Initialize libcds
    cds::Initialize();

    // If main thread uses lock-free containers
    // the main thread should be attached to libcds infrastructure
    cds::threading::Manager::attachThread();
}

void tst_maps::cleanupTestCase()
{
    cds::threading::Manager::detachThread();

    // Terminate libcds
    cds::Terminate();
}

#include <iostream>
#include <memory>
#include <thread>
#include <random>
#include <ext/mt_allocator.h>
#include "../../../../src/address.h"

struct object
{
    char c[32];

    using pointer = std::shared_ptr<object>;
    using default_allocator = __gnu_cxx::__mt_alloc<object>;
};


struct hash1 {
    size_t operator()(ultra::address const& s) const
    {
        return ultra::address_hash()(s);
    }
};

struct hash2 : private hash1 {
    size_t operator()(ultra::address const& s) const
    {
        size_t h = ~(hash1::operator()(s)) ;
        return ~h + 0x9e3779b9 + (h << 6) + (h >> 2) ;
    }
};


class Generator
{
    std::uniform_int_distribution<int> distr;
    std::mt19937_64 generator;

public:
    int operator ()()
    {
        return distr(generator);
    }
};


void tst_maps::cockoo_unordered()
{
    Generator generator;
    std::vector<ultra::address> vec(10000000);
    std::generate(vec.begin(), vec.end(), [&]() {
            return ultra::address {
                generator(), generator(), generator()
            };
        });

    cds::container::CuckooMap<ultra::address, object::pointer,
            cds::container::cuckoo::make_traits
            <
                cds::opt::hash<std::tuple<hash1, hash2>>,
                cds::opt::equal_to<std::equal_to<ultra::address>>,
                cds::container::cuckoo::store_hash<true>,
                cds::opt::node_allocator<object::default_allocator>
            >::type>  cuckoo_map;

    object::default_allocator alloc;
    constexpr int iterations = 2000000, num_producers = 4;
    std::vector<std::thread> producer_threads{num_producers};

    auto lambda_producer = [&]() {
        cds::threading::Manager::attachThread();
        for (int i = 0; i != iterations; ++i) {
            cuckoo_map.emplace(
                ultra::address{generator(), generator(), generator()},
                        std::allocate_shared<object>(alloc));
        }
        cds::threading::Manager::detachThread();
    };

    QBENCHMARK {
        std::generate(producer_threads.begin(), producer_threads.end(),
                      [&]() { return std::thread(lambda_producer); });

        for(std::thread &thr : producer_threads) thr.join();
    }
}

void tst_maps::cockoo_ordered()
{
//    Generator generator;
//    std::vector<ultra::address> vec(1000000);
//    std::generate(vec.begin(), vec.end(), [&]() {
//            return ultra::address {
//                generator(), generator(), generator()
//            };
//        });

//    cds::container::CuckooMap<ultra::address, object,
//            cds::container::cuckoo::make_traits
//            <
//                cds::opt::hash<std::tuple<hash1, hash2>>,
//                cds::opt::less<std::less<ultra::address>>,
//                cds::container::cuckoo::probeset_type<
//                    cds::container::cuckoo::vector<4>>,
//                cds::container::cuckoo::store_hash<true>
//            >::type>  cuckoo_map;

//    QBENCHMARK {
//        for(const ultra::address &addr : vec)
//            cuckoo_map.insert(addr, object());
//    }
}

QTEST_APPLESS_MAIN(tst_maps)

#include "tst_maps.moc"
