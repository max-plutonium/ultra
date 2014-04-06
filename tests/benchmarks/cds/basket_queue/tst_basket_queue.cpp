#include <QString>
#include <QtTest>

#include <cds/init.h>
#include <cds/gc/hp.h> // for cds::HP (Hazard Pointer) garbage collector
#include <cds/container/basket_queue.h>

class tst_basket_queue : public QObject
{
    Q_OBJECT

    // Initialize Hazard Pointer singleton
    cds::gc::HP hpGC;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testCaseHP();
};


void tst_basket_queue::initTestCase()
{
    // Initialize libcds
    cds::Initialize();

    // If main thread uses lock-free containers
    // the main thread should be attached to libcds infrastructure
    cds::threading::Manager::attachThread();
}

void tst_basket_queue::cleanupTestCase()
{
    cds::threading::Manager::detachThread();

    // Terminate libcds
    cds::Terminate();
}

#include <iostream>
#include <memory>
#include <thread>
#include <ext/mt_allocator.h>

struct message
{
    char c[32];

    using pointer = std::shared_ptr<message>;
    using default_allocator = __gnu_cxx::__mt_alloc<message>;
};


template <typename Queue>
struct TestHelper
{
    Queue queue;

    message::default_allocator alloc;
    static constexpr std::size_t num_producers = 8, num_consumers = 8;
    static constexpr std::size_t iterations = 1000000;
    std::atomic_int producer_count, consumer_count;
    std::atomic_bool done;

    std::vector<std::thread>
            producer_threads{num_producers}, consumer_threads{num_consumers};

    TestHelper() : producer_count{0}, consumer_count{0}, done{false}
    { }

    void operator ()()
    {
        auto lambda_producer = [&]() {
            cds::threading::Manager::attachThread();
            for (int i = 0; i != iterations; ++i) {
                producer_count.fetch_add(1, std::memory_order_acq_rel);
                queue.push(std::allocate_shared<message>(alloc));
            }
            cds::threading::Manager::detachThread();
        };

        auto lambda_consumer = [&]() {
            cds::threading::Manager::attachThread();
            while (!done) {
                message::pointer ttt;
                while (queue.pop(ttt))
                    consumer_count.fetch_add(1, std::memory_order_acq_rel);
            }

            message::pointer ttt;
            while (queue.pop(ttt))
                consumer_count.fetch_add(1, std::memory_order_acq_rel);
            cds::threading::Manager::detachThread();
        };

        std::generate(producer_threads.begin(), producer_threads.end(),
                      [&]() { return std::thread(lambda_producer); });

        std::generate(consumer_threads.begin(), consumer_threads.end(),
                      [&]() { return std::thread(lambda_consumer); });

        for(std::thread &thr : producer_threads) thr.join();
        done = true;
        for(std::thread &thr : consumer_threads) thr.join();
    }
};


void tst_basket_queue::testCaseHP()
{
    using hp_basket_queue
        = cds::container::BasketQueue<cds::gc::HP, message::pointer,
            cds::opt::allocator<message::default_allocator>,
            cds::opt::alignment<cds::opt::cache_line_alignment>>;

    TestHelper<hp_basket_queue> hp_tester;

    QBENCHMARK {
        hp_tester();
    }

    std::cerr << "produced " << hp_tester.producer_count << " objects." << std::endl;
    std::cerr << "consumed " << hp_tester.consumer_count << " objects." << std::endl;
}

QTEST_APPLESS_MAIN(tst_basket_queue)

#include "tst_basket_queue.moc"
