#include "../../src/core/system.h"
#include <gmock/gmock.h>

#include "benchmark.h"

using namespace ultra::core;

struct test_system : public testing::Test
{
    system::machine_context this_ctx, *child_ctx;
};

TEST_F(test_system, stack_allocation)
{
    EXPECT_TRUE(system::is_stack_unbound());
    auto def_stacksize = system::default_stacksize();
    auto min_stacksize = system::minimum_stacksize();
    auto max_stacksize = system::maximum_stacksize();
    ASSERT_LE(min_stacksize, def_stacksize);
    ASSERT_LE(def_stacksize, max_stacksize);

    system::stack stack = system::allocate_stack(65535);
    ASSERT_GE(stack.first, 65535);
    EXPECT_TRUE(stack.second);
    system::deallocate_stack(stack);

    benchmark("test_system: 1000000 stack allocs-deallocs", 1000000) {
        stack = system::allocate_stack(65535);
        system::deallocate_stack(stack);
    }
}

static test_system *test_system_instance;

static void test_system_context_function(std::intptr_t arg)
{
    ASSERT_EQ(111, arg);
    arg = system::switch_context(*test_system_instance->child_ctx,
                                 test_system_instance->this_ctx, 222);
    ASSERT_EQ(333, arg);
    arg = system::switch_context(*test_system_instance->child_ctx,
                                 test_system_instance->this_ctx, 444);
    ASSERT_EQ(555, arg);
    arg = system::switch_context(*test_system_instance->child_ctx,
                                 test_system_instance->this_ctx, 666);
    while(true) {
        arg = system::switch_context(*test_system_instance->child_ctx,
                                     test_system_instance->this_ctx, arg - 1);
    }
}

TEST_F(test_system, context_switch)
{
    test_system_instance = this;
    system::stack stack = system::allocate_stack(65535);
    child_ctx = system::make_context(stack, &test_system_context_function);
    ASSERT_TRUE(child_ctx);

    std::intptr_t result = system::switch_context(this_ctx, *child_ctx, 111);
    ASSERT_EQ(222, result);
    result = system::switch_context(this_ctx, *child_ctx, 333);
    ASSERT_EQ(444, result);
    result = system::switch_context(this_ctx, *child_ctx, 555);
    ASSERT_EQ(666, result);

    benchmark("test_system: 1000x1000 context switches", 1000)
        for(int i = 1000; i;)
            i = system::switch_context(this_ctx, *child_ctx, i);

    test_system_instance = nullptr;
    system::deallocate_stack(stack);
}
