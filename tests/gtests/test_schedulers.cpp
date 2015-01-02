#include "../../src/execution_service.h"
#include "mock_types.h"

using namespace ultra;

TEST(test_schedulers, fifo)
{
    fifo_scheduler sched;
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());

    task_ptr t(new mock_task);
    sched.push(t);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(1), sched.size());

    task_ptr ret = sched.schedule();
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());
    EXPECT_EQ(t, ret);

    task_ptr t2(new mock_task);
    sched.push(t2);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(1), sched.size());
    sched.clear();
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());

    // Test FIFO
    sched.push(t);
    sched.push(t2);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(2), sched.size());
    EXPECT_EQ(t, sched.schedule());
    EXPECT_EQ(t2, sched.schedule());
}

TEST(test_schedulers, lifo)
{
    lifo_scheduler sched;
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());

    task_ptr t(new mock_task);
    sched.push(t);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(1), sched.size());

    task_ptr ret = sched.schedule();
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());
    EXPECT_EQ(t, ret);

    task_ptr t2(new mock_task);
    sched.push(t2);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(1), sched.size());
    sched.clear();
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());

    // Test LIFO
    sched.push(t);
    sched.push(t2);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(2), sched.size());
    EXPECT_EQ(t2, sched.schedule());
    EXPECT_EQ(t, sched.schedule());
}

TEST(test_schedulers, prio)
{
    prio_scheduler sched;
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());

    task_ptr t(new mock_task(1));
    sched.push(t);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(1), sched.size());

    task_ptr ret = sched.schedule();
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());
    EXPECT_EQ(t, ret);

    task_ptr t2(new mock_task(3));
    sched.push(t2);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(1), sched.size());
    sched.clear();
    EXPECT_TRUE(sched.empty());
    EXPECT_EQ(std::size_t(0), sched.size());

    // Test prio
    task_ptr t3(new mock_task(2));
    sched.push(t3);
    sched.push(t);
    sched.push(t2);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(std::size_t(3), sched.size());
    EXPECT_EQ(t2, sched.schedule());
    EXPECT_EQ(t3, sched.schedule());
    EXPECT_EQ(t, sched.schedule());
}
