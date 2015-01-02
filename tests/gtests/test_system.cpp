#include "../../src/core/system.h"
#include <gmock/gmock.h>

#include "benchmark.h"

using namespace ultra::core;

struct test_system : public testing::Test
{
protected:
    machine_stack stack1, stack2;

public:
    test_system()
        : stack1(system::allocate_stack(4096))
        , stack2(system::allocate_stack(4096))
    { }

    ~test_system() {
        system::deallocate_stack(stack1);
        system::deallocate_stack(stack2);
    }
};

TEST_F(test_system, stack_allocation)
{
    EXPECT_TRUE(system::is_stack_unbound());
    auto def_stacksize = system::default_stacksize();
    auto min_stacksize = system::minimum_stacksize();
    auto max_stacksize = system::maximum_stacksize();
    ASSERT_LE(min_stacksize, def_stacksize);
    ASSERT_LE(def_stacksize, max_stacksize);

    machine_stack stack = system::allocate_stack(65535);
    ASSERT_GE(stack.first, 65535);
    EXPECT_TRUE(stack.second);
    system::deallocate_stack(stack);

    benchmark("test_system: 1000 stack allocs-deallocs", 1000) {
        stack = system::allocate_stack(65535);
        system::deallocate_stack(stack);
    }
}

static std::intptr_t test_context_function(std::intptr_t arg)
{
    EXPECT_TRUE(system::inside_context());
    EXPECT_EQ(1, arg);

    for(std::intptr_t i = arg + 1; i < 10; i += 2)
        EXPECT_EQ(i + 1, system::yield_context(i));

    EXPECT_TRUE(system::inside_context());
    return system::yield_context(10) + 10;
}

TEST_F(test_system, context_switch)
{
    machine_context *child_ctx = system::make_context(stack1, &test_context_function);
    ASSERT_TRUE(child_ctx);

    EXPECT_FALSE(system::inside_context());

    for(std::intptr_t i = 1; i < 10; i += 2)
        EXPECT_EQ(i + 1, system::install_context(child_ctx, i));

    EXPECT_FALSE(system::inside_context());
}

static std::intptr_t test_context_loop(std::intptr_t arg)
{
    while(true)
        arg = system::yield_context(arg - 1);
    return 0;
}

TEST_F(test_system, context_switch_benchmark_loop)
{
    machine_context *child_ctx = system::make_context(stack1, &test_context_loop);
    ASSERT_TRUE(child_ctx);
    benchmark("test_system: 1000x1000 context switches", 1000)
        for(int i = 1000; i;)
            i = system::install_context(child_ctx, i);
}

TEST_F(test_system, context_switch_with_return)
{
    machine_context *child_ctx = system::make_context(stack1, &test_context_function);
    ASSERT_TRUE(child_ctx);

    EXPECT_FALSE(system::inside_context());

    for(std::intptr_t i = 1; i < 10; i += 2)
        EXPECT_EQ(i + 1, system::install_context(child_ctx, i));
    EXPECT_EQ(21, system::install_context(child_ctx, 11));

    EXPECT_FALSE(system::inside_context());
}

static std::intptr_t test_nested_context_function1(std::intptr_t arg)
{
    EXPECT_TRUE(system::inside_context());
    EXPECT_EQ(123, arg);

    arg = system::yield_context(456);
    EXPECT_EQ(789, arg);

    return arg + 1;
}

static std::intptr_t test_nested_context_function2(std::intptr_t arg)
{
    EXPECT_TRUE(system::inside_context());
    EXPECT_NE(0, arg);

    machine_context *nested_ctx = system::make_context(
            *reinterpret_cast<machine_stack *>(arg),
                &test_nested_context_function1, system::current_context());

    EXPECT_EQ(456, system::install_context(nested_ctx, 123));
    EXPECT_TRUE(system::inside_context());
    EXPECT_EQ(100, system::yield_context(-10));
    EXPECT_TRUE(system::inside_context());
    EXPECT_EQ(790, system::install_context(nested_ctx, 789));

    EXPECT_TRUE(system::inside_context());
    return 10;
}

TEST_F(test_system, switch_nested_contexts)
{
    machine_context *child_ctx = system::make_context(stack1, &test_nested_context_function2);
    ASSERT_TRUE(child_ctx);

    EXPECT_FALSE(system::inside_context());
    EXPECT_EQ(-10, system::install_context(child_ctx, reinterpret_cast<std::intptr_t>(&stack2)));
    EXPECT_FALSE(system::inside_context());
    EXPECT_EQ(10, system::install_context(child_ctx, 100));
    EXPECT_FALSE(system::inside_context());
}
