#include "../../src/core/thread_pool.h"
#include <gmock/gmock.h>
#include <chrono>

class mock_task : public ultra::task
{
public:
    MOCK_METHOD0(run, void());
};

using testing::Mock;

TEST(test_thread_pool, run_async_simple)
{
    using namespace ultra;
    mock_task *mt = new mock_task;
    task_ptr ptask(mt);

    EXPECT_CALL(*mt, run()).Times(1);

    core::thread_pool pool;
    pool.run_async(std::move(ptask));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

std::string fff(std::string &r) {

    return r.append("123");
}

TEST(test_thread_pool, run_async_for_callable)
{
    using namespace ultra;

    core::thread_pool pool;
    std::string rr = "9";
    auto fut = pool.run_async(fff, rr);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::string r = fut.get();
    r.at(1);
}
