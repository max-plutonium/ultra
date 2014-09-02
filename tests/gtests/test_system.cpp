#include "../../src/core/system.h"
#include <gmock/gmock.h>

TEST(test_system, stack_allocation)
{
    using namespace ultra::core;

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
