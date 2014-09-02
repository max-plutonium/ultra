#include "../../src/core/thread_pool.h"
#include <gmock/gmock.h>
#include <chrono>
#include <queue>

class mock_task : public ultra::task
{
public:
    explicit mock_task(int prio = 0) : ultra::task(prio) { }
    MOCK_METHOD0(run, void());
};

TEST(test_thread_pool, schedule_simple)
{
    using namespace ultra;
    mock_task *mt = new mock_task;
    task_ptr ptask(mt);

    EXPECT_CALL(*mt, run()).Times(1);

    core::thread_pool pool;
    pool.schedule(std::move(ptask));
    pool.wait_for_done();
}

std::string fff(std::string &r) {

    return r.append("123");
}

TEST(test_thread_pool, schedule_callable)
{
    using namespace ultra;

    core::thread_pool pool;
    std::string rr = "9";
    auto fut = pool.schedule(fff, rr);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::string r = fut.get();
    r.at(1);
}
