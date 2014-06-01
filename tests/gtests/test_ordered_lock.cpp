#include "../../src/core/locks.h"
#include "mock_types.h"
#include <gmock/gmock.h>

using testing::InSequence;

class mock_mutex
{
public:
    mock_mutex() = default;
    mock_mutex(const mock_mutex&) = delete;
    mock_mutex &operator=(const mock_mutex&) = delete;
    mock_mutex(mock_mutex&&) noexcept = default;
    mock_mutex &operator=(mock_mutex&&) noexcept = default;
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
};


TEST(test_ordered_lock, ctors)
{
    ultra::core::ordered_lock<mock_mutex, mock_mutex> lock1;

//    not lockable - should not be compiled
//    ultra::core::ordered_lock<mock_mutex, std::string> lock2;
//    ultra::core::ordered_lock<float, mock_mutex> lock3;

    EXPECT_FALSE(lock1.owns_lock());
    EXPECT_FALSE(!!lock1);

    auto pair = lock1.release();
    EXPECT_EQ(nullptr, pair.first);
    EXPECT_EQ(nullptr, pair.second);

    {
        InSequence seq;
        mock_mutex mtx1, mtx2;
        EXPECT_CALL(mtx1, lock()).Times(1);
        EXPECT_CALL(mtx2, lock()).Times(1);
        EXPECT_CALL(mtx1, unlock()).Times(1);
        EXPECT_CALL(mtx2, unlock()).Times(1);

        ultra::core::ordered_lock<mock_mutex, mock_mutex> lock2 { mtx1, mtx2 };

        EXPECT_TRUE(lock2.owns_lock());
        EXPECT_TRUE(!!lock2);
    }

    {
        InSequence seq;
        mock_mutex mtx1, mtx2;
        EXPECT_CALL(mtx1, lock()).Times(0);
        EXPECT_CALL(mtx2, lock()).Times(0);
        EXPECT_CALL(mtx1, unlock()).Times(0);
        EXPECT_CALL(mtx2, unlock()).Times(0);

        ultra::core::ordered_lock<mock_mutex, mock_mutex> lock2 { mtx1, mtx2, std::defer_lock };

        EXPECT_FALSE(lock2.owns_lock());
        EXPECT_FALSE(!!lock2);
    }

    {
        InSequence seq;
        mock_mutex mtx1, mtx2;
        EXPECT_CALL(mtx1, lock()).Times(0);
        EXPECT_CALL(mtx2, lock()).Times(0);
        EXPECT_CALL(mtx1, unlock()).Times(1);
        EXPECT_CALL(mtx2, unlock()).Times(1);

        ultra::core::ordered_lock<mock_mutex, mock_mutex> lock2 { mtx1, mtx2, std::adopt_lock };

        EXPECT_TRUE(lock2.owns_lock());
        EXPECT_TRUE(!!lock2);
    }

    InSequence seq;
    mock_mutex mtx1, mtx2;
    EXPECT_CALL(mtx1, lock()).Times(1);
    EXPECT_CALL(mtx2, lock()).Times(1);
    EXPECT_CALL(mtx1, unlock()).Times(0);
    EXPECT_CALL(mtx2, unlock()).Times(0);

    ultra::core::ordered_lock<mock_mutex, mock_mutex> lock2 { mtx1, mtx2 };

    EXPECT_TRUE(lock2.owns_lock());
    EXPECT_TRUE(!!lock2);

    lock1 = std::move(lock2);

    EXPECT_TRUE(lock1.owns_lock());
    EXPECT_TRUE(!!lock1);
    EXPECT_FALSE(lock2.owns_lock());
    EXPECT_FALSE(!!lock2);

    pair = lock1.release();
    EXPECT_EQ(&mtx1, pair.first);
    EXPECT_EQ(&mtx2, pair.second);

    pair = lock2.release();
    EXPECT_EQ(nullptr, pair.first);
    EXPECT_EQ(nullptr, pair.second);
}

TEST(test_ordered_lock, lock)
{
    InSequence seq;
    mock_mutex mtx1, mtx2;
    EXPECT_CALL(mtx1, lock()).Times(1);
    EXPECT_CALL(mtx2, lock()).Times(1);
    EXPECT_CALL(mtx1, unlock()).Times(0);
    EXPECT_CALL(mtx2, unlock()).Times(0);

    ultra::core::ordered_lock<mock_mutex, mock_mutex> lock { mtx1, mtx2, std::defer_lock };

    lock.lock();

    EXPECT_TRUE(lock.owns_lock());
    EXPECT_TRUE(!!lock);

    EXPECT_THROW(lock.lock(), std::system_error);

    auto pair = lock.release();
    EXPECT_EQ(&mtx1, pair.first);
    EXPECT_EQ(&mtx2, pair.second);

    EXPECT_TRUE(lock.owns_lock());
    EXPECT_TRUE(!!lock);

    EXPECT_THROW(lock.lock(), std::system_error);

    ultra::core::ordered_lock<mock_mutex, mock_mutex> lock2;
    lock2.lock();

    EXPECT_TRUE(lock2.owns_lock());
    EXPECT_TRUE(!!lock2);

    EXPECT_THROW(lock2.lock(), std::system_error);
}

TEST(test_ordered_lock, unlock)
{
    InSequence seq;
    mock_mutex mtx1, mtx2;
    EXPECT_CALL(mtx1, lock()).Times(0);
    EXPECT_CALL(mtx2, lock()).Times(0);
    EXPECT_CALL(mtx1, unlock()).Times(1);
    EXPECT_CALL(mtx2, unlock()).Times(1);

    ultra::core::ordered_lock<mock_mutex, mock_mutex> lock { mtx1, mtx2, std::adopt_lock };

    lock.unlock();

    EXPECT_FALSE(lock.owns_lock());
    EXPECT_FALSE(!!lock);

    EXPECT_THROW(lock.unlock(), std::system_error);

    auto pair = lock.release();
    EXPECT_EQ(&mtx1, pair.first);
    EXPECT_EQ(&mtx2, pair.second);

    EXPECT_FALSE(lock.owns_lock());
    EXPECT_FALSE(!!lock);

    EXPECT_THROW(lock.unlock(), std::system_error);

    ultra::core::ordered_lock<mock_mutex, mock_mutex> lock2;
    lock2.lock();
    lock2.unlock();

    EXPECT_FALSE(lock2.owns_lock());
    EXPECT_FALSE(!!lock2);

    EXPECT_THROW(lock2.unlock(), std::system_error);
}

TEST(test_ordered_lock, swap)
{
    InSequence seq;
    mock_mutex mtx1, mtx2;
    EXPECT_CALL(mtx1, lock()).Times(1);
    EXPECT_CALL(mtx2, lock()).Times(1);
    EXPECT_CALL(mtx1, unlock()).Times(1);
    EXPECT_CALL(mtx2, unlock()).Times(1);

    ultra::core::ordered_lock<mock_mutex, mock_mutex> lock { mtx1, mtx2 };
    ultra::core::ordered_lock<mock_mutex, mock_mutex> lock2;

    EXPECT_TRUE(lock.owns_lock());
    EXPECT_TRUE(!!lock);
    EXPECT_FALSE(lock2.owns_lock());
    EXPECT_FALSE(!!lock2);

    lock.swap(lock2);

    EXPECT_FALSE(lock.owns_lock());
    EXPECT_FALSE(!!lock);
    EXPECT_TRUE(lock2.owns_lock());
    EXPECT_TRUE(!!lock2);
}
