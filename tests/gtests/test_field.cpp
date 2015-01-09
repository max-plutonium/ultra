#include "../../src/field.h"
#include <gmock/gmock.h>

using namespace ultra;

TEST(test_field, create_interp)
{
    field fld(0, 0, 1);
    auto int1 = fld.create_interp(1);
    auto int2 = fld.create_interp(2);
    auto int3 = fld.create_interp(3);

    EXPECT_EQ(address(0, 0, 1, 1), int1->get_address());
    EXPECT_EQ(address(0, 0, 1, 2), int2->get_address());
    EXPECT_EQ(address(0, 0, 1, 3), int3->get_address());

    return;
}
