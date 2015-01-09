#include "../../src/ultra.h"
#include "mock_types.h"
#include <boost/lexical_cast.hpp>

using namespace ultra;

TEST(test_task, test)
{
    mock_task t;
    t.set_prio(999);
    EXPECT_EQ(999, t.prio());

    mock_task t2(1000);
    task_prio_less tpl;
    EXPECT_TRUE(tpl(t, t2));

    task_prio_greather tpg;
    EXPECT_TRUE(tpg(t2, t));
}

static int test_function_task_func1(std::string str)
{
    return boost::lexical_cast<int>(str);
}

struct test_function_task_struct
{
    int test_function_task_func2(std::string str)
    {
        return boost::lexical_cast<int>(str);
    }
};

TEST(test_function_task, test)
{
    function_task<int (std::string)> ft(1, test_function_task_func1, "123");
    std::future<int> fut = ft.get_future();
    ft.run();
    EXPECT_EQ(1, ft.prio());
    EXPECT_EQ(123, fut.get());

    test_function_task_struct obj;
    function_task<int (std::string)> ft2(2,
        core::make_action(&test_function_task_struct::test_function_task_func2,
                          &obj), "123");
    std::future<int> fut2 = ft2.get_future();
    ft2.run();
    EXPECT_EQ(2, ft2.prio());
    EXPECT_EQ(123, fut2.get());

    auto ptr = std::make_shared<test_function_task_struct>();
    function_task<int (std::string)> ft3(3,
        core::make_action(&test_function_task_struct::test_function_task_func2,
                          ptr), "123");
    std::future<int> fut3 = ft3.get_future();
    ft3.run();
    EXPECT_EQ(3, ft3.prio());
    EXPECT_EQ(123, fut3.get());
    EXPECT_EQ(2, ptr.use_count());
}

static boost::coroutines::symmetric_coroutine<void>::call_type *coro1, *coro2;

class test_execution_unit : public execution_unit<int>
{
public:
    int a;
    test_execution_unit(int a)
        : execution_unit<int>(0), a(a) { }

    // execution_unit interface
protected:
    virtual void sched_context(boost::coroutines::
            symmetric_coroutine<void>::yield_type &yield)
    {
        (*_coro)(3);
        ++a;
        yield(a == 1 ? *coro2 : *coro1);
        return;
    }
    virtual int task_context(boost::coroutines::
            symmetric_coroutine<int>::yield_type &yield)
    {
        EXPECT_EQ(3, yield.get());
        return 7;
    }
};

TEST(test_execution_unit, test)
{
    test_execution_unit eu(0), eu2(2);
    coro1 = &eu;
    coro2 = &eu2;
    eu.run();
    EXPECT_EQ(1, eu.a);
    EXPECT_EQ(3, eu2.a);
    return;
}
