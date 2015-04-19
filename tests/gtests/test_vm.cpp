#include "../../src/vm.h"
#include <gmock/gmock.h>


class test_vm : public ::testing::Test
{
protected:
};

TEST_F(test_vm, create)
{
    ultra::vm &vm = *ultra::vm::instance();

    return;
}
