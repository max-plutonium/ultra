/*
 * Copyright (C) 2015 Max Plutonium <plutonium.max@gmail.com>
 *
 * This file is part of the test suite of the ULTRA library.
 *
 * The ULTRA library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * The ULTRA library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the ULTRA library. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <gmock/gmock.h>

#include "../ultra/node.h"

using namespace ultra;

#include <deque>

template <typename Message>
class receiver : public node
{
public:
    receiver(executor e = std::experimental::system_executor()) : node(e)
    {
    }

    // Block until a message has been received.
    Message wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]{ return !message_queue_.empty(); });
        Message msg(std::move(message_queue_.front()));
        message_queue_.pop_front();
        return msg;
    }

    // Handle a new message by adding it to the queue and waking a waiter.
    void message_handler(Message msg, node_ref from)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        message_queue_.push_back(std::move(msg));
        condition_.notify_one();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<Message> message_queue_;
};

TEST(Node, OneSignalInSameExecutor)
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    node sender;
    receiver<int> recv1, recv2;

    sender.connect<int>(&recv1, std::bind(&decltype(recv1)::message_handler, &recv1, _1, _2));
    sender.connect<int>(&recv2, std::bind(&decltype(recv2)::message_handler, &recv2, _1, _2));
    sender.emit(123);

    EXPECT_EQ(123, recv1.wait());
    EXPECT_EQ(123, recv2.wait());
}

#include <experimental/loop_scheduler>

TEST(Node, OneSignalInDifferentExecutors)
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    std::experimental::loop_scheduler sched1, sched2;

    node sender;
    receiver<int> recv1(sched1.get_executor());
    receiver<int> recv2(sched2.get_executor());

    sender.connect<int>(&recv1, std::bind(&decltype(recv1)::message_handler, &recv1, _1, _2));
    sender.connect<int>(sched2.get_executor(), std::bind(&decltype(recv2)::message_handler, &recv2, _1, _2));
    sender.emit(123);

    EXPECT_EQ(1, sched1.run());
    EXPECT_EQ(1, sched2.run());

    EXPECT_EQ(123, recv1.wait());
    EXPECT_EQ(123, recv2.wait());
}

TEST(Node, MultipleSignalsInSameExecutor)
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    std::experimental::loop_scheduler sched;

    node sender;
    receiver<int> recv1(sched.get_executor());
    receiver<int> recv2(sched.get_executor());

    sender.connect<int>(&recv1, std::bind(&decltype(recv1)::message_handler, &recv1, _1, _2));
    sender.connect<int>(sched.get_executor(), std::bind(&decltype(recv2)::message_handler, &recv2, _1, _2));
    sender.emit(123);
    sender.emit(456);
    sender.emit(789);

    EXPECT_EQ(5, sched.run());

    EXPECT_EQ(123, recv1.wait());
    EXPECT_EQ(456, recv1.wait());
    EXPECT_EQ(789, recv1.wait());

    EXPECT_EQ(123, recv2.wait());
    EXPECT_EQ(456, recv2.wait());
    EXPECT_EQ(789, recv2.wait());
}

TEST(Node, MultipleSignalsInDifferentExecutors)
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    std::experimental::loop_scheduler sched1, sched2;

    node sender;
    receiver<int> recv1(sched1.get_executor());
    receiver<int> recv2(sched2.get_executor());

    sender.connect<int>(&recv1, std::bind(&decltype(recv1)::message_handler, &recv1, _1, _2));
    sender.connect<int>(sched2.get_executor(), std::bind(&decltype(recv2)::message_handler, &recv2, _1, _2));
    sender.emit(123);
    sender.emit(456);
    sender.emit(789);

    EXPECT_EQ(2, sched1.run());
    EXPECT_EQ(3, sched2.run());

    EXPECT_EQ(123, recv1.wait());
    EXPECT_EQ(456, recv1.wait());
    EXPECT_EQ(789, recv1.wait());

    EXPECT_EQ(123, recv2.wait());
    EXPECT_EQ(456, recv2.wait());
    EXPECT_EQ(789, recv2.wait());
}
