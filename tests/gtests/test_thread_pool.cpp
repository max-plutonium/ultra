#include "../../src/core/thread_pool.h"
#include "mock_types.h"
#include <chrono>

struct mock_scheduler : ultra::core::scheduler
{
    MOCK_METHOD1(push, void (ultra::task_ptr));
    MOCK_METHOD1(schedule, ultra::task_ptr (std::chrono::milliseconds));
    MOCK_CONST_METHOD0(size, std::size_t ());
    MOCK_CONST_METHOD0(empty, bool ());
    MOCK_METHOD0(clear, void ());
};

using testing::Eq;
using testing::_;
using testing::Return;

TEST(test_thread_pool, schedule_simple)
{
    using namespace ultra;
    mock_task *mt = new mock_task;
    task_ptr ptask(mt);

    core::thread_pool<mock_scheduler> pool(1);

    EXPECT_EQ(std::size_t(0), pool.thread_count());

    EXPECT_CALL(*mt, run()).Times(1);
    EXPECT_CALL(*static_cast<mock_scheduler *>(pool.sched().get()),
                empty()).WillRepeatedly(Return(true));

    pool.execute(ptask);

    EXPECT_EQ(std::size_t(1), pool.thread_count());

    pool.wait_for_done();
}

template <typename Tp>
struct typed_test_thread_pool : testing::Test
{
    ultra::core::thread_pool<Tp> pool;
    std::atomic_size_t _run_count { 0 };

    void test_function_empty() { }

    void test_function_nosleep() {
        ++_run_count;
    }

    void test_function_sleep(std::size_t msecs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
        ++_run_count;
    }
};

struct expiry_timeout_task : ultra::task
{
    std::mutex _mtx;
    std::condition_variable _cond;
    std::size_t _run_count = 0;
    std::thread::id _tid;

    // task interface
public:
    virtual void run()
    {
        std::unique_lock<std::mutex> lk(_mtx);
        _tid = std::this_thread::get_id();
        ++_run_count;
        _cond.notify_one();
    }
};

using test_types = testing::Types<ultra::core::fifo_scheduler,
    ultra::core::lifo_scheduler, ultra::core::prio_scheduler>;
TYPED_TEST_CASE(typed_test_thread_pool, test_types);

TYPED_TEST(typed_test_thread_pool, schedule_function_nosleep)
{
    TestFixture::pool.execute_callable(
                &typed_test_thread_pool<TypeParam>::test_function_nosleep, this);

    TestFixture::pool.wait_for_done();
    ASSERT_EQ(std::size_t(1), TestFixture::_run_count);
}

TYPED_TEST(typed_test_thread_pool, schedule_function_multiple)
{
    const int runs = 10;

    for(int i = 0; i < runs; ++i)
        TestFixture::pool.execute_callable(
                &typed_test_thread_pool<TypeParam>::test_function_sleep, this, 10);

    TestFixture::pool.wait_for_done();
    EXPECT_EQ(std::size_t(runs), TestFixture::_run_count);

    for(int i = 0; i < runs; ++i)
        TestFixture::pool.execute_callable(
                &typed_test_thread_pool<TypeParam>::test_function_nosleep, this);

    TestFixture::pool.wait_for_done();
    EXPECT_EQ(std::size_t(runs * 2), TestFixture::_run_count);

    for(int i = 0; i < 5000; ++i)
        TestFixture::pool.execute_callable(
                &typed_test_thread_pool<TypeParam>::test_function_empty, this);

    TestFixture::pool.wait_for_done();
}

TYPED_TEST(typed_test_thread_pool, wait_for_done)
{
    const int runs = 1000;

    for(int i = 0; i < runs; ++i) {
        TestFixture::pool.execute_callable(
            &typed_test_thread_pool<TypeParam>::test_function_nosleep, this);
    }

    TestFixture::pool.wait_for_done(10);
    EXPECT_EQ(std::size_t(runs), TestFixture::_run_count);
}

TYPED_TEST(typed_test_thread_pool, thread_recycling)
{
    std::shared_ptr<expiry_timeout_task> task = std::make_shared<expiry_timeout_task>();

    std::unique_lock<std::mutex> lk(task->_mtx);

    TestFixture::pool.execute(task);
    task->_cond.wait(lk);
    std::thread::id thread_id_1 = task->_tid;

    TestFixture::pool.execute(task);
    task->_cond.wait(lk);
    std::thread::id thread_id_2 = task->_tid;
    EXPECT_EQ(thread_id_1, thread_id_2);

    TestFixture::pool.execute(task);
    task->_cond.wait(lk);
    std::thread::id thread_id_3 = task->_tid;
    EXPECT_EQ(thread_id_2, thread_id_3);
}

