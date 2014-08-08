#include "../../src/core/thread_pool.h"
#include <gmock/gmock.h>
#include <chrono>

class mock_task : public ultra::task
{
public:
    explicit mock_task(int prio = 0) : ultra::task(prio) { }
    MOCK_METHOD0(run, void());
};

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

    core::thread_pool pool;
    pool.schedule(std::move(ptask));
    pool.wait_for_done();
}

TEST_F(test_thread_pool, schedule_function_nosleep)
{
    using namespace ultra;

    {
        core::thread_pool pool;
        pool.schedule(&test_thread_pool::test_function_nosleep, this);
    }

    ASSERT_EQ(1, _function_count);
}

TEST_F(test_thread_pool, schedule_function_multiple)
{
    using namespace ultra;
    const int runs = 10;

    {
        core::thread_pool pool;
        for(int i = 0; i < runs; ++i)
            pool.schedule(&test_thread_pool::test_function_sleep, this, 1000);
    }

    EXPECT_EQ(runs, _function_count);

    {
        core::thread_pool pool;
        for(int i = 0; i < runs; ++i)
            pool.schedule(&test_thread_pool::test_function_nosleep, this);
    }

    EXPECT_EQ(runs * 2, _function_count);

    {
        core::thread_pool pool;
        for(int i = 0; i < runs; ++i)
            pool.schedule(&test_thread_pool::test_function_empty, this);
    }
}
