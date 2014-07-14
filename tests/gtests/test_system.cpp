#include "../../src/core/system.h"
#include <gmock/gmock.h>

using namespace ultra::core;

struct test_system : public testing::Test
{
    system::machine_context_ptr this_ctx, child_ctx;
};

TEST_F(test_system, stack_allocation)
{
    EXPECT_TRUE(system::is_stack_unbound());
    auto def_stacksize = system::default_stacksize();
    auto min_stacksize = system::minimum_stacksize();
    auto max_stacksize = system::maximum_stacksize();
    ASSERT_LE(min_stacksize, def_stacksize);
    ASSERT_LE(def_stacksize, max_stacksize);

    system::stack s = system::allocate_stack(65535);
    ASSERT_GE(s.first, 65535);
    EXPECT_TRUE(s.second);
    system::deallocate_stack(s);
}

static test_system *test_instance;

void context_function(void *v)
{
    ASSERT_EQ((void *)0x111, v);
    v = system::switch_context(test_instance->child_ctx,
                               test_instance->this_ctx, (void *)0x222);
    ASSERT_EQ((void *)0x333, v);
    v = system::switch_context(test_instance->child_ctx,
                               test_instance->this_ctx, (void *)0x444);
    ASSERT_EQ((void *)0x555, v);
    system::switch_context(test_instance->child_ctx,
                           test_instance->this_ctx, (void *)0x666);
}

TEST_F(test_system, context_switch)
{
    test_instance = this;
    system::stack s = system::allocate_stack(65535);
    child_ctx = system::make_context(s, &context_function);
    ASSERT_TRUE(child_ctx.get());

    void *vvv = system::switch_context(this_ctx, child_ctx, (void *)0x111);
    ASSERT_EQ((void *)0x222, vvv);
    vvv = system::switch_context(this_ctx, child_ctx, (void *)0x333);
    ASSERT_EQ((void *)0x444, vvv);
    vvv = system::switch_context(this_ctx, child_ctx, (void *)0x555);
    ASSERT_EQ((void *)0x666, vvv);

    test_instance = nullptr;
}