TYPED_TEST(typed_test_thread_pool, expiry_timeout)
{
    std::shared_ptr<expiry_timeout_task> task = std::make_shared<expiry_timeout_task>();

    std::unique_lock<std::mutex> lk(task->_mtx);

    std::chrono::milliseconds expiry_timeout = TestFixture::pool.expiry_timeout();
    TestFixture::pool.set_expiry_timeout(std::chrono::milliseconds(10));
    EXPECT_EQ(std::chrono::milliseconds(10), TestFixture::pool.expiry_timeout());

    TestFixture::pool.execute(task);
    task->_cond.wait(lk);
    EXPECT_EQ(std::size_t(1), task->_run_count);
    std::thread::id thread_id_1 = task->_tid;

    // Ждём больше, чем период "истечения" потока
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Запускаем задачу заново, поток должен перезапуститься
    TestFixture::pool.execute(task);
    task->_cond.wait(lk);
    EXPECT_EQ(std::size_t(2), task->_run_count);
    std::thread::id thread_id_2 = task->_tid;
    EXPECT_EQ(thread_id_1, thread_id_2);

    TestFixture::pool.set_expiry_timeout(expiry_timeout);
    EXPECT_EQ(expiry_timeout, TestFixture::pool.expiry_timeout());
}

#include <unistd.h>

TYPED_TEST(typed_test_thread_pool, timeout_race)
{
    std::shared_ptr<expiry_timeout_task> task = std::make_shared<expiry_timeout_task>();

    TestFixture::pool.set_max_thread_count(1);
    TestFixture::pool.set_expiry_timeout(std::chrono::milliseconds(50));
    constexpr std::size_t nr_tasks = 20;
    for(std::size_t i = 0; i < nr_tasks; ++i) {
        TestFixture::pool.execute(task);
        usleep(55000);
    }

    EXPECT_EQ(std::size_t(20), task->_run_count);
    TestFixture::pool.wait_for_done();
}

struct typed_test_thread_pool_max_thread_count
        : typed_test_thread_pool<mock_scheduler>,
        testing::WithParamInterface<std::size_t>
{ };

TEST_P(typed_test_thread_pool_max_thread_count, max_thread_count_invoke)
{
    using namespace ultra;
    using namespace std::chrono;
    EXPECT_CALL(*static_cast<mock_scheduler *>(pool.sched().get()),
                empty()).WillRepeatedly(Return(true));

    pool.set_max_thread_count(GetParam());
    EXPECT_EQ(GetParam(), pool.max_thread_count());
}

INSTANTIATE_TEST_CASE_P(My, typed_test_thread_pool_max_thread_count,
    testing::Values(1, -1, 2, -2, 12345, -6789, -000));

#include <semaphore.h>

struct waiting_task : ultra::task
{
    sem_t _sem_for_started, _sem_for_finish;
    std::atomic_size_t _run_count { 0 };

    waiting_task() {
        sem_init(&_sem_for_started, 0, 0);
        sem_init(&_sem_for_finish, 0, 0);
    }

    ~waiting_task() {
        sem_destroy(&_sem_for_started);
        sem_destroy(&_sem_for_finish);
    }

    // task interface
public:
    virtual void run()
    {
        _run_count.fetch_add(1);
        sem_post(&_sem_for_started);
        sem_wait(&_sem_for_finish);
    }
};

