#include "../../src/core/thread_pool.h"
#include <gmock/gmock.h>
#include "mock_types.h"
#include <chrono>

class test_thread_pool : public testing::Test
{
protected:
    std::atomic_size_t _function_count;

public:
    test_thread_pool() : _function_count(0) { }

    void test_function_empty() { }

    void test_function_nosleep() {
        ++_function_count;
    }

    void test_function_sleep(std::size_t msecs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
        ++_function_count;
    }

    std::string fff(std::string &r) {

        return r.append("123");
    }
};

TEST_F(test_thread_pool, schedule_simple)
{
    using namespace ultra;
    mock_task *mt = new mock_task;
    task_ptr ptask(mt);

    EXPECT_CALL(*mt, run()).Times(1);

    core::thread_pool *pool = core::thread_pool::instance();
    pool->schedule(std::move(ptask));
    pool->wait_for_done();
}

TEST_F(test_thread_pool, schedule_function_nosleep)
{
    using namespace ultra;

    {
        core::thread_pool pool;
        pool.schedule(&test_thread_pool::test_function_nosleep, this);
    }

    ASSERT_EQ(std::size_t(1), _function_count);
}

TEST_F(test_thread_pool, schedule_function_multiple)
{
    using namespace ultra;
    const int runs = 10;

    {
        core::thread_pool pool;
        for(int i = 0; i < runs; ++i)
            pool.schedule(&test_thread_pool::test_function_sleep, this, 100);
    }

    EXPECT_EQ(std::size_t(runs), _function_count);

    {
        core::thread_pool pool;
        for(int i = 0; i < runs; ++i)
            pool.schedule(&test_thread_pool::test_function_nosleep, this);
    }

    EXPECT_EQ(std::size_t(runs * 2), _function_count);

    {
        core::thread_pool pool;
        for(int i = 0; i < 5000; ++i)
            pool.schedule(&test_thread_pool::test_function_empty, this);
    }
}

TEST_F(test_thread_pool, wait_for_done)
{
    using namespace ultra;
    const int runs = 1000;

    for(int i = 0; i < runs; ++i) {
        core::thread_pool pool;
        pool.schedule(&test_thread_pool::test_function_nosleep, this);
    }

    ASSERT_EQ(std::size_t(runs), _function_count);
}

static std::thread::id recycled_id;

static void thread_recycling_function()
{
    recycled_id = std::this_thread::get_id();
}

TEST_F(test_thread_pool, thread_recycling)
{
    using namespace ultra;
    core::thread_pool pool;

    pool.schedule(thread_recycling_function);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::thread::id thread_id_1 = recycled_id;

    pool.schedule(thread_recycling_function);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::thread::id thread_id_2 = recycled_id;
    EXPECT_EQ(thread_id_1, thread_id_2);

    pool.schedule(thread_recycling_function);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::thread::id thread_id_3 = recycled_id;
    EXPECT_EQ(thread_id_2, thread_id_3);
}

TEST_F(test_thread_pool, expiry_timeout)
{
    using namespace ultra;
    core::thread_pool pool;

    std::chrono::milliseconds expiry_timeout =  pool.expiry_timeout();
    pool.set_expiry_timeout(std::chrono::milliseconds(10));
    EXPECT_EQ(std::chrono::milliseconds(10), pool.expiry_timeout());

    pool.schedule(thread_recycling_function);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    std::thread::id thread_id_1 = recycled_id;

    // Запускаем задачу заново, поток должен перезапуститься
    pool.schedule(thread_recycling_function);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    std::thread::id thread_id_2 = recycled_id;
    EXPECT_EQ(thread_id_1, thread_id_2);

}