TYPED_TEST(typed_test_thread_pool, start_stop_threads)
{
    std::shared_ptr<waiting_task> task = std::make_shared<waiting_task>();

    TestFixture::pool.set_max_thread_count(1);
    TestFixture::pool.execute(task);
    sem_trywait(&task->_sem_for_started);

    // Макс. кол-во потоков = 1, пул не должен стартовать другой поток.
    TestFixture::pool.execute(task);
    sem_trywait(&task->_sem_for_started);

    // Увеличиваем макс. кол-во потоков и задача начнет выполняться
    TestFixture::pool.set_max_thread_count(2);
    sem_trywait(&task->_sem_for_started);

    // Но по-прежнему больше задач запустить параллельно нельзя
    TestFixture::pool.execute(task);
    sem_trywait(&task->_sem_for_started);

    // Увеличиваем кол-во потоков, чтобы за раз стартовало больше одного потока
    TestFixture::pool.execute(task);
    TestFixture::pool.set_max_thread_count(4);
    sem_trywait(&task->_sem_for_started);
    sem_trywait(&task->_sem_for_started);

    // Но по-прежнему больше задач запустить параллельно нельзя
    TestFixture::pool.execute(task);
    TestFixture::pool.execute(task);
    sem_trywait(&task->_sem_for_started);
    sem_trywait(&task->_sem_for_started);

    // Уменьшаем кол-во потоков, чтобы 2 активных потока прекратились
    TestFixture::pool.set_max_thread_count(2);
    EXPECT_EQ(std::size_t(4), TestFixture::pool.thread_count());
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);

    // Но пул по-прежнему не может стартовать больше потоков
    TestFixture::pool.execute(task);
    TestFixture::pool.execute(task);
    sem_trywait(&task->_sem_for_started);
    sem_trywait(&task->_sem_for_started);

    // Даём всем оставшимся задачам запуститься
    TestFixture::pool.execute(task);
    TestFixture::pool.execute(task);
    TestFixture::pool.execute(task);
    TestFixture::pool.execute(task);
    TestFixture::pool.set_max_thread_count(8);
    sem_trywait(&task->_sem_for_started);
    sem_trywait(&task->_sem_for_started);
    sem_trywait(&task->_sem_for_started);
    sem_trywait(&task->_sem_for_started);

    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    sem_post(&task->_sem_for_finish);
    TestFixture::pool.wait_for_done();
}

struct typed_test_thread_pool_reserve_thread
        : typed_test_thread_pool<mock_scheduler>,
        testing::WithParamInterface<std::size_t>
{ };

TEST_P(typed_test_thread_pool_reserve_thread, reserve_thread)
{
    using namespace ultra;
    using namespace std::chrono;
    EXPECT_CALL(*static_cast<mock_scheduler *>(pool.sched().get()),
                empty()).WillRepeatedly(Return(true));

    pool.set_max_thread_count(GetParam());

    for (std::size_t i = 0; i < GetParam(); ++i)
        pool.reserve_thread();

    pool.reserve_thread();
    EXPECT_EQ(pool.thread_count(), (GetParam() > 0 ? GetParam() : 0) + 1);
    pool.reserve_thread();
    EXPECT_EQ(pool.thread_count(), (GetParam() > 0 ? GetParam() : 0) + 2);

    pool.release_thread();
    pool.release_thread();

    for (std::size_t i = 0; i < GetParam(); ++i)
        pool.release_thread();
}

INSTANTIATE_TEST_CASE_P(My, typed_test_thread_pool_reserve_thread,
    testing::Values(1, 2, 3, 5, 11, 12345));

TEST_P(typed_test_thread_pool_reserve_thread, release_thread)
{
    using namespace ultra;
    using namespace std::chrono;
    EXPECT_CALL(*static_cast<mock_scheduler *>(pool.sched().get()),
                empty()).WillRepeatedly(Return(true));

    pool.set_max_thread_count(GetParam());

    for (std::size_t i = 0; i < GetParam(); ++i)
        pool.reserve_thread();

    std::size_t reserved = pool.thread_count();
    while(reserved-- > 0) {
        pool.release_thread();
        EXPECT_EQ(reserved, pool.thread_count());
    }

    EXPECT_EQ(std::size_t(0), pool.thread_count());

    pool.release_thread();
    EXPECT_EQ(std::size_t(-1), pool.thread_count());
    pool.reserve_thread();
    EXPECT_EQ(std::size_t(0), pool.thread_count());
}

TYPED_TEST(typed_test_thread_pool, reserve_and_start)
{
    TestFixture::pool.set_max_thread_count(1);
    EXPECT_EQ(std::size_t(0), TestFixture::pool.thread_count());

    TestFixture::pool.reserve_thread();
    EXPECT_EQ(std::size_t(1), TestFixture::pool.thread_count());

    std::shared_ptr<waiting_task> task = std::make_shared<waiting_task>();

    TestFixture::pool.execute(task);
    EXPECT_EQ(std::size_t(2), TestFixture::pool.thread_count());
    sem_wait(&task->_sem_for_started);
    sem_post(&task->_sem_for_finish);
    EXPECT_EQ(std::size_t(1), task->_run_count);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(std::size_t(1), TestFixture::pool.thread_count());

    TestFixture::pool.release_thread();
    EXPECT_EQ(std::size_t(0), TestFixture::pool.thread_count());
}
